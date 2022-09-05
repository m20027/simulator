#-*-coding:utf-8-*-
"""シミュレータ本体の動作確認用サンプル(2)。
　初期行動判断モデル(ルールベース)どうしの戦闘を10回、可視化なしで実行し、各戦闘結果を一つのcsvファイルに出力する。
　(1)に比べ、機体のモデルがより簡略化されたものとなる。
"""
import time
from ASRCAISim1.GymManager import SimpleEvaluator,getDefaultPolicyMapper
import OriginalModelSample #Factoryへの登録のためにこのファイルで直接使用せずとも必須

def main():
	configs=[
		"../config/BVR2v2_rand_MP.json", #基本的な戦闘場面(ルールや機体の配置等)を設定したファイル
		"../config/Initial2v2.json", #報酬やAgentの割当等、行動判断とその学習に関する設定を記述したファイル
		"./config_MP.json" #簡略化された機体モデルを記述したファイル
	]
	context={
		"config":configs+[{"Manager":{
			"ViewerType":"None",
			"seed":12345,
            "Loggers":{
                "MultiEpisodeLogger":{
                    "class":"MultiEpisodeLogger",
                    "config":{
                        "prefix":"./results/FirstSample_MP",
                        "episodeInterval":1,
                        "ratingDenominator":100
	                }
                }
	        }
		}}],
		"worker_index":0,
		"vector_index":0
	}
	policies={} #actionの計算が必要なもののみStandalonePolicyを与える必要がある
	policyMapper=getDefaultPolicyMapper()
	evaluator=SimpleEvaluator(context,policies,policyMapper)
	
	numEpisodes=10
	for episodeCount in range(numEpisodes):
		startT=time.time()
		evaluator.run()
		endT=time.time()
		print("episode(",episodeCount+1,"/",numEpisodes,"), running time=",endT-startT,"s, env time=",evaluator.env.manager.getTime(),"s, avg. fps=",(evaluator.env.manager.getTime()/(endT-startT)))

if __name__ == "__main__":
    main()
