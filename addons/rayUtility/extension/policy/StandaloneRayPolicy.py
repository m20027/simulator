#-*-coding:utf-8-*-
import numpy as np
import pickle
from collections import OrderedDict
import gym
import logging
from ray.rllib.utils import merge_dicts
from ray.rllib.utils.spaces.space_utils import flatten_to_single_ndarray
from ray.rllib.models import ModelCatalog
from ray.rllib.models.preprocessors import NoPreprocessor, Preprocessor
from ray.rllib.policy.tf_policy import TFPolicy
from ray.rllib.utils.framework import try_import_tf, try_import_torch
from ASRCAISim1.policy.StandalonePolicy import StandalonePolicy
from ASRCAISim1.addons.rayUtility.extension.common import loadPolicyWeights,savePolicyWeights

tf1, tf, tfv = try_import_tf()
torch, _ = try_import_torch()

logger = logging.getLogger(__name__)

class StandaloneRayPolicy(StandalonePolicy):
	"""Ray.RLLibのPolicyをTrainerから独立して動かすためのクラス。
	"""
	def __init__(self,policyName,config,isLocal=False,isDeterministic=False):
		trainer_config=config["trainer_config"]
		self.name=policyName
		self.policy_class=config["policy_class"]
		self.weight=config.get("weight",None)
		self.policy_spec_config=config.get("policy_spec_config",{})
		self._remote_config=trainer_config
		self._local_config=merge_dicts(
			trainer_config,
			{"tf_session_args": trainer_config["local_tf_session_args"]})
		self.preprocessing_enabled=True
		self.isLocal=isLocal
		self.isDeterministic=isDeterministic
		self.policy=None
	def _build(self,policy_spec,policy_config):
		#derived from ray.rllib.evaluation.RolloutWorker._build_policy_map
		cls,obs_space,act_space,conf=policy_spec
		logger.debug("Creating policy for {}".format(policy_spec))
		merged_conf=merge_dicts(policy_config,conf)
		merged_conf["num_workeres"]=0
		merged_conf["worker_index"]=0
		if(self.preprocessing_enabled):
			self.preprocessor=ModelCatalog.get_preprocessor_for_space(
				obs_space, merged_conf.get("model"))
			obs_space=self.preprocessor.observation_space
		else:
			self.preprocessor=NoPreprocessor(obs_space)
		if isinstance(obs_space, (gym.spaces.Dict, gym.spaces.Tuple)):
			raise ValueError(
				"Found raw Tuple|Dict space as input to policy. "
				"Please preprocess these observations with a "
				"Tuple|DictFlatteningPreprocessor.")
		# Tf.
		framework = policy_config.get("framework", "tf")
		if framework in ["tf2", "tf", "tfe"]:
			assert tf1
			if framework in ["tf2", "tfe"]:
				#assert tf1.executing_eagerly()
				if not tf1.executing_eagerly():
					tf1.enable_eager_execution()
				if hasattr(cls, "as_eager"):
					cls = cls.as_eager()
					if policy_config.get("eager_tracing"):
						cls = cls.with_tracing()
				elif not issubclass(cls, TFPolicy):
					pass  # could be some other type of policy
				else:
					raise ValueError("This policy does not support eager "
									"execution: {}".format(cls))
			with tf1.variable_scope(self.name):
				self.policy = cls(obs_space, act_space, merged_conf)
		# non-tf.
		else:
			self.policy = cls(obs_space, act_space, merged_conf)

		logger.info("Built policy: {}".format(self.policy))
		logger.info("Built preprocessor: {}".format(self.preprocessor))
		if(self.weight is not None):
			loadPolicyWeights(self.policy,self.weight)
	def reset(self):
		self.states={}
		self.prev_action={}
		self.prev_rewards={}
	def step(self,observation,reward,done,info,agentFullName,observation_space,action_space):
		if(done):
			return None
		if(self.policy is None):
			policy_spec=[self.policy_class,observation_space,action_space,self.policy_spec_config]
			if(self.isLocal):
				self._build(policy_spec,self._local_config)
			else:
				self._build(policy_spec,self._remote_config)
		if(not agentFullName in self.states):
			self.states[agentFullName]=self.policy.get_initial_state()
			flat = flatten_to_single_ndarray(self.policy.action_space.sample())
			if hasattr(self.policy.action_space, "dtype"):
				self.prev_action[agentFullName]=np.zeros_like(flat, dtype=self.policy.action_space.dtype)
			else:
				self.prev_action[agentFullName]=np.zeros_like(flat)
			self.prev_rewards[agentFullName]=0.0
		processed_obs=self.preprocessor.transform(observation)
		action,state_out,info=self.policy.compute_single_action(
			processed_obs,
			self.states[agentFullName],
			prev_action = self.prev_action[agentFullName],
			prev_reward = self.prev_rewards[agentFullName],
			explore = not self.isDeterministic
		)
		self.states[agentFullName]=state_out
		self.prev_action[agentFullName]=flatten_to_single_ndarray(action)
		self.prev_rewards[agentFullName]=reward
		return action
