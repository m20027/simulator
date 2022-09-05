#-*-coding:utf-8-*-
from math import *
import sys
import numpy as np
from ASRCAISim1.libCore import *
from OriginalModelSample.libOriginalModelSample import *

class R3PyRewardSample01(TeamReward):
	"""いくつかの観点に基づいた報酬の実装例。
	(1)Bite(誘導弾シーカで目標を捕捉)への加点
	(2)誘導弾目標のメモリトラック落ちへの減点
	(3)敵探知への加点(生存中の敵の何%を探知できているか)
	(4)過剰な機動への減点
	(5)前進・後退への更なる加減点
    (6)保持している力学的エネルギー(回転を除く)の増減による加減点
	"""
	def __init__(self,modelConfig,instanceConfig):
		super().__init__(modelConfig,instanceConfig)
		if(self.isDummy):
			return
		self.pBite=getValueFromJsonKRD(self.modelConfig,"pBite",self.randomGen,2.0)
		self.pMemT=getValueFromJsonKRD(self.modelConfig,"pMemT",self.randomGen,0.2)
		self.pDetect=getValueFromJsonKRD(self.modelConfig,"pDetect",self.randomGen,0.001)
		self.pVel=getValueFromJsonKRD(self.modelConfig,"pVel",self.randomGen,0.0)
		self.pOmega=getValueFromJsonKRD(self.modelConfig,"pOmega",self.randomGen,0.0)
		self.pLine=getValueFromJsonKRD(self.modelConfig,"pLine",self.randomGen,0.0)
		self.pEnergy=getValueFromJsonKRD(self.modelConfig,"pEnergy",self.randomGen,2e-5)
	def onEpisodeBegin(self):#初期化
		self.j_target="All"#個別のconfigによらず強制的に対象を指定する
		super().onEpisodeBegin()
		o=self.manager.getRuler().observables
		self.westSider=o["westSider"]()
		self.eastSider=o["eastSider"]()
		self.forwardAx=o["forwardAx"]()
		self.friends={
			team:[
				f for f in self.manager.getAssets(lambda a:a.getTeam()==team and isinstance(a,Fighter))
			]
			for team in self.reward
		}
		self.totalEnergy={
			team:sum([
				np.linalg.norm(f.velI())**2/2-gravity*f.posI()[2] for f in self.manager.getAssets(lambda a:a.getTeam()==team and isinstance(a,Fighter))
			])
			for team in self.reward
		}
		self.enemies={
			team:[
				f for f in self.manager.getAssets(lambda a:a.getTeam()!=team and isinstance(a,Fighter))
			]
			for team in self.reward
		}
		self.friendMsls={
			team:[
				f for f in self.manager.getAssets(lambda a:a.getTeam()==team and isinstance(a,Missile))
			]
			for team in self.reward
		}
		self.numMissiles={team:len(self.friendMsls[team]) for team in self.reward}
		self.biteFlag={team:np.full(self.numMissiles[team],False)
			for team in self.reward}
		self.memoryTrackFlag={team:np.full(self.numMissiles[team],False)
			for team in self.reward}
	def onStepEnd(self):
		delta={t:0.0 for t in self.reward}
		for team in self.reward:
			#(1)Biteへの加点、(2)メモリトラック落ちへの減点
			for i,m in enumerate(self.friendMsls[team]):
				if(m.hasLaunched and m.isAlive):
					if(m.mode==Missile.Mode.SELF and not self.biteFlag[team][i]):
						self.reward[team]+=self.pBite
						delta[team]+=self.pBite
						self.biteFlag[team][i]=True
					if(m.mode==Missile.Mode.MEMORY and not self.memoryTrackFlag[team][i]):
						self.reward[team]-=self.pMemT
						delta[team]-=self.pMemT
						self.memoryTrackFlag[team][i]=True
			#(3)敵探知への加点(生存中の敵の何%を探知できているか)(データリンク前提)
			track=[]
			for f in self.friends[team]:
				if(f.isAlive()):
					track=[Track3D(t) for t in f.observables["sensor"]["track"]]
					break
			numAlive=0
			numTracked=0
			for f in self.enemies[team]:
				if(f.isAlive()):
					numAlive+=1
					for t in track:
						if(t.isSame(f)):
							numTracked+=1
							break
			if(numAlive>0):
				self.reward[team]+=(1.0*numTracked/numAlive)*self.pDetect
				delta[team]+=(1.0*numTracked/numAlive)*self.pDetect
			ene=0.0
			for f in self.friends[team]:
				pos=f.posI()
				vel=f.velI()
				omega=f.omegaI()
				#(4)過剰な機動への減点(角速度ノルムに対してL2、高度方向速度に対してL1正則化)
				self.reward[team]+=-self.pVel*abs(vel[2])-(np.linalg.norm(omega)**2)*self.pOmega
				#(5)前進・後退への更なる加減点
				self.reward[team]+=self.pLine if np.dot(self.forwardAx[team],vel[0:2])>=0 else -self.pLine
				#(6)保持している力学的エネルギー(回転を除く)の増減による加減点
				ene+=np.linalg.norm(vel)**2/2-gravity*pos[2]
			self.reward[team]+=(ene-self.totalEnergy[team])*self.pEnergy
			delta[team]+=(ene-self.totalEnergy[team])*self.pEnergy
			self.totalEnergy[team]=ene
		super().onStepEnd()
