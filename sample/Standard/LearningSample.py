#-*-coding:utf-8
"""SimpleRayLearnerを用いて分散学習を行うためのサンプルスクリプト。
SimpleRayLearnerのコンフィグの大半をコマンドライン引数でjsonファイルを指定することで設定し、
一部のjson化しにくい部分を本スクリプトで設定している。
以下のようにコマンドライン引数としてjsonファイルを与えることで学習が行われる。
python LearningSample.py config.json
"""
import sys
import json
import ray
from SimpleRayLearner import SimpleRayLearner
import OriginalModelSample #Factoryへの登録が必要なものは直接使用せずともインポートしておく必要がある
from ASRCAISim1.addons.rayUtility.extension.policy import DummyInternalRayPolicy
from ray.rllib.agents.impala import ImpalaTrainer
from ray.rllib.agents.ppo import APPOTrainer
from ray.rllib.agents.impala.vtrace_tf_policy import VTraceTFPolicy
from ray.rllib.agents.impala.vtrace_torch_policy import VTraceTorchPolicy
from ray.rllib.agents.ppo.appo_tf_policy import AsyncPPOTFPolicy
from ray.rllib.agents.ppo.appo_torch_policy import AsyncPPOTorchPolicy

class DummyInternalRayPolicyForAPPO(DummyInternalRayPolicy):
	#APPOの場合、self.update_targetとして引数なしメソッドを持っていなければならない。
	#
	def __init__(self,observation_space,action_space,config):
		super().__init__(observation_space,action_space,config)
		def do_update_dummy():
			return
		self.update_target=do_update_dummy


availableTrainers={
	"IMPALA":{
		"trainer":ImpalaTrainer,
		"tf":VTraceTFPolicy,
		"torch":VTraceTorchPolicy
	},
	"APPO":{
		"trainer":APPOTrainer,
		"tf":AsyncPPOTFPolicy,
		"torch":AsyncPPOTorchPolicy,
		"internal":DummyInternalRayPolicyForAPPO
	},
}

if __name__ == "__main__":
	if(len(sys.argv)>1):
		config=json.load(open(sys.argv[1],'r'))
		ray.init()
		#ray.init("auto") #別途rayのclusterを起動しておく場合はこちら。
		learner=SimpleRayLearner(
			config,
			availableTrainers
		)
		learner.run()

