#-*-coding:utf-8
import sys
import time
import json
import ray
from ray.rllib.env.env_context import EnvContext

@ray.remote
def worker(config,worker_index):
	from ASRCAISim1.addons.rayUtility.RayManager import RayManager
	import OriginalModelSample #Factoryへの登録のためにこのファイルで直接使用せずとも必須
	env=RayManager(EnvContext(
		{"config":config["env_config"]},
		worker_index,
		0
	))
	env.seed(config["seed"]+worker_index)
	for i in range(config["num_episodes_per_worker"]):
		startT=time.time()
		ob=env.reset()
		action_space=env.get_action_space()
		action=action_space.sample()
		dones={"__all__":False}
		while not dones["__all__"]:
			ob, reward, dones, info = env.step(action)
		endT=time.time()
		print("Episode(",worker_index,",",i+1,") ended in ",endT-startT," seconds.")
	return True
class ExpertTrajectoryGatherer:
	def __init__(self,config):
		self.config=json.load(open(config,'r'))
		self.config["env_config"].append({
			"Manager":{"Loggers":{
				"ExpertTrajectoryWriter":{
					"class":"ExpertTrajectoryWriter",
					"config":{
						"prefix":self.config["save_dir"]+("" if self.config["save_dir"][-1]=="/" else "/")
					}
				}
			}}
		})
	def run(self):
		if(not ray.is_initialized()):
			ray.init()
		import signal
		original=signal.getsignal(signal.SIGINT)
		res=[worker.remote(self.config,i) for i in range(self.config["num_workers"])]
		def sig_handler(sig,frame):
			for r in res:
				ray.kill(r)
			signal.signal(signal.SIGINT,original)
		signal.signal(signal.SIGINT,sig_handler)
		ray.get(res)
		return res
		

if __name__ == "__main__":
	if(len(sys.argv)>1):
		gatherer=ExpertTrajectoryGatherer(sys.argv[1])
		gatherer.run()
