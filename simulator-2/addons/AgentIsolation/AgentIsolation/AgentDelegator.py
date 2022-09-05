#-*-coding:utf-8-*-
from math import *
from gym import spaces
import numpy as np
import sys
from ASRCAISim1.libCore import *
from .libAgentIsolation import AgentDelegatorBase

class AgentDelegator(AgentDelegatorBase):
	"""シミュレーションのメイン環境側で動く方。
	"""
	def __init__(self,modelConfig,instanceConfig):
		super().__init__(modelConfig,instanceConfig)
		if(self.isDummy):
			return
		self.socketServer=self.modelConfig.at("socketServer")()
		self.socketPort=self.modelConfig.at("socketPort")()
		self.initialize()
	def post(self,command,data):
		"""コマンドとデータを送って、返事を貰う。

			Args:
				command (str): コマンドの種類を表す文字列。
				data (object): Pickle化可能な任意のデータオブジェクト。
		"""
		import socket
		import pickle
		bufferSize=4096
		conn=socket.socket(socket.AF_INET,socket.SOCK_STREAM)
		conn.connect((self.socketServer,self.socketPort))
		msg=pickle.dumps([self.getFullName(),command,data])
		header="HEADER:{:16d}".format(len(msg)).encode("utf-8")
		conn.send(header)
		ack=conn.recv(bufferSize).decode("utf-8")
		assert(ack[4:]=="OK")
		conn.send(msg)
		header=conn.recv(bufferSize).decode("utf-8")
		msgLen=int(header[7:])
		conn.send("ACK:OK".encode("utf-8"))
		received=0
		ret=b""
		while received<msgLen:
			part=conn.recv(bufferSize)
			received+=len(part)
			if(len(part)>0):
				ret+=part
		assert(received==msgLen)
		conn.close()
		return pickle.loads(ret)
	def initialize(self):
		#隔離環境側の初期化
		agentState=self.post("initialize",self.makeSimulationState())
		self.parseAgentState(agentState)
	#ユーザー定義関数
	def action_space(self):
		#行動空間の定義(必須)
		agentState,space=self.post("action_space",self.makeSimulationState())
		self.parseAgentState(agentState)
		return space #gym.Spaceなので直接のjson化はできない。pickle化するか別の書式に変換してやり取りする
	def observation_space(self):
		#観測空間の定義(必須)
		agentState,space=self.post("observation_space",self.makeSimulationState())
		self.parseAgentState(agentState)
		return space #gym.Spaceなので直接のjson化はできない。pickle化するか別の書式に変換してやり取りする
	def makeObs(self):
		#Observationの生成(必須)
		agentState,observation=self.post("makeObs",self.makeSimulationState())
		self.parseAgentState(agentState)
		return observation #基本的にはnp.ndarrayなので直接のjson化はできない。pickle化するか別の書式に変換してやり取りする
	def deploy(self,action):
		#Actionの解釈とdecisions and/or commandsの生成 (1stepに1回実行)(必須)
		simState=self.makeSimulationState()
		#data["action"]=action #基本的にはnp.ndarrayなので直接のjson化はできない。pickle化するか別の書式に変換してやり取りする
		agentState=self.post("deploy",(simState,action))
		self.parseAgentState(agentState)
	def validate(self):
		#コンストラクタ外の初期化処理(observablesに依存するものを記述する)
		agentState=self.post("validate",self.makeSimulationState())
		self.parseAgentState(agentState)
	def perceive(self,inReset):
		#1tick単位の処理(perceive)を記述する。decisions and/or commandsの複雑な生成処理を行う場合等に用いる。
		simState=self.makeSimulationState()
		agentState=self.post("perceive",(simState,inReset))
		self.parseAgentState(agentState)
	def control(self):
		#1tick単位の処理(control)を記述する。decisions and/or commandsの複雑な生成処理を行う場合等に用いる。
		agentState=self.post("control",self.makeSimulationState())
		self.parseAgentState(agentState)
	def behave(self):
		#1tick単位の処理(behave)を記述する。decisions and/or commandsの複雑な生成処理を行う場合等に用いる。
		agentState=self.post("behave",self.makeSimulationState())
		self.parseAgentState(agentState)