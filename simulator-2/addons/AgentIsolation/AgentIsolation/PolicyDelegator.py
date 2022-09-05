#-*-coding:utf-8-*-
from ASRCAISim1.policy.StandalonePolicy import StandalonePolicy

class PolicyDelegator(StandalonePolicy):
	"""シミュレーションのメイン環境側で動く方。
	"""
	def __init__(self,policyName,server,port):
		self.policyName=policyName
		self.socketServer=server
		self.socketPort=port
	def reset(self):
		self.post("reset",None)
	def step(self,observation,reward,done,info,agentFullName,observation_space,action_space):
		data={
			"observation":observation,
			"reward":reward,
			"done":done,
			"info":info,
			"agentFullName":agentFullName,
			"observation_space":observation_space,
			"action_space":action_space
		}
		return self.post("step",data)
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
		msg=pickle.dumps([self.policyName,command,data])
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