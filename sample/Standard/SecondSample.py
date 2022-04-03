#-*-coding:utf-8-*-
"""OpenAI gym環境としてのインターフェース確認用サンプル。
　サンプルのAgentモデル2種(Blue側は1機ずつ行動、Red側は2機分一纏めに行動)どうしのランダム行動による戦闘を1回行う。
　なお、このサンプルはコンフィグを変えた後等のAgent,Policy割当の確認の際にも用いることができる。
"""
import time
from ASRCAISim1.GymManager import GymManager,getDefaultPolicyMapper
from ASRCAISim1.policy.StandalonePolicy import StandalonePolicy
import OriginalModelSample #Factoryへの登録のためにこのファイルで直接使用せずとも必須

class DummyPolicy(StandalonePolicy):
	"""指定されたaction_spaceに合う形のactionをランダムにサンプリングするダミーポリシー
	"""
	def step(self,observation,reward,done,info,agentFullName,observation_space,action_space):
		return action_space.sample()

def main():
	#環境の生成
	configs=[
		"../config/BVR2v2_rand.json", #基本的な戦闘場面(ルールや機体の配置等)を設定したファイル
		"../config/ForSecondSample.json" #報酬やAgentの割当等、行動判断とその学習に関する設定を記述したファイル
	]
	context={
		"config":configs+[{"Manager":{
			"ViewerType":"None",
			"seed":123456,
            "Loggers":{
	        }
		}}],
		"worker_index":0,
		"vector_index":0
	}
	env=GymManager(context)
	
	#Agent→Policyの割当
	policyMapper=getDefaultPolicyMapper()

	#Policyの生成
	policies={}
	action_space=env.action_space
	for fullName in action_space:
		policyName=policyMapper(fullName)
		if(not policyName in policies):
			policies[policyName]=DummyPolicy()
	
	#生成状況の確認
	print("=====Policies=====")
	for name,policy in policies.items():
		print(name," = ",type(policy))
	print("=====Policy Map (at reset)=====")
	for fullName in action_space:
		print(fullName," -> ",policyMapper(fullName))
	print("=====Agent to Asset map=====")
	for agent in env.manager.getAgents():
		print(agent.getFullName(), " -> ","{")
		for port,parent in agent.parents.items():
			print("  ",port," : ",parent.getFullName())
		print("}")
	#シミュレーションの実行
	print("=====running simulation(s)=====")
	numEpisodes=1
	for episodeCount in range(numEpisodes):
		startT=time.time()
		obs=env.reset()
		rewards={k:0.0 for k in obs.keys()}
		dones={k:False for k in obs.keys()}
		infos={k:None for k in obs.keys()}
		for p in policies.values():
			p.reset()
		dones["__all__"]=False
		while not dones["__all__"]:
			observation_space=env.get_observation_space()
			action_space=env.get_action_space()
			actions={k:policies[policyMapper(k)].step(
				o,
				rewards[k],
				dones[k],
				infos[k],
				k,
				observation_space[k],
				action_space[k]
			) for k,o in obs.items() if policyMapper(k) in policies}
			obs, rewards, dones, infos = env.step(actions)
		endT=time.time()
		print("episode(",episodeCount+1,"/",numEpisodes,"), running time=",endT-startT,"s, env time=",env.manager.getTime(),"s, avg. fps=",(env.manager.getTime()/(endT-startT)))

if __name__ == "__main__":
    main()
