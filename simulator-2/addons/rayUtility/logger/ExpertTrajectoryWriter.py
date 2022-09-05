#-*-coding:utf-8-*-
from math import *
import sys,os,time
import numpy as np
from datetime import datetime
from ASRCAISim1.libCore import *
import ray
from ray.rllib.policy.sample_batch import SampleBatch
from ray.rllib.policy.view_requirement import ViewRequirement
from ray.rllib.evaluation.collectors.simple_list_collector import _AgentCollector
from ray.rllib.evaluation.sample_batch_builder import SampleBatchBuilder
from ray.rllib.utils.spaces.space_utils import flatten_to_single_ndarray
from ray.rllib.models.preprocessors import get_preprocessor
from ray.rllib.offline.io_context import IOContext
from ray.rllib.offline.json_writer import JsonWriter

class ExpertTrajectoryWriter(Callback):
	"""異なるobservation,action形式を持つAgentを摸倣する際のTrajectory記録用のLogger。
	RLlibのSampleBatchBuilderを使用する。
	Managerのconfigで指定することを要求する。
	"""
	def __init__(self,modelConfig,instanceConfig):
		super().__init__(modelConfig,instanceConfig)
		if(self.isDummy):
			return
		self.prefix=getValueFromJsonKRD(self.modelConfig,"prefix",self.randomGen,"")
		self.episodeCounter=-1
		self.writers={}
		self.view_reqs={}
		self.agentCollectors={}
		self.preprocessors={}
		self.dones={}
	def onEpisodeBegin(self):
		self.episodeCounter+=1
		agent_index=0
		for agent in self.manager.experts.values():
			identifier=agent.trajectoryIdentifier
			if(not identifier in self.writers):
				if(len(self.prefix)==0 or self.prefix[-1]=='/'):
					path=self.prefix+identifier
				else:
					path=self.prefix+"_"+identifier
				os.makedirs(os.path.dirname(path),exist_ok=True)
				ioctx=IOContext()
				ioctx.worker_index=self.manager.worker_index
				self.writers[identifier]=JsonWriter(path,ioctx)
				self.preprocessors[identifier]=get_preprocessor(agent.imitator_observation_space())(agent.imitator_observation_space())
				self.view_reqs[identifier]={
					SampleBatch.OBS: ViewRequirement(space=self.preprocessors[identifier].observation_space),
					SampleBatch.NEXT_OBS: ViewRequirement(
						data_col=SampleBatch.OBS,
						shift=1,
						space=self.preprocessors[identifier].observation_space),
					SampleBatch.ACTIONS:ViewRequirement(
						space=agent.imitator_action_space(), used_for_compute_actions=False),
					SampleBatch.PREV_ACTIONS: ViewRequirement(
						data_col=SampleBatch.ACTIONS,
						shift=-1,
						space=agent.imitator_action_space()),
					SampleBatch.REWARDS: ViewRequirement(),
					SampleBatch.PREV_REWARDS: ViewRequirement(
						data_col=SampleBatch.REWARDS, shift=-1),
					SampleBatch.DONES: ViewRequirement(),
					SampleBatch.EPS_ID: ViewRequirement(),
					SampleBatch.UNROLL_ID: ViewRequirement(),
					SampleBatch.AGENT_INDEX: ViewRequirement(),
					SampleBatch.ACTION_PROB: ViewRequirement(),
					SampleBatch.ACTION_LOGP: ViewRequirement(),
					"t": ViewRequirement(),
				}
			if ray.__version__>="1.8.0":
				#1.8.0において_AgentCollectorの__init__にもPolicyを与える必要が生じたため、ダミーを与える
				class Dummy:
					def is_recurrent(self):
						return False
				self.agentCollectors[identifier]=_AgentCollector(self.view_reqs[identifier],Dummy())
			else:
				self.agentCollectors[identifier]=_AgentCollector(self.view_reqs[identifier])
			self.agentCollectors[identifier].add_init_obs(
				self.episodeCounter,
				agent_index,
				self.manager.vector_index,
				-1,
				self.preprocessors[identifier].transform(agent.imitatorObs)
			)
			self.dones[identifier]=False
			agent_index+=1
	def onStepEnd(self):
		agent_index=0
		for agent in self.manager.experts.values():
			identifier=agent.trajectoryIdentifier
			if(not self.dones[identifier]):
				self.dones[identifier]=self.manager.dones[agent.getName()+":"+agent.expertModelName+":"+agent.expertPolicyName]
				self.agentCollectors[identifier].add_action_reward_next_obs(
					{
						"t":self.manager.getTickCount()//self.manager.getAgentInterval(),
						"env_id":self.manager.vector_index,
						SampleBatch.AGENT_INDEX:agent_index,
						SampleBatch.ACTIONS:agent.imitatorAction,
						SampleBatch.REWARDS:self.manager.rewards[agent.getName()+":"+agent.expertModelName+":"+agent.expertPolicyName],
						SampleBatch.DONES:self.dones[identifier],
						SampleBatch.NEXT_OBS:self.preprocessors[identifier].transform(agent.imitatorObs),
						SampleBatch.ACTION_PROB:1.0,
						SampleBatch.ACTION_LOGP:0.0
					}
				)
			agent_index+=1
	def onEpisodeEnd(self):
		for identifier in self.writers:
			self.writers[identifier].write(self.agentCollectors[identifier].build(self.view_reqs[identifier]))