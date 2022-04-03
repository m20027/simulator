#-*-coding:utf-8
import gym
import os
import sys
import json
import glob
import shutil
import copy
import datetime
import numpy as np
import ray
import ray.ray_constants as ray_constants
from ray.rllib.env.env_context import EnvContext
from ray.tune.registry import register_env
from ray.tune.utils.trainable import TrainableUtil
from ray.tune.logger import UnifiedLogger
from ray.tune.result import DEFAULT_RESULTS_DIR
from ray.rllib.policy.sample_batch import DEFAULT_POLICY_ID
from ASRCAISim1.addons.rayUtility.RayManager import RayManager
from ASRCAISim1.addons.rayUtility.RayManager import RaySinglizedEnv
from ASRCAISim1.addons.rayUtility.extension import loadWeights,saveWeights
from ASRCAISim1.addons.rayUtility.extension.policy import DummyInternalRayPolicy

from ray.rllib.agents.impala import ImpalaTrainer
from ray.rllib.agents.impala.vtrace_torch_policy import VTraceTorchPolicy
from ray.rllib.agents.impala.vtrace_tf_policy import VTraceTFPolicy

def defaultEnvCreator(config: EnvContext):
	return RayManager(config)

def defaultSinglizedEnvCreator(config: EnvContext):
	return RaySinglizedEnv(config)

class SimpleRayLearner:
	"""一つ以上のTrainerを並列に動かし、一連の学習を行うためのサンプルクラス。
	このサンプルでは複数のTrainerを跨いだ重み共有の機能は提供していないが、各種Callbackを駆使して重みの読み書きやコンフィグの書き換えを行うような拡張を行うことで実現可能。
	__init__に与えるconfig及びavailableTrainersの記述方法は以下の通り。
	config={
    	"envCreator": Optional[Callable[[EnvContext],gym.Env]], #環境インスタンスを生成する関数。省略時はRayManagerを生成する関数となる。
		"register_env_as": Optional[str], #登録する環境の名称。省略時は"ASRCAISim1"
		"as_singleagent": Optional[bool], #シングルエージェントとして環境及びTrainerを扱うかどうか。デフォルトはFalseだが、MARWILによる摸倣学習を使用する場合にはTrueとする必要がある。
    	"seed":Optional[int], #乱数のシード値。
    	"train_steps":int, #学習のステップ数。rayのTrainer.trainを呼ぶ回数を指す。
    	"refresh_interval":int, #学習中にTrainerの再読込を行う周期。ray 1.8.0時点において発生するメモリリーク対策としてインスタンス再生成によるメモリ解放を試みるもの。
    	"checkpoint_interval":int, #チェックポイントの生成周期。
    	"save_dir":str, #ログの保存場所。rayの全ノードから共通してアクセスできるパスでなければならない。(以下全てのパスについて同様)
    	"experiment_name":str, #試行に付与する名称。save_dir以下にこの名称のディレクトリが生成され、各試行のログはその下にrun_YYYY-mm-dd_HH-MM-SSというディレクトリとして保存される。
    	"restore":Optional[str], #既存のチェックポイントから読み込む場合にチェックポイントを含むログのパスを指定する。上記のrun_YYYY-mm-dd_HH-MM-SSの階層まで指定する。
    	"restore_checkpoint_number":Union[int,"latest",None], #既存のチェックポイントから読み込む場合、チェックポイントの番号を指定する。"latest"と指定することで当該ログ内の最新のチェックポイントを自動で検索する。
    	"trainer_common_config":{ #各Trainerに与えるコンフィグの共通部分を記述する。デフォルト値はray.rllib.trainer.pyを始めとする各Trainerクラスの定義とともに示されている。以下は主要な項目を例示する。
        	"env_config":{ #環境に渡されるコンフィグを記述する。詳細は各Envクラスの定義を参照のこと。
            	"config":Union[dict,list[Union[dict,str]], #GymManagerクラスで要求
				"overrider": Optional[Callable[[dict,int,int],dict]], #GymManagerクラスで要求
				"target": Union[Callable[[str],bool],str], #SinglizedEnvクラスで要求
				"policies": Dict[str,StandalonePolicy], #SinglizedEnvクラスで要求
				"policyMapper": Optional[Callable[[str],str]], #SinglizedEnvクラスで要求
				"exposeImitator": bool (False if omitted), #SinglizedEnvクラスで要求
				"runUntilAllDone": bool (True if omitted), #SinglizedEnvクラスで要求
        	},
        	"model":dict,#NNモデルの構造を定義する。ray.rllib.models.catalog.pyに設定項目の一覧とデフォルト値が示されている。
    	},
    	"trainers":{ #Trainer名をキーとしたdictにより、生成するTrainerインスタンスを指定する。
	        <Trainer's name>:{
				"trainer_type": str, #Trainerの種類。availableTrainersのキーから選択する。
            	"config_overrider":Optional[dict[str,Any]], #trainer_common_configを上書きするためのdict。主に"num_workers"を上書きすることになる。
            	"policies_to_train":list[str], #このTrainerにより学習を行うPolicy名のリスト。
        	},
			...
    	},
		"policies": { #Policyに関する設定。Policy名をキーとしたdictで与える。
			<Policy's name>: {
				"initial_weight": None or str, #初期重み(リセット時も含む)のパス。
				"save_path_at_the_end": None or str, #学習終了時の重みを別途保存したい場合、そのパスを記述する。
			},
			...
		},
	}
	availableTrainers={
		<trainer_type>:{
			"trainer": Trainer, #使用するTrainerクラス。
			"tf": TFPolicy, #Tensorflowを使用する場合のPolicyクラス。
			"torch": TorchPolicy, #PyTorchを使用する場合のPolicyクラス。
			"internal": Optional[Policy], #actionをInternalに計算するAgentに対応する、ダミーのPolicyクラス。省略時はDummyInternalRayPolicyとなる。
	}

	また、保存されるログは
	logdir="run_"+datetime.datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
	experiment_log_path=os.path.join(config["save_dir"],config["experiment_name"],logdir)
	として、以下のような構成となる。なお、拡張子datで保存される学習モデル(重み)は、
	Policy.get_weights()で得られるnp.arrayのlist又はdictをそのままpickleでdumpしたものである。
	experiment_log_path
		policies
			initial_weight
				<policy>.dat
			checkpoints
				checkpoint_<step>
					<policy>.dat
		trainers
			<trainer>
				checkpoint_<step>
					checkpoint-<step>
				<other files created by ray>
	"""
	def __init__(self,
				config,
				availableTrainers={
					"IMPALA":{
						"trainer":ImpalaTrainer,
						"tf":VTraceTFPolicy,
						"torch":VTraceTorchPolicy,
						"inernal":DummyInternalRayPolicy
					}
				}
				):
		"""初期化処理
		
		Args:
			config (dict): 学習の各種設定値を記述したdict。
			aveilableTrainers (dict): 指定可能なTrainerの一覧を示すdict。
		"""
		self.config=config
		self.availableTrainers=availableTrainers
	def _policySetup(self,trainer_type,trainer_config,policies_to_train):
		"""対象のTrainerインスタンスにとって必要となるPolicyインスタンスを生成する。
		本サンプルでは、SimulationManagerのコンフィグが以下のように記述されていることを前提とする。
		(1)BlueとRedのAgentがAgentConfigDispatcherの"BlueAgents"と"RedAgents"をaliasとして参照することとされている。
		(2)AgentConfigDispatcherにおいて、各policy名に対応するAgentモデルを表すコンフィグが同名のaliasとして記述されている。
		(3)AgentConfigDispatcherにおいて、"BlueAgents"と"RedAgents"が"type"=="alias"であり、これらに(2)に該当するpolicy名のaliasを指定することで適切なコンフィグが得られるものとなっている。

		Args:
			trainer_type (str): Trainerの種類を表す文字列。self.availableTrainersのキーから選択する。
			trainer_config (dict): 対象とするTrainerインスタンスを生成するためのコンフィグ。
			policies_to_train (list): 対象とするTrainerインスタンスが学習対象とするPolicyの名前のリスト。
		"""
		policies={}
		#policies_to_trainに含まれる全種類のPolicyを一度ずつ登場させ、spaceの情報を取得する。
		dummyConfig=copy.deepcopy(trainer_config["env_config"])
		dummyConfig["config"]["Manager"]["Viewer"]="None"
		dummyConfig["config"]["Manager"]["Loggers"]={}
		for policyName in self.config["policies"]:
			agentConfigDispatcher=dummyConfig["config"]["Manager"]["AgentConfigDispatcher"]
			numBlue=len(agentConfigDispatcher["BlueAgents"]["overrider"][0]["elements"])
			numRed=len(agentConfigDispatcher["RedAgents"]["overrider"][0]["elements"])
			agentConfigDispatcher["BlueAgents"]["alias"]=policyName
			agentConfigDispatcher["BlueAgents"]["overrider"][0]["elements"]=[
				{"type":"direct","value":{"name":"Blue"+str(i+1),"policy":policyName}}
				for i in range(numBlue)
			]
			agentConfigDispatcher["RedAgents"]["alias"]=policyName
			agentConfigDispatcher["RedAgents"]["overrider"][0]["elements"]=[
				{"type":"direct","value":{"name":"Red"+str(i+1),"policy":policyName}}
				for i in range(numRed)
			]
			dummyEnv=RayManager(EnvContext(
				dummyConfig,
				0,
				0
			))
			dummyEnv.reset()
			ob=dummyEnv.get_observation_space()
			ac=dummyEnv.get_action_space()
			for key in ac:
				_,m_name,p_name=key.split(":")
				if(p_name==policyName):
					policyClass=self.availableTrainers[trainer_type]["torch"] if trainer_config.get("framework")=="torch" else self.availableTrainers[trainer_type]["tf"]
					policy=[policyClass,ob[key],ac[key],{}]
					if(policyName in policies_to_train):
						policies[policyName]=policy
				elif(p_name=="Internal"):
					policies[m_name]=[self.availableTrainers[trainer_type].get("internal",DummyInternalRayPolicy),ob[key],ac[key],{}]
				else:
					raise ValueError("Invalid policy config.")
		return policies
	def run(self):
		"""
		configに従い学習を実行する。
		"""
		if(self.config["trainer_common_config"]["framework"]=="tf"):
			import tensorflow as tf
			tf.compat.v1.disable_eager_execution()
		if(not ray.is_initialized()):
			#rayの初期化がされていない場合、ここで初期化する。外側で初期化しておけば複数台のPCに処理を分散させることも可能。
			ray.init()
		#Environmentの登録(envCreatorに別のCallableを与えることで、環境の生成をカスタマイズすることも可能)
		envCreator=self.config.get("envCreator",defaultEnvCreator)
		envName=self.config.get("register_env_as","ASRCAISim1")
		self.isSingleAgent=self.config.get("as_singleagent",False) #Env,Trainerともにシングルエージェントとして動かす場合にTrueとする。
		register_env(envName,envCreator)
		#チェックポイントの読み込み設定
		# restore (str or None): 読み込むログのパス。後述のexperiment_log_pathに相当。
		# restore_checkpoint_number (int or "latest"): チェックポイントの番号。"latest"を指定すると最新のものが自動的に検索される。
		restore_path=self.config.get("restore",None)
		if(restore_path is not None):
			restore_checkpoint_number=self.config.get("restore_checkpoint_number","latest")
			if(restore_checkpoint_number=="latest"):
				restore_checkpoint_number=max([int(os.path.basename(c)[11:])
					for c in glob.glob(os.path.join(restore_path,"**","checkpoint_*"),recursive=True)])
			assert(isinstance(restore_checkpoint_number,int))
		# ログ保存ディレクトリの設定を行う。
		# save_dir (str): 保存先のディレクトリ。省略時はrayのデフォルト(~/ray_results)となる。
		# experiment_name (str): 試行の名称。省略時は"PFSPLearner"となる。
		# save_dir/experiment_name/run_YYYY-mm-dd-HH-MM-SS/というディレクトリ(=experiment_log_path)以下に各ログファイルが生成される。
		save_dir=self.config.get("save_dir",DEFAULT_RESULTS_DIR)
		experiment_name=self.config.get("experiment_name","PFSPLearner")
		suffix=datetime.datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
		experiment_log_path=os.path.join(save_dir,experiment_name,"run_"+suffix)
		#初期重みとして指定した重みファイルのコピー
		if(restore_path is None):
			#チェックポイントからの再開でないとき、configからパスを取得
			for key,value in self.config["policies"].items():
				initial_weight=value.get("initial_weight",None)
				if(initial_weight is not None):
					dstDir=os.path.join(experiment_log_path,"policies","initial_weight")
					if(not os.path.exists(dstDir)):
						os.makedirs(dstDir)
					shutil.copy2(initial_weight,os.path.join(dstDir,key+".dat"))
		else:
			#チェックポイントからの再開のとき、読み込み元からコピー
			for key in self.config["policies"].keys():
				initial_weight=os.path.join(restore_path,"policies","initial_weight",key+".dat")
				if(os.path.exists(initial_weight)):
					dstDir=os.path.join(experiment_log_path,"policies","initial_weight")
					if(not os.path.exists(dstDir)):
						os.makedirs(dstDir)
					shutil.copy2(initial_weight,os.path.join(dstDir,key+".dat"))
		#初期重みのパスの置換
		for key in self.config["policies"].keys():
			initial_weight=os.path.join(experiment_log_path,"policies","initial_weight",key+".dat")
			if(os.path.exists(initial_weight)):
				self.config["policies"][key]["initial_weight"]=initial_weight
			else:
				self.config["policies"][key]["initial_weight"]=None
		#ダミー環境を一度生成し、完全な形のenv_configを取得する。
		dummyEnv=RayManager(EnvContext(
			self.config["trainer_common_config"]["env_config"],
			0,
			0
		))
		self.completeEnvConfig={
			key:value for key,value in self.config["trainer_common_config"]["env_config"].items() if key!="config"
		}
		self.completeEnvConfig["config"]={
				"Manager":dummyEnv.getManagerConfig()(),
				"Factory":dummyEnv.getFactoryModelConfig()()
			}
		self.config["trainer_common_config"]["env_config"]=self.completeEnvConfig
		def policyMapper(agentId,episode=None,**kwargs):
			"""
			エージェントのfullNameから対応するポリシー名を抽出する関数。
			agentId=agentName:modelName:policyName
			"""
			agentName,modelName,policyName=agentId.split(":")
			if(policyName=="Internal"):
				policyName=modelName
			return policyName
		#初期重みの読み込み
		initial_weights={}
		for key,value in self.config["policies"].items():
			initial_weight=value.get("initial_weight",None)
			if(initial_weight is not None):
				initial_weights[key]=loadWeights(initial_weight)
		#Trainerインスタンスの生成
		trainer_index=0
		trainers={}
		refresh_interval=self.config.get("refresh_interval",0)
		if(refresh_interval>=1):
			#リフレッシュ有りの場合
			trainer_construct_info={}
		policies_on_which_trainer={}
		for trainerName,trainerSpec in self.config["trainers"].items():
			trainer_config=copy.deepcopy(self.config["trainer_common_config"])
			trainer_config["env"]=envName
			if("seed" in self.config):
				trainer_config["env_config"]["config"]["Manager"]["seed"]=self.config["seed"]+trainer_index
			elif(not "seed" in trainer_config["env_config"]["config"]["Manager"]):
				trainer_config["env_config"]["config"]["Manager"]["seed"]=np.random.randint(2**31)+trainer_index
			tc_overrider=trainerSpec.get("config_overrider",None)
			if(tc_overrider is not None):
				trainer_config.update(tc_overrider)
			policies_to_train=trainerSpec.get("policies_to_train",[])
			#Policyインスタンスの生成
			policies=self._policySetup(trainerSpec["trainer_type"],trainer_config,policies_to_train)
			print("===========policies for ",trainerName,"===========")
			print(list(policies))
			print("===========policies for ",trainerName,"===========")
			#SimulationManagerのインスタンスごとにシードを変えるためのoverriderを追加
			def overrider(config,worker_index,vector_index):
				nw=trainer_config["num_workers"]
				if("seed" in config["Manager"]):
					config["Manager"]["seed"]=config["Manager"]["seed"]+worker_index+nw*vector_index
				return config
			trainer_config["env_config"]["overrider"]=overrider
			if(not self.isSingleAgent):
				trainer_config["multiagent"]={
					"policies":policies,
					"policy_mapping_fn":policyMapper,
					"policies_to_train":policies_to_train
				}	
			else:
				# シングルエージェントとして扱う場合、policies及びpolicies_to_trainには一つのみ指定する。
				# なお、そのポリシーは、TrainerとしてはDEFAULT_POLICY_ID="default_policy"という名称で保持することになる。
				if(len(policies)!=1 or len(policies_to_train)!=1):
					raise ValueError("You must specify exactly 1 policy for each trainer, under 'single agent' condition.")
				trainer_config["multiagent"]={
					"policies":{DEFAULT_POLICY_ID:next(iter(policies.values()))}
				}
			#各Trainerインスタンスのログ保存ディレクトリの設定
			def get_logger_creator(trainerName_):
				experiment_log_path_for_trainer=os.path.join(experiment_log_path,"trainers",trainerName_)
				if not os.path.exists(experiment_log_path_for_trainer):
					os.makedirs(experiment_log_path_for_trainer)
				def logger_creator(config):
					return UnifiedLogger(config, experiment_log_path_for_trainer, loggers=None)
				return logger_creator
			trainerClass=self.availableTrainers[trainerSpec["trainer_type"]]["trainer"]
			remoteTrainerClass=ray.remote(num_gpus=trainer_config["num_gpus"])(trainerClass)
			logger_creator=get_logger_creator(trainerName)
			if(refresh_interval>=1):
				#リフレッシュ有りの場合、各Trainerの再構成に必要な情報を残しておく
				trainer_construct_info[trainerName]={
					"class":remoteTrainerClass,
					"config":trainer_config,
					"logger_creator":logger_creator
				}
			trainers[trainerName]=remoteTrainerClass.remote(config=trainer_config,logger_creator=logger_creator)
			for policyName in policies_to_train:
				if(policyName in policies_on_which_trainer):
					raise ValueError("A policy can be trained by only one trainer.")
				policies_on_which_trainer[policyName]=trainerName
			#初期重みのTrainerへのセット
			initial_weights_for_trainer={key:initial_weights[key] for key in policies_to_train if key in initial_weights}
			if(len(initial_weights_for_trainer)>0):
				if(self.isSingleAgent):
					initial_weights_for_trainer={DEFAULT_POLICY_ID:next(iter(initial_weights_for_trainer))}
				ray.get(trainers[trainerName].set_weights.remote(initial_weights_for_trainer))
			#チェックポイントから再開する場合、ここで読み込みを実施
			if(restore_path is not None):
				checkpoints=[c
					for c in glob.glob(os.path.join(restore_path,"trainers",trainerName,"**","checkpoint-*"),recursive=True)
					if not ".tune_metadata" in c]
				checkpoint=checkpoints[[int(os.path.basename(c)[11:]) for c in checkpoints].index(restore_checkpoint_number)]
				ray.get(trainers[trainerName].restore.remote(checkpoint))
			trainer_index+=1
		trainingWeights={}
		for trainerName,trainer in trainers.items():
			if(len(self.config["trainers"][trainerName]["policies_to_train"])>0):
				if(self.isSingleAgent):
					policyName=next(iter(self.config["trainers"][trainerName]["policies_to_train"]))
					trainingWeights.update({policyName:ray.get(trainer.get_weights.remote([DEFAULT_POLICY_ID]))[DEFAULT_POLICY_ID]})
				else:
					trainingWeights.update(ray.get(trainer.get_weights.remote(self.config["trainers"][trainerName]["policies_to_train"])))
		#学習の実行
		latestCheckpoint={}
		for i in range(self.config["train_steps"]):
			if(restore_path is None):
				total_steps=i+1
			else:
				total_steps=restore_checkpoint_number+i+1
			results=ray.get([trainer.train.remote() for trainer in trainers.values()])
			for trainerName,trainer in trainers.items():
				if(len(self.config["trainers"][trainerName]["policies_to_train"])>0):
					if(self.isSingleAgent):
						policyName=next(iter(self.config["trainers"][trainerName]["policies_to_train"]))
						trainingWeights.update({policyName:ray.get(trainer.get_weights.remote([DEFAULT_POLICY_ID]))[DEFAULT_POLICY_ID]})
					else:
						trainingWeights.update(ray.get(trainer.get_weights.remote(self.config["trainers"][trainerName]["policies_to_train"])))
			if(total_steps%self.config["checkpoint_interval"]==0 or (i+1)==self.config["train_steps"]):
				#途中経過の保存(デフォルトでは"~/ray_results/以下に保存される。)
				for trainerName,trainer in trainers.items():
					latestCheckpoint[trainerName]=ray.get(trainer.save.remote())
				#重みの保存
				checkpoint_dir=TrainableUtil.make_checkpoint_dir(os.path.join(experiment_log_path,"policies/checkpoints"),total_steps)
				for key,weights in trainingWeights.items():
					filename=key+".dat"
					saveWeights(weights,os.path.join(checkpoint_dir,filename))
				print("Checkpoint at ",total_steps,"-th step is saved.")
			if(refresh_interval>=1 and total_steps%refresh_interval==0 and (i+1)<self.config["train_steps"]):
				#Trainerのリフレッシュ(ray1.8.0時点で存在するメモリリーク対策)
				#チェックポイントを作成し、新たなTrainerインスタンスで読み直すことで引き継ぐ
				print("===================refresh!==============")
				#このステップでチェックポイントが未作成の場合、作成
				if(total_steps%self.config["checkpoint_interval"]==0 or (i+1)==self.config["train_steps"]):
					pass
				else:
					for trainerName,trainer in trainers.items():
						latestCheckpoint[trainerName]=ray.get(trainer.save.remote())
					checkpoint_dir=TrainableUtil.make_checkpoint_dir(os.path.join(experiment_log_path,"policies","checkpoints"),total_steps)
					for key,weights in trainingWeights.items():
						filename=key+".dat"
						saveWeights(weights,os.path.join(checkpoint_dir,filename))
				#Trainerインスタンスの再生成とチェックポイントの読み込み
				print("latestCheckpoint=",latestCheckpoint)
				for trainerName,trainer in trainers.items():
					ray.get(trainer.cleanup.remote())
					ray.kill(trainer)
				trainers={}
				for trainerName,info in trainer_construct_info.items():
					trainers[trainerName]=info["class"].remote(config=info["config"],logger_creator=info["logger_creator"])
					ray.get(trainers[trainerName].restore.remote(latestCheckpoint[trainerName]))
		#学習終了時の重み保存先が指定されているものについて、指定されたパスへ保存する。
		for key,value in self.config["policies"].items():
			save_path_at_the_end=value.get("save_path_at_the_end",None)
			if(save_path_at_the_end is not None and key in trainingWeights):
				weights=trainingWeights[key]
				saveWeights(weights,save_path_at_the_end)
		print("Training is finished.")
		[trainer.cleanup.remote() for trainer in trainers.values()]
