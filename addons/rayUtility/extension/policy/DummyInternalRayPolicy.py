#-*-coding:utf-8
from ray.rllib.policy import Policy
import numpy as np
class DummyInternalRayPolicy(Policy):
	"""
	InternalなAgentはobs,actionともにDiscrete(1)を設定しているので適当にsampleして返すだけ。
	"""
	def __init__(self,observation_space,action_space,config):
		super().__init__(observation_space,action_space,config)
	def compute_actions(
		self,
		obs_batch,
		state_batches=None,
		prev_action_batch=None,
		prev_reward_batch=None,
		info_batch=None,
		episodes=None,
		explore=None,
		timestep=None,
		**kwargs):
		return np.stack([self.action_space.sample() for _ in obs_batch],axis=0),[],{}
	def learn_on_batch(self,samples):
		pass
	def get_weights(self):
		pass
	def set_weights(self,weights):
		pass