#-*-coding:utf-8-*-
import os,json
import ASRCAISim1
from ASRCAISim1.libCore import Factory
from ASRCAISim1.common import addPythonClass
from ASRCAISim1.addons.rayUtility.extension.policy import StandaloneRayPolicy
#①Agentクラスオブジェクトを返す関数を定義
"""
以下はサンプルのAgentクラスを借りてくる場合の例
"""
def getUserAgentClass():
    from OriginalModelSample import R3PyAgentSample01
    return R3PyAgentSample01

#②Agentモデル登録用にmodelConfigを表すjsonを返す関数を定義
"""
なお、modelConfigとは、Agentクラスのコンストラクタに与えられる二つのjson(dict)のうちの一つであり、設定ファイルにおいて
{
	"Factory":{
		"Agent":{
			"modelName":{
				"class":"className",
				"config":{...}
			}
		}
	}
}の"config"の部分に記載される{...}のdictをが該当する。
"""	
def getUserAgentModelConfig():
    return json.load(open(os.path.join(os.path.dirname(__file__),"config.json"),"r"))

#③Agentの種類(一つのAgentインスタンスで1機を操作するのか、2機両方を操作するのか)を返す関数を定義
"""AgentがAssetとの紐付けに使用するportの名称は本来任意であるが、
　簡単のために1機を操作する場合は"0"、2機を操作する場合は"0"と"1"で固定とする。
"""
def isUserAgentSingleAsset():
	#1機だけならばTrue,2機両方ならばFalseを返すこと。
	return True

#④StandalonePolicyを返す関数を定義
def getUserPolicy():
	from ray.rllib.agents.trainer import with_common_config
	from ray.rllib.agents.impala.vtrace_torch_policy import VTraceTorchPolicy
	import gym
	tc=with_common_config(json.load(open(os.path.join(os.path.dirname(__file__),"trainer_config.json"),"r")))
	policyClass=VTraceTorchPolicy
	policyConfig={
		"trainer_config":tc,
		"policy_class":policyClass,
		"policy_spec_config":{},
		"weight":os.path.join(os.path.dirname(__file__),"weights.dat")
	}
	return StandaloneRayPolicy("my_policy",policyConfig,False,False)
