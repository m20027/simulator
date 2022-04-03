#-*-coding:utf-8-*-
from typing import Any, Callable, Dict
import gym
from ASRCAISim1.libCore import SimulationManager,ExpertWrapper
from ASRCAISim1.policy import StandalonePolicy

class GymManager(gym.Env):
	"""
	gym.Envを継承した、SimulationManagerのラッパークラス。
	Ray.RLLibのインターフェースに準じて、コンストラクタに与える引数はrayのEnvContextに相当するdict型のcontextのみ。
    context={
        "config":Union[Dict[str,Any]|List[Union[Dict[str,Any]|str]]],
        "overrider":Optional[Callable[[Dict[str,Any],int,int],Dict[str,Any]]],
        "worker_index":int,
        "vector_index":int
    }
		Args:
			context (dict): context["config"]にSimulationManagerに渡すjsonを持たせる。また、必要に応じて、context["overrider"]にworker_index及びvector_indexに応じたconfig置換関数(std::function<nl::json(const nl::json&,int,int)>相当)を与える。
	"""
	def __init__(self,context: Dict[str,Any]):
		self.context=context
		self.worker_index=context.get("worker_index",0)
		self.vector_index=context.get("vector_index",0)
		self.manager=SimulationManager(context["config"],self.worker_index,self.vector_index,context.get("overrider",None))
		self.get_observation_space()
		self.get_action_space()
	def render(self,mode="Default"):
		"""gym.Envのインターフェースに合わせて実装しているが、
		現時点では、Viewerクラスが独自に描画するため、render関数としては動作しない。
		"""
		pass
	def seed(self,seed=None):
		"""乱数のseedを設定する。
		"""
		return self.manager.seed(seed)
	def reset(self):
		"""環境のリセットを行う。
		"""
		ret=self.manager.reset()
		self.get_observation_space()
		self.get_action_space()
		return ret
	def step(self,action):
		"""環境を1ステップすすめる。
		"""
		return self.manager.step(action)
	def get_observation_space(self):
		"""gym.Envとして外部に見せる状態空間を計算する。

		gymのインターフェースを厳密に守ると、今のところ状態空間と行動空間は固定であり、かつメソッドでなくプロパティとして保持しなければならず、stable baselinesもその前提で実装されている。
		しかし、このManagerはエピソードごとに可変とすることもできてしまうため、
		LearnerとExpertの種類・数が固定となるようにconfigの作成時に気をつけるものとする。
		"""
		self.observation_space=gym.spaces.Dict(self.manager.get_observation_space())
		return self.observation_space
	def get_action_space(self):
		"""gym.Envとして外部に見せる行動空間を計算する。

		gymのインターフェースを厳密に守ると、今のところ状態空間と行動空間は固定であり、かつメソッドでなくプロパティとして保持しなければならず、stable baselinesもその前提で実装されている。
		しかし、このManagerはエピソードごとに可変とすることもできてしまうため、
		LearnerとExpertの種類・数が固定となるようにconfigの作成時に気をつけるものとする。
		"""
		self.action_space=gym.spaces.Dict(self.manager.get_action_space())
		return self.action_space
	def getManagerConfig(self):
		return self.manager.getManagerConfig()
	def getFactoryModelConfig(self):
		return self.manager.getFactoryModelConfig()
	def setViewerType(self,viewerType):
		self.manager.setViewerType(viewerType)
	def requestReconfigure(self,managerReplacer,factoryReplacer):
		"""configの変更を予約する。現在実施中のエピソードが終了後、次のreset時に反映される。
		反映は、dictのupdateにより行われるため、対応するキーの値が置き換わるのみであり、dictそのものの置き換えではない。
		Args:
			managerReplacer (dict): Managerのconfigを置換するためのdict
			factoryReplacer (dict): Factoryのconfigを置換するためのdictであり、rootでなくmodels以下の階層のみで渡す。
		"""
		self.manager.requestReconfigure(managerReplacer,factoryReplacer)

def getDefaultPolicyMapper(mapperForAuto=None):
	"""エージェントとポリシーのマッピング関数を生成する。
		基本的には環境側のconfigでポリシー名を指定しておくものとする。
		環境の出力となる辞書のキーとなるAgentのfullNameは、fullName=agentName:modelName:policyNameの形式としている。
		Args:
			mapperForAuto (callable): エージェント名とモデル名を引数にとりポリシー名を返す任意の関数。環境側のポリシー名が"Auto"のときに呼ばれる。
	"""
	def ret(fullName: str):
		agentName,modelName,policyName=fullName.split(":")
		if(mapperForAuto is not None and policyName=="Auto"):
			policyName=mapperForAuto(agentName,modelName)
		return policyName
	return ret

class SinglizedEnv(GymManager):
	"""一つを除いたAgentのactionを内部で処理することによって、シングルエージェント環境として扱うためのラッパークラスの実装例。
	contextの追加要素={
        "target": Union[Callable[[str],bool],str],
        "policies": Dict[str,StandalonePolicy]
		"policyMapper": Optional[Callable[[str],str]],
		"exposeImitator": bool (False if omitted),
		"runUntilAllDone": bool (True if omitted)
    }
		Args:
			target (Union[Callable[[str],bool],str]): observation,actionの入出力対象のAgentを特定するための関数。AgentのfullNameを引数にとってboolを返す関数を与える。最初にTrueとなったAgentを対象とみなす。
				Callableの代わりに文字列を指定した場合、その文字列で始まるかどうか(startswith)を判定条件にすることが可能。
			policies (Dict[str,StandalonePolicy]): 内部でactionを計算するためのStandalonePolicyオブジェクトのdict。キーはpolicyの名称となる。
			policyMapper (Callable[[str],str]): AgentのfullNameから使用するpolicyの名称を計算する。省略した場合はfullNameの末尾にあるpolicyName部分が使われる。
			exposeImitator (bool): ExpertWrapperをtargetとする際、Imitatorのobservationとactionを入出力対象として扱うか否か。デフォルトはFalse。
			runUntilAllDone (bool): 対象AgentのdoneがTrueとなっても、done["__all__"]がTrueとなるまで内部でエピソードの計算を継続するか否か。デフォルトはTrue。
	"""
	def __init__(self,context: Dict[str,Any]):
		target_=context["target"]
		if(isinstance(target_,str)):
			self.target=lambda fullName:fullName.startswith(target_)
		elif(isinstance(target_,Callable)):
			self.target=target_
		else:
			raise ValueError("SinglizedEnv requires Callable[[str],bool] or str for 'target' in context.")
		self.targetAgent=None
		self.policies=context.get("policies",{})
		self.originalPolicyMapper=context.get("policyMapper",getDefaultPolicyMapper())
		self.policyMapper=self.originalPolicyMapper
		self.exposeImitator=context.get("exposeImitator",False)
		self.runUntilAllDone=context.get("runUntilAllDone",True)
		super().__init__(context)
	def reset(self):
		self.targetAgent=None
		self.obs=super().reset()
		self.rewards={}
		self.rewards={k:0.0 for k in self.obs.keys()}
		self.dones={k:False for k in self.obs.keys()}
		self.infos={k:None for k in self.obs.keys()}
		self.dones["__all__"]=False
		for p in self.policies.values():
			p.reset()
		if(self.isExpertWrapper):
			self.targetObs=self.targetAgent.imitatorObs
		else:
			self.targetObs=self.obs[self.targetIdentifier]
		return self.targetObs
	def step(self,action):
		observation_space=self.get_observation_space()
		action_space=self.get_action_space()
		actions={k:self.policies[self.policyMapper(k)].step(
			o,
			self.rewards[k],
			self.dones[k],
			self.infos[k],
			k,
			observation_space[k],
			action_space[k]
		) for k,o in self.obs.items() if self.policyMapper(k) in self.policies}
		if(not (self.isExpertWrapper and self.exposeImitator)):
			actions[self.targetIdentifier]=action
		self.obs,self.rewards,self.dones,self.infos=super().step(actions)
		if(self.isExpertWrapper):
			self.targetObs=self.targetAgent.imitatorObs
		else:
			self.targetObs=self.obs[self.targetIdentifier]
		ret=self.targetObs,self.rewards[self.targetIdentifier],self.dones[self.targetIdentifier],self.infos[self.targetIdentifier]
		if(self.dones[self.targetIdentifier] and not self.dones["__all__"]):
			if(self.runUntilAllDone):
				while(not self.dones["__all__"]):
					observation_space=self.get_observation_space()
					action_space=self.get_action_space()
					actions={k:self.policies[self.policyMapper(k)].step(o,k,observation_space[k],action_space[k]) for k,o in self.obs.items() if self.policyMapper(k) in self.policies}
					if(self.targetIdentifier in actions):
						actions.pop(self.targetIdentifier)
					self.obs,self.rewards,self.dones,self.infos=super().step(actions)
			else:
				self.manager.stopEpisodeExternally()
		return ret
	def get_observation_space(self):
		self.manager.get_observation_space()
		if(self.targetAgent is None):
			self.setupTarget()
		if(self.isExpertWrapper and self.exposeImitator):
			self.observation_space=self.targetAgent.imitator_observation_space()
		else:
			self.observation_space=self.targetAgent.observation_space()
		return self.observation_space
	def get_action_space(self):
		self.manager.get_action_space()
		if(self.targetAgent is None):
			self.setupTarget()
		if(self.isExpertWrapper and self.exposeImitator):
			self.action_space=self.targetAgent.imitator_action_space()
		else:
			self.action_space=self.targetAgent.action_space()
		return self.action_space
	def setupTarget(self):
		agents={agent.getFullName():agent for agent in self.manager.getAgents()}
		for fullName,agent in agents.items():
			if(self.target(fullName)):
				self.targetIdentifier=fullName
				self.targetAgent=agent
				self.isExpertWrapper=isinstance(self.targetAgent,ExpertWrapper)
				break
		assert self.targetAgent is not None

class SimpleEvaluator:
	"""全Agentの行動判断をStandalonePolicyにより内部で処理する評価用環境の最小構成。
	勝敗や得点の抽出等、結果の取得・利用はCallbackを用いてもよいし、このクラスを改変して実装してもよい。
		Args:
			context (Dict[str,Any]): GymManagerに与えるcontext。
			policies (Dict[str,StandalonePolicy]): 内部でactionを計算するためのStandalonePolicyオブジェクトのdict。キーはpolicyの名称となる。
			policyMapper (Callable[[str],str]): AgentのfullNameから使用するpolicyの名称を計算する。省略した場合はfullNameの末尾にあるpolicyName部分が使われる。
	"""
	def __init__(self,context: Dict[str,Any], policies: Dict[str,StandalonePolicy], policyMapper: Callable[[str],str]=getDefaultPolicyMapper()):
		self.env=GymManager(context)
		self.policies=policies
		self.policyMapper=policyMapper
	def run(self):
		obs=self.env.reset()
		rewards={k:0.0 for k in obs.keys()}
		dones={k:False for k in obs.keys()}
		infos={k:None for k in obs.keys()}
		for p in self.policies.values():
			p.reset()
		dones["__all__"]=False
		while not dones["__all__"]:
			observation_space=self.env.get_observation_space()
			action_space=self.env.get_action_space()
			actions={k:self.policies[self.policyMapper(k)].step(
				o,
				rewards[k],
				dones[k],
				infos[k],
				k,
				observation_space[k],
				action_space[k]
			) for k,o in obs.items() if self.policyMapper(k) in self.policies}
			obs, rewards, dones, infos = self.env.step(actions)
