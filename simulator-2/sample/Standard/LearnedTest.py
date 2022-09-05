#-*-coding:utf-8-*-
import os
import sys
import json
import time
import numpy as np
import weakref
from ASRCAISim1.addons.rayUtility.RayManager import RayManager,getDefaultRayPolicyMapper,RaySimpleEvaluator
from ASRCAISim1.addons.rayUtility.extension.policy import DummyInternalRayPolicy,StandaloneRayPolicy
import OriginalModelSample #Factoryへの登録のためにこのファイルで直接使用せずとも必須
import ray
from ray.rllib.env.env_context import EnvContext
from ray.rllib.agents.trainer import with_common_config

from ray.rllib.agents.impala.vtrace_torch_policy import VTraceTorchPolicy
from ray.rllib.agents.impala.vtrace_tf_policy import VTraceTFPolicy

def main(base_config_j,weight_path):
	base_config=json.load(open(base_config_j,'r'))
	ray.init()
	configs=["../config/BVR2v2_rand.json","../config/Learned2v2.json"]
	context=EnvContext(
		{
			"config":configs+[{"Manager":{
					"ViewerType":"None",
					"seed":123456,
                	"Loggers":{
                    	"MultiEpisodeLogger":{
                        	"class":"MultiEpisodeLogger",
                        	"config":{
                            	"prefix":"./results/LearnedTest",
                            	"episodeInterval":1,
                            	"ratingDenominator":100
	                        }
                    	}
	                }
				}}]
		},
		0,
		0
	)
	env=RayManager(context)
	def get_space(env,policyNames):
		ret={p:[None,None] for p in policyNames}
		ob=env.get_observation_space()
		ac=env.get_action_space()
		for key in ac:
			agentName,modelName,policyName=key.split(":")
			if(policyName in policyNames and ret[policyName][0] is None):
				ret[policyName][0]=ob[key]
				ret[policyName][1]=ac[key]
		return ret
	spaces=get_space(env,["Learned"])
	tc=with_common_config(base_config["trainer_config"])
	policyClass=VTraceTorchPolicy if tc.get("framework")=="torch" else VTraceTFPolicy
	
	policyConfig={
		"Learned":{
			"trainer_config":tc,
			"policy_class":policyClass,
			"policy_spec_config":{},
			"weight":weight_path,
			"instanciate":True
		}
	}
	policies={k:StandaloneRayPolicy(k,v) for k,v in policyConfig.items() if v["instanciate"]}
	policyMapper=getDefaultRayPolicyMapper()
	evaluator=RaySimpleEvaluator(context,policies,policyMapper)
	numEpisodes=1
	for episodeCount in range(numEpisodes):
		startT=time.time()
		evaluator.run()
		endT=time.time()
		print("episode(",episodeCount+1,"/",numEpisodes,"), running time=",endT-startT,"s, env time=",evaluator.env.manager.getTime(),"s, avg. fps=",(evaluator.env.manager.getTime()/(endT-startT)))
	

if __name__ == "__main__":
	if(len(sys.argv)>2):
		main(sys.argv[1],sys.argv[2])
