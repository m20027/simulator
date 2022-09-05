#-*-coding:utf-8-*-
from math import *
import sys,os,time
import numpy as np
import datetime
from ASRCAISim1.libCore import *

class EpisodewiseLoggerBase(Callback):
	"""エピソード単位でログファイルを生成するためのベースクラス。
	エピソード間に跨るログファイルを生成したい場合は、ファイルの生成タイミングを工夫すれば同様に実装できる。
	"""
	def __init__(self,modelConfig,instanceConfig):
		super().__init__(modelConfig,instanceConfig)
		self.prefix=getValueFromJsonKRD(self.modelConfig,"prefix",self.randomGen,"")
		self.outerInterval=getValueFromJsonKRD(self.modelConfig,"outerInterval",self.randomGen,1)
		self.innerInterval=getValueFromJsonKRD(self.modelConfig,"innerInterval",self.randomGen,1)
		self.outerCounter=0
		self.innerCounter=0
		self.file=None
	def onEpisodeBegin(self):
		path=self.prefix+"_"+datetime.datetime.now().strftime("%Y%m%d%H%M%S")+".csv"
		os.makedirs(os.path.dirname(path),exist_ok=True)
		self.file=open(path,'w')
		self.makeHeader()
		self.makeFrameOnStep()
	def onStepEnd(self):
		if(self.outerCounter%self.outerInterval==0):
			self.makeFrameOnStep()
		self.outerCounter+=1
	def onInnerStepEnd(self):
		if(self.innerCounter%self.innerInterval==0):
			self.makeFrameOnInnerStep()
		self.innerCounter+=1
	def onEpisodeEnd(self):
		self.file.close()
		self.file=None
	def makeHeader(self):
		pass
	def makeFrameOnStep(self):
		pass
	def makeFrameOnInnerStep(self):
		pass

class BasicLogger(EpisodewiseLoggerBase):
	"""エピソード単位のログ出力の実装例。
	各陣営の戦闘機と誘導弾の基本的な情報を吐き出す。
	"""
	def __init__(self,modelConfig,instanceConfig):
		super().__init__(modelConfig,instanceConfig)
	def makeHeader(self):
		row=["Time[s]"]
		teams=self.manager.getTeams()
		for team in teams:
			row.extend([team+".remFighters",team+".remMissiles"])
		for team in teams:
			for f in self.manager.getAssets(lambda a:isinstance(a,Fighter) and a.getTeam()==team):
				row.extend([f.getFullName()+".isAlive"])
				row.extend([f.getFullName()+".pos.x[m]",f.getFullName()+".pos.y[m]",f.getFullName()+".pos.z[m]"])
				row.extend([f.getFullName()+".vel.x[m/s]",f.getFullName()+".vel.y[m/s]",f.getFullName()+".vel.z[m/s]"])
				row.extend([f.getFullName()+".att.roll[rad]",f.getFullName()+".att.pitch[rad]",f.getFullName()+".att.yaw[rad]"])
				row.extend([f.getFullName()+".omega.x[rad/s]",f.getFullName()+".omega.y[rad/s]",f.getFullName()+".omega.z[rad/s]"])
		for team in teams:
			for f in self.manager.getAssets(lambda a:isinstance(a,Missile) and a.getTeam()==team):
				row.extend([f.getFullName()+".isFlying",f.getFullName()+".target.truth"])
				row.extend([f.getFullName()+".pos.x[m]",f.getFullName()+".pos.y[m]",f.getFullName()+".pos.z[m]"])
				row.extend([f.getFullName()+".vel.x[m/s]",f.getFullName()+".vel.y[m/s]",f.getFullName()+".vel.z[m/s]"])
				row.extend([f.getFullName()+".att.roll[rad]",f.getFullName()+".att.pitch[rad]",f.getFullName()+".att.yaw[rad]"])
				row.extend([f.getFullName()+".omega.x[rad/s]",f.getFullName()+".omega.y[rad/s]",f.getFullName()+".omega.z[rad/s]"])
		self.file.write(','.join(row)+"\n")
	def makeFrameOnStep(self):
		pass
	def makeFrameOnInnerStep(self):
		row=[]
		row.append(format(self.manager.getTime(),'0.6f'))
		teams=self.manager.getTeams()
		for team in teams:
			remFighters=0
			remMissiles=0
			for f in self.manager.getAssets(lambda a:isinstance(a,Fighter) and a.getTeam()==team):
				if(f.isAlive()):
					remFighters+=1
			for f in self.manager.getAssets(lambda a:isinstance(a,Missile) and a.getTeam()==team):
				if(f.isAlive()):
					remMissiles+=1
			row.extend([str(remFighters),str(remMissiles)])
		for team in teams:
			for f in self.manager.getAssets(lambda a:isinstance(a,Fighter) and a.getTeam()==team):
				row.extend([str(f.isAlive())])
				if(f.isAlive()):
					row.extend([format(x,'+0.16e') for x in f.posI()])
					row.extend([format(x,'+0.16e') for x in f.velI()])
					row.extend([format(x,'+0.16e') for x in f.toEulerAngle()])
					row.extend([format(x,'+0.16e') for x in f.omegaI()])
				else:
					row.extend([format(x,'+0.16e') for x in np.array([0.,0.,0.])])
					row.extend([format(x,'+0.16e') for x in np.array([0.,0.,0.])])
					row.extend([format(x,'+0.16e') for x in np.array([0.,0.,0.])])
					row.extend([format(x,'+0.16e') for x in np.array([0.,0.,0.])])
		for team in teams:
			for f in self.manager.getAssets(lambda a:isinstance(a,Missile) and a.getTeam()==team):
				if(f.isAlive() and f.hasLaunched):
					tgtName="None"
					for tgt in self.manager.getAssets(lambda a:isinstance(a,Fighter) and a.getTeam()!=team):
						if(f.target.isSame(tgt)):
							tgtName=tgt.getFullName()
							break
					row.extend(["True",tgtName])
					row.extend([format(x,'+0.16e') for x in f.posI()])
					row.extend([format(x,'+0.16e') for x in f.velI()])
					roll=0.0
					ex=f.velI()/np.linalg.norm(f.velI())
					yaw=atan2(ex[1],ex[0])
					pitch=atan2(-ex[2],sqrt(ex[0]*ex[0]+ex[1]*ex[1]))
					row.extend([format(x,'+0.16e') for x in [roll,pitch,yaw]])
					row.extend([format(x,'+0.16e') for x in f.omegaI()])
				else:
					row.extend(["False","None"])
					row.extend([format(x,'+0.16e') for x in np.array([0.,0.,0.])])
					row.extend([format(x,'+0.16e') for x in np.array([0.,0.,0.])])
					row.extend([format(x,'+0.16e') for x in np.array([0.,0.,0.])])
					row.extend([format(x,'+0.16e') for x in np.array([0.,0.,0.])])
		self.file.write(','.join(row)+"\n")
