#-*-coding:utf-8-*-
from math import *
import pygame
import sys,os,time
import numpy as np
import datetime
import cv2
from OpenGL.GL import *
from OpenGL.GLU import *
from OpenGL.GLUT import *
from ASRCAISim1.libCore import *

class GodViewLogger(Callback):
	"""画像として戦域保存の実装例。
	同じく可視化サンプルのGodViewで描画されたサーフェスを連番画像ファイルに出力することで行う。
	"""
	def __init__(self,modelConfig,instanceConfig):
		super().__init__(modelConfig,instanceConfig)
		self.prefix=getValueFromJsonKRD(self.modelConfig,"prefix",self.randomGen,"./GVL")
		self.episodeInterval=getValueFromJsonKRD(self.modelConfig,"episodeInterval",self.randomGen,1)
		self.innerInterval=getValueFromJsonKRD(self.modelConfig,"innerInterval",self.randomGen,1)
		self.episodeCounter=0
		self.innerCounter=0
		self.file=None
	def onEpisodeBegin(self):
		self.innerCounter=0
		if(self.episodeCounter%self.episodeInterval==0):
			try:
				path=self.prefix+"_e{:04d}_t{:06.1f}.png".format(self.episodeCounter,self.manager.getTime())
				os.makedirs(os.path.dirname(path),exist_ok=True)
				viewer=self.manager.getViewer()
				width=viewer.width
				height=viewer.height
				img=np.zeros((height,width,3),dtype=np.uint8)
				glReadPixels(0,0,width,height,GL_BGR,GL_UNSIGNED_BYTE,img.data)
				img=np.ascontiguousarray(img[::-1])
				cv2.imwrite(path,img)
				#pygame.image.save(self.manager.getViewer().window,path)
			except Exception as e:
				print("failed to save an image: ",self.name)
				raise e
	def onInnerStepEnd(self):
		self.innerCounter+=1
		if(self.episodeCounter%self.episodeInterval==0):
			if(self.innerCounter%self.innerInterval==0):
				try:
					path=self.prefix+"_e{:04d}_t{:06.1f}.png".format(self.episodeCounter,self.manager.getTime())
					os.makedirs(os.path.dirname(path),exist_ok=True)
					viewer=self.manager.getViewer()
					width=viewer.width
					height=viewer.height
					img=np.zeros((height,width,3),dtype=np.uint8)
					glReadPixels(0,0,width,height,GL_BGR,GL_UNSIGNED_BYTE,img.data)
					img=np.ascontiguousarray(img[::-1])
					cv2.imwrite(path,img)
					#pygame.image.save(self.manager.getViewer().window,path)
				except Exception as e:
					print("failed to save an image: ",self.name)
					raise e
	def onEpisodeEnd(self):
		self.episodeCounter+=1
