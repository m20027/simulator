#-*-coding:utf-8
"""SimpleRayLearnerを用いて摸倣学習を行うためのサンプルスクリプト。
SimpleRayLearnerのコンフィグの大半をコマンドライン引数でjsonファイルを指定することで設定し、
一部のjson化しにくい部分を本スクリプトで設定している。
以下のようにコマンドライン引数としてjsonファイルを与えることで学習が行われる。
python ImitationSample.py config.json

また、摸倣学習を行う際には教師データの指定を行う必要があり、
Trainerのconfigにおいて、
	"input":"/your/expert/data/path/Traj*/*.json",
のようにデータのパスを指定する必要がある。
本サンプルでは、ExpertTrajectortGather.pyで生成された軌跡データ(jsonファイル)を教師データとして用いることを前提としている。
"""
import sys
import json
import ray
from ray.rllib.env.env_context import EnvContext
from SimpleRayLearner import SimpleRayLearner,defaultSinglizedEnvCreator
import OriginalModelSample #Factoryへの登録が必要なものは直接使用せずともインポートしておく必要がある
from ASRCAISim1.addons.rayUtility.extension.policy import DummyInternalRayPolicy
from ASRCAISim1.addons.rayUtility.extension.agents.marwil import RNNMARWILTrainer
from ASRCAISim1.addons.rayUtility.extension.agents.marwil import RNNMARWILTFPolicy
from ASRCAISim1.addons.rayUtility.extension.agents.marwil import RNNMARWILTorchPolicy

availableTrainers={
	"RNNMARWIL":{
		"trainer":RNNMARWILTrainer,
		"tf":RNNMARWILTFPolicy,
		"torch":RNNMARWILTorchPolicy
	},
}

if __name__ == "__main__":
	if(len(sys.argv)>1):
		config=json.load(open(sys.argv[1],'r'))
		ray.init()
		#ray.init("auto") #別途rayのclusterを起動しておく場合はこちら。
		#摸倣学習はシングルエージェント化した環境を与える
		config["envCreator"]=defaultSinglizedEnvCreator
		config["register_env_as"]="ASRCAISim1Singlized"
		config["as_singleagent"]=True
		learner=SimpleRayLearner(
			config,
			availableTrainers
		)
		learner.run()

