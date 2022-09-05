#-*-coding:utf-8-*-

class StandalonePolicy:
	"""強化学習フレームワークから独立させてPolicyを使用するためのインターフェース。
	Environmentのresetとstepの呼び出しに合わせてこのクラスのresetとstepが呼ばれるものとする。
	"""
	def reset(self):
		"""Episodeの初期化処理を記述する。複数のAgentで共用する場合も1回のみ呼ばれる。特定のAgentに依存する処理が必要な場合はstep関数にて実施するものとする。

			Args:
				agentFullName (str): 計算対象のAgentの完全な名称であり、agentName:modelName:policyNameの形をとる。
		"""
		pass
	def step(self,observation,reward,done,info,agentFullName,observation_space,action_space):
		"""observation等からactionを計算する処理を記述する。

			Args:
				observation: Environmentから得られたobservation。
				reward: Environmentから得られたreward。
				done: Environmentから得られたdone。
				info: Environmentから得られたinfo。
				agentFullName (str): 計算対象のAgentの完全な名称であり、agentName:modelName:policyNameの形をとる。
				observation_space (gym.space.Space): 与えられたObservationのSpace。
				action_space (gym.space.Space): 計算すべきActionのSpace。
		"""
		#if(done):
		#	return None
		#action= some_computation(observation)
		#return action
		return None