#-*-coding:utf-8-*-
import os,json
import ASRCAISim1
from ASRCAISim1.libCore import Factory
from ASRCAISim1.common import addPythonClass
from ASRCAISim1.policy import StandalonePolicy

#①Agentクラスオブジェクトを返す関数を定義
"""
以下はサンプルのAgentクラスを借りてくる場合の例
"""
def getUserAgentClass():
    from OriginalModelSample import R3PyAgentSample02
    return R3PyAgentSample02

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
	return False

#④StandalonePolicyを返す関数を定義
"""
以下はランダム行動のポリシーとする例
"""
class DummyPolicy(StandalonePolicy):
	"""指定されたaction_spaceに合う形のactionをランダムにサンプリングするダミーポリシー
	"""
	def step(self,observation,reward,done,info,agentFullName,observation_space,action_space):
		return action_space.sample()

def getUserPolicy():
    return DummyPolicy()
