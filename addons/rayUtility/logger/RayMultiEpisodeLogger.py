#-*-coding:utf-8-*-
from math import *
import sys,os,time
import numpy as np
import datetime
from ASRCAISim1.libCore import *
import ray

@ray.remote
class Gatherer:
	def __init__(self,config):
		self.prefix=config["prefix"]
		self.episodeInterval=config["episodeInterval"]
		self.ratingDenominator=config["ratingDenominator"]
		self.episodeCounter=0
		self.totalSteps=0
		self.winCount=0
		self.recentWinLoses=[]
		self.file=None
	def onEpisodeBegin(self,info):
		pass
	def onEpisodeEnd(self,info):
		if(self.file is None):
			path=self.prefix+"_"+datetime.datetime.now().strftime("%Y%m%d%H%M%S")+".csv"
			os.makedirs(os.path.dirname(path),exist_ok=True)
			self.file=open(path,'w')
			self.makeHeader(info)
		self.episodeCounter+=1
		self.totalSteps+=info["numSteps"]
		if(info["winLose"]):
			self.winCount+=1
		if(self.episodeCounter<=self.ratingDenominator):
			self.recentWinLoses.append(1.0 if info["winLose"] else 0.0)
		else:
			self.recentWinLoses[(self.episodeCounter-1)%self.ratingDenominator]=1.0 if info["winLose"] else 0.0
		if(self.episodeCounter%self.episodeInterval==0):
			self.makeFrame(info)
		print("Episodes=(",self.winCount,"/",self.episodeCounter,"), recent winning rate=",100.0*sum(self.recentWinLoses)/len(self.recentWinLoses),", endReason=",info["endReason"],"finished at t=",info["finishedTime"],",","with  score=", info["scores"]," and totalRewards=",info["totalRewards"]," in ",info["calcTime"]," seconds.")
	def makeHeader(self,info):
		row=["Episode"]
		scores=info["scores"]
		row.extend(["score["+team+"]" for team in scores.keys()])
		totalRewards=info["totalRewards"]
		row.extend(["totalReward["+agent+"]" for agent in totalRewards.keys()])
		row.extend(["finishedTime[s]","numSteps","calcTime[s]","BlueWin","WinCount","WinRate[%]"])
		numAlives=info["numAlives"]
		row.extend(["numAlives["+team+"]" for team in numAlives.keys()])
		row.extend(["endReason"])
		self.file.write(','.join(row)+"\n")
		self.file.flush()
	def makeFrame(self,info):
		row=[str(self.episodeCounter)]
		scores=info["scores"]
		row.extend([format(s,'+0.16e') for s in scores.values()])
		totalRewards=info["totalRewards"]
		row.extend([format(t,'+0.16e') for t in totalRewards.values()])
		row.extend([
			format(info["finishedTime"],'+0.16e'),
			str(info["numSteps"]),
			format(info["calcTime"],'+0.16e'),
			"1" if info["winLose"] else "0"])
		row.append(str(self.winCount))
		row.append(format(100.0*sum(self.recentWinLoses)/len(self.recentWinLoses),'+0.16e'))
		numAlives=info["numAlives"]
		row.extend([format(s,'+0.16e') for s in numAlives.values()])
		row.extend([info["endReason"]])
		self.file.write(','.join(row)+"\n")
		self.file.flush()
class RayMultiEpisodeLogger(Callback):
	"""???????????????????????????????????????????????????????????????(ray??????????????????)

	??????????????????????????????????????????????????????????????????
	"""
	def __init__(self,modelConfig,instanceConfig):
		super().__init__(modelConfig,instanceConfig)
		if(self.isDummy):
			return
		self.prefix=getValueFromJsonKRD(self.modelConfig,"prefix",self.randomGen,"")
		self.episodeInterval=getValueFromJsonKRD(self.modelConfig,"episodeInterval",self.randomGen,1)
		self.ratingDenominator=getValueFromJsonKRD(self.modelConfig,"ratingDenominator",self.randomGen,100)
		try:
			self.gatherer=Gatherer.options(name=self.getName()).remote(self.modelConfig())
		except ValueError:
			self.gatherer=ray.get_actor(self.getName())
		
		self.episodeCounter=0
		self.winLose=False
		self.finishedTime=0.0
		self.startTime=0.0
	def onEpisodeBegin(self):
		self.startT=time.time()
		ruler=self.manager.getRuler()
		self.westSider=ruler.westSider
		self.eastSider=ruler.eastSider
		self.gatherer.onEpisodeBegin.remote({
			"winLose":self.winLose,
			"finishedTime":self.finishedTime,
			"numSteps":self.manager.getTickCount()/self.manager.getAgentInterval(),
			"calcTime":0.0,
			"scores":ruler.score,
			"totalRewards":{agent.getName():self.manager.totalRewards[agent.getFullName()] for agent in self.manager.getAgents()},
			"numAlives":{team:np.sum([f.isAlive()
				for f in self.manager.getAssets(lambda a:isinstance(a,Fighter) and a.getTeam()==team)
			]) for team in ruler.score},
			"endReason":ruler.endReason.name
		})
	def onEpisodeEnd(self):
		ruler=self.manager.getRuler()
		self.episodeCounter+=1
		self.winLose=(ruler.winner==self.eastSider)
		self.finishedTime=self.manager.getTime()
		endT=time.time()
		self.gatherer.onEpisodeEnd.remote({
			"winLose":self.winLose,
			"finishedTime":self.finishedTime,
			"numSteps":self.manager.getTickCount()/self.manager.getAgentInterval(),
			"calcTime":endT-self.startT,
			"scores":ruler.score,
			"totalRewards":{agent.getName():self.manager.totalRewards[agent.getFullName()] for agent in self.manager.getAgents()},
			"numAlives":{team:np.sum([f.isAlive()
				for f in self.manager.getAssets(lambda a:isinstance(a,Fighter) and a.getTeam()==team)
			]) for team in ruler.score},
			"endReason":ruler.endReason.name
		})
