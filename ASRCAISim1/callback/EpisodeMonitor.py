#-*-coding:utf-8-*-
from math import *
import sys,os,time
import numpy as np
import datetime
from ASRCAISim1.libCore import *

class EpisodeMonitor(Callback):
	"""エピソード終了時に得点と実行時間を出力するだけのデバッグ向けクラス。
	"""
	def __init__(self,modelConfig,instanceConfig):
		super().__init__(modelConfig,instanceConfig)
		self.episodeCounter=0
	def onEpisodeBegin(self):
		self.episodeCounter+=1
		self.startT=time.time()
	def onStepEnd(self):
		#if(self.manager.tick%100==0):
		#	print("t=",self.manager.t)
		pass
	def onEpisodeEnd(self):
		endT=time.time()
		#print("running time=",endT-startT,",avg. fps=",(self.manager.t/(endT-self.startT)))
		print("Episode",self.episodeCounter," finished at t=",self.manager.t,",","with  score=", self.manager.ruler.score," in ",endT-self.startT," seconds.")

