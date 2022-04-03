#-*-coding:utf-8-*-
from math import *
from gym import spaces
import numpy as np
import sys
from ASRCAISim1.libCore import *
from OriginalModelSample.libOriginalModelSample import *

class R3PyAgentSample01(SingleAssetAgent):
	"""1機につき1つのAgentを割り当てる、分散方式での行動判断モデルの実装例。
	時系列情報の活用については、RNNを使わずとも、キーフレームでの値を並列で吐き出せる形で実装している。
	もしRNNを使う場合は、キーフレームを指定せず、瞬時値をそのまま観測として出力すればよい。
	場外・墜落の回避については、学習に委ねず南北・高度方向の移動に制限をかけることで対処している。
	もし一時的に場外に出ることが勝利に繋がりうると思えば、制限をなくしてもよい。
	速度については、遅くなりすぎると機動力が低下するため、下限速度を設定できるようにしている。
	射撃については、全弾連射する等ですぐに攻撃能力を喪失するような状況を予め回避するため、同時射撃数を制限できるようにしている。
	1. 観測データについて
		* 東側でも西側でも同じになるように、西側のベクトルはx,y軸について反転させる。
		* キーフレームは「n秒前のフレーム」として、そのnのlistをconfigで"pastPoints"キーにより与える
		* 各フレームの観測データの内訳
			1. 自機情報
				1. 位置・・・x,y成分をRulerのdOutとdLineの大きい方で除し、z成分をRulerのhLimで除して正規化したもの。
				2. 速度・・・速度のノルムをfgtrVelNormalizerで正規化したものと、速度方向の単位ベクトルの4次元に分解したもの。
				3. 残弾数・・・intそのまま。（ただし一括してBoxの値として扱われる）
			2. 味方機情報
				1. 位置・・・距離をrangeNormalizerで正規化したものと、相対位置方向の単位ベクトルの4次元に分解したもの。
				2. 速度・・・自機と同じ
				3.残弾数・・・自機と同じ
			3. 敵機情報・・・見えている敵機航跡のうち、近いものから順に最大maxTrackNum機分。無い場合は0埋め。
				1. 位置・・・味方機と同じ
				2. 速度・・・味方機と同じ
			4. 味方誘導弾情報(自機も含む)
				1. 位置・・・味方機と同じ
				2. 速度・・・自機と同じ
				3. 誘導状態・・・guided,self,memoryの3通りについて、one-hot形式で与える。
			5. 敵誘導弾情報・・・見えている誘導弾航跡のうち、近いものから順に最大maxMissileNum発分。無い場合は0埋め。
				1. 到来方向・・・慣性座標系での到来方向を表した単位ベクトル。
	2. 行動の形式について
		左右旋回、上昇・下降、加減速、射撃対象の4種類を離散化したものをMultiDiscreteで与える。
		1. 左右旋回・・・自機正面を0とした「行きたい方角(右を正)」で指定。
		2. 上昇・下降・・・水平を0とし、上昇・下降角度(下降を正)で指定。
		3. 加減速・・・目標速度を基準速度(≒現在速度)+ΔVとすることで表現し、ΔVで指定。
		4. 射撃対象・・・0を射撃なし、1〜maxTrackNumを対応するlastTrackInfoのTrackへの射撃とする。
	Attributes:
		* configで指定するもの
		turnScale (double): 最大限に左右旋回する際の目標方角の値。degで指定。
		turnDim (int): 左右旋回の離散化数。0を用意するために奇数での指定を推奨。
		pitchScale (double): 最大限に上昇・下降する際の目標角度の値。degで指定。
		pitchDim (int): 上昇・下降の離散化数。0を用意するために奇数での指定を推奨。
		accelScale (double): 最大限に加減速する際のΔVの絶対値。
		accelDim (int): 加減速の離散化数。0を用意するために奇数での指定を推奨。
		hMin (double): 高度下限。下限を下回ったら下回り具合に応じて上昇方向に針路を補正する。
		hMax (double): 高度上限。上限を上回ったら上回り具合に応じて下降方向に針路を補正する。
		dOutLimitRatio (double): 南北方向への戦域逸脱を回避するための閾値。中心からdOut×dOutLimitRatio以上外れたら外れ具合に応じて中心方向に針路を補正する。
		rangeNormalizer (float): 距離の正規化のための除数
		fgtrVelNormalizer (float): 機体速度の正規化のための除数
		mslVelNormalizer (float): 誘導弾速度の正規化のための除数
		maxTrackNum (dict): 観測データとして使用する味方、敵それぞれの航跡の最大数。{"Friend":3,"Enemy":4}のようにdictで指定する。
		maxMissileNum (dict): 観測データとして使用する味方、敵それぞれの誘導弾情報の最大数。書式はmaxTrackNumと同じ。
		pastPoints (list of int): キーフレームのリスト。「nステップ前のフレーム」としてnのリストで与える。空で与えれば瞬時値のみを使用する。RNNにも使える(はず)
		pastData (list of numpy.ndarray): 過去のフレームデータを入れておくリスト。キーフレームの間隔が空いていても、等間隔でなければ全フレーム分必要なので、全フレーム部分用意し、リングバッファとして使用する。
		minimumV (double): 下限速度。この値を下回ると指定した目標速度に戻るまで強制的に加速する。
		minimumRecoveryV (Double): 速度下限からの回復を終了する速度。下限速度に達した場合、この値に達するまで強制的に加速する。
		minimumRecoveryDstV (Double): 速度下限からの回復目標速度。下限速度に達した場合、この値を目標速度として加速する。
		maxSimulShot (int): 同時射撃数の制限。自身が発射した、飛翔中の誘導弾がこの数以下のときのみ射撃可能。
	"""
	def __init__(self,modelConfig,instanceConfig):
		super().__init__(modelConfig,instanceConfig)
		if(self.isDummy):
			return
		self.turnScale=deg2rad(getValueFromJsonKRD(self.modelConfig,"turnScale",self.randomGen,90.0))
		self.turnDim=getValueFromJsonKRD(self.modelConfig,"turnDim",self.randomGen,7)
		self.pitchScale=deg2rad(getValueFromJsonKRD(self.modelConfig,"pitchScale",self.randomGen,45.0))
		self.pitchDim=getValueFromJsonKRD(self.modelConfig,"pitchDim",self.randomGen,7)
		self.accelScale=getValueFromJsonKRD(self.modelConfig,"accelScale",self.randomGen,30.0)
		self.accelDim=getValueFromJsonKRD(self.modelConfig,"accelDim",self.randomGen,7)
		self.hMin=getValueFromJsonKRD(self.modelConfig,"hMin",self.randomGen,1000.0)
		self.hMax=getValueFromJsonKRD(self.modelConfig,"hMax",self.randomGen,15000.0)
		self.dOutLimitRatio=getValueFromJsonKRD(self.modelConfig,"dOutLimitRatio",self.randomGen,0.9)
		self.rangeNormalizer=getValueFromJsonKRD(self.modelConfig,"rangeNormalizer",self.randomGen,100000.0)
		self.fgtrVelNormalizer=getValueFromJsonKRD(self.modelConfig,"fgtrVelNormalizer",self.randomGen,300.0)#正規化用
		self.mslVelNormalizer=getValueFromJsonKRD(self.modelConfig,"mslVelNormalizer",self.randomGen,2000.0)#正規化用
		self.maxTrackNum=getValueFromJsonKRD(self.modelConfig,"maxTrackNum",self.randomGen,{"Friend":0,"Enemy":2})#味方(自分以外)及び敵の最大の航跡数
		self.maxMissileNum=getValueFromJsonKRD(self.modelConfig,"maxMissileNum",self.randomGen,{"Friend":0,"Enemy":1})#考慮する誘導弾の最大数
		self.pastPoints=getValueFromJsonKRD(self.modelConfig,"pastPoints",self.randomGen,[])
		self.minimumV=getValueFromJsonKRD(self.modelConfig,"minimumV",self.randomGen,150.0)
		self.minimumRecoveryV=getValueFromJsonKRD(self.modelConfig,"minimumRecoveryV",self.randomGen,180.0)
		self.minimumRecoveryDstV=getValueFromJsonKRD(self.modelConfig,"minimumRecoveryDstV",self.randomGen,200.0)
		self.maxSimulShot=getValueFromJsonKRD(self.modelConfig,"maxSimulShot",self.randomGen,2)
		self.singleDim=8+9*self.maxTrackNum["Friend"]+8*self.maxTrackNum["Enemy"]+11*self.maxMissileNum["Friend"]+3*self.maxMissileNum["Enemy"]
		if(len(self.pastPoints)>0):
			self.pastData=[np.zeros(self.singleDim) for i in range(max(self.pastPoints))]#過去の観測情報を入れるためのリスト
		else:
			self.pastData=[]
		self.lastTrackInfo=[] #makeObsで出力したときの航跡情報。(deployと)controlで使用するために保持しておく必要がある。
		self.launchFlag=False
		self.velRecovery=False
		self.target=Track3D()
	def validate(self):
		rulerObs=self.manager.getRuler().observables()
		self.dOut=rulerObs["dOut"]
		self.dLine=rulerObs["dLine"]
		self.hLim=rulerObs["hLim"]
		self.xyInv=np.array([1,1,1]) if (self.getTeam()==rulerObs["eastSider"]) else np.array([-1,-1,1])
		if(self.parent.isinstance(CoordinatedFighter)):
			self.parent.setFlightControllerMode("fromDirAndVel")
		else:
			fSpec=self.parent.observables["spec"]["dynamics"]
			self.omegaScale=np.array([1.0/fSpec["rollMax"](),1.0/fSpec["pitchMax"](),1.0/fSpec["yawMax"]()])
		myMotion=MotionState(self.parent.observables["motion"])
		self.baseV=np.linalg.norm(myMotion.vel)
		self.dstV=self.baseV
		self.dstDir=np.array([0,-self.xyInv[1],0])
		self.lastAction=np.array([self.turnDim//2,self.pitchDim//2,self.accelDim//2,0])
	def observation_space(self):
		#自機(3+4+1)dim、味方機(4+4+1)dim、彼機(4+4)dim、味方誘導弾(4+4+3)dim、彼誘導弾3dim
		floatLow,floatHigh=-sys.float_info.max,sys.float_info.max
		self_low=np.array([floatLow,floatLow,floatLow,floatLow,-1,-1,-1,0])
		self_high=np.array([floatHigh,floatHigh,floatHigh,floatHigh,1,1,1,floatHigh])
		friend_low=np.array([floatLow,-1,-1,-1,floatLow,-1,-1,-1,0])
		friend_high=np.array([floatHigh,1,1,1,floatHigh,1,1,1,floatHigh])
		enemy_low=np.array([floatLow,-1,-1,-1,floatLow,-1,-1,-1])
		enemy_high=np.array([floatHigh,1,1,1,floatHigh,1,1,1])
		msl_friend_low=np.array([floatLow,-1,-1,-1,floatLow,-1,-1,-1,0,0,0])
		msl_friend_high=np.array([floatHigh,1,1,1,floatHigh,1,1,1,1,1,1])
		msl_enemy_low=np.array([-1,-1,-1])
		msl_enemy_high=np.array([1,1,1])
		obs_low=np.concatenate((
			self_low,
			np.concatenate([friend_low]*self.maxTrackNum["Friend"]),
			np.concatenate([enemy_low]*self.maxTrackNum["Enemy"]),
			np.concatenate([msl_friend_low]*self.maxMissileNum["Friend"]),
			np.concatenate([msl_enemy_low]*self.maxMissileNum["Enemy"])
		))
		obs_high=np.concatenate((
			self_high,
			np.concatenate([friend_high]*self.maxTrackNum["Friend"]),
			np.concatenate([enemy_high]*self.maxTrackNum["Enemy"]),
			np.concatenate([msl_friend_high]*self.maxMissileNum["Friend"]),
			np.concatenate([msl_enemy_high]*self.maxMissileNum["Enemy"])
		))
		obs_low=np.concatenate([obs_low]*(1+len(self.pastPoints)))
		obs_high=np.concatenate([obs_high]*(1+len(self.pastPoints)))
		return spaces.Box(low=obs_low,high=obs_high,dtype=np.float32)
	def makeObs(self):
		#何回目の観測かを計算(初回を0とする)
		count=round(self.manager.getTickCount()/self.manager.getAgentInterval())
		current=self.makeSingleObs()
		if(len(self.pastPoints)==0):
			return current
		if(count==0):
			#初回は過去の仮想データを生成(誘導弾なし、敵側の情報なし)
			myPos0=self.parent.observables["motion"]["pos"]()
			myVel=self.parent.observables["motion"]["vel"]()
			friendPos0=[np.zeros([3])]*self.maxTrackNum["Friend"]()
			friendVel=[np.zeros([3])]*self.maxTrackNum["Friend"]()
			idx=0
			for n,f in self.parent.observables["shared"]["fighter"].items():
				if(n==self.parent.getFullName()):#自分を除く
					continue
				if(idx>=self.maxTrackNum["Friend"]):#既に最大機数分記録した
					break
				friendPos0[idx]=np.array(f["motion"]["pos"]())
				friendVel[idx]=np.array(f["motion"]["vel"]())
				idx+=1
			dt=self.manager.getAgentInterval()*self.manager.getBaseTimeStep()
			for t in range(max(self.pastPoints)):
				obs=np.zeros([self.singleDim],dtype=np.float32)
				ofs=0
				delta=(t+1)*dt
				#自機
				vel=myVel
				myPos=pos=myPos0-vel*delta
				V=np.linalg.norm(vel)
				obs[ofs+0:ofs+3]=pos*self.xyInv/np.array([max(self.dOut,self.dLine),max(self.dOut,self.dLine),self.hLim])
				obs[ofs+3]=V/self.fgtrVelNormalizer
				obs[ofs+4:ofs+7]=(vel/V)*self.xyInv
				obs[ofs+7]=self.parent.observables["weapon"]["remMsls"]()
				ofs+=8
				idx=0
				#味方機
				for n,f in self.parent.observables["shared"]["fighter"].items():
					if(n==self.parent.getFullName()):#自分を除く
						continue
					if(idx>=self.maxTrackNum["Friend"]):#既に最大機数分記録した
						break
					vel=friendVel[idx]
					pos=friendPos0[idx]-vel*delta
					m=MotionState(self.parent.observables["motion"])
					dr=m.relPtoH(fPos)-myPos
					dv=m.relPtoH(fVel)
					R=np.linalg.norm(dr)
					V=np.linalg.norm(dv)
					obs[ofs+0]=R/self.rangeNormalizer
					obs[ofs+1:ofs+4]=(dr/R)*self.xyInv
					obs[ofs+4]=V/self.fgtrVelNormalizer
					obs[ofs+5:ofs+8]=(dv/V)*self.xyInv
					obs[ofs+8]=f["weapon"]["remMsls"]()
					ofs+=9
					idx+=1
				self.pastData[t]=obs
		bufferSize=max(self.pastPoints)
		idx=bufferSize-1-(count%bufferSize)
		totalObs=np.concatenate((current,np.concatenate([self.pastData[(idx+i)%bufferSize] for i in self.pastPoints])))
		self.pastData[idx]=current
		return totalObs
	def makeSingleObs(self):
		"""1フレーム分の観測データを生成する。
		"""
		obs=np.zeros([self.singleDim],dtype=np.float32)
		#自機
		myMotion=MotionState(self.parent.observables["motion"])
		pos=myMotion.pos
		vel=myMotion.vel
		V=np.linalg.norm(vel)
		ofs=0
		obs[ofs+0:ofs+3]=pos*self.xyInv/np.array([max(self.dOut,self.dLine),max(self.dOut,self.dLine),self.hLim])
		obs[ofs+3]=V/self.fgtrVelNormalizer
		obs[ofs+4:ofs+7]=(vel/V)*self.xyInv
		obs[ofs+7]=self.parent.observables["weapon"]["remMsls"]()
		ofs+=8
		#味方機
		idx=0
		for n,f in self.parent.observables["shared"]["fighter"].items():
			if(n==self.parent.getFullName()):#自分を除く
				continue
			if(idx>=self.maxTrackNum["Friend"]):#既に最大機数分記録した
				break
			if(f["isAlive"]):#生存しているもののみ値を入れる
				fm=MotionState(f["motion"])
				dr=myMotion.absPtoH(fm.pos)
				dv=myMotion.relPtoH(fm.vel)
				R=np.linalg.norm(dr)
				V=np.linalg.norm(dv)
				obs[ofs+0]=R/self.rangeNormalizer
				obs[ofs+1:ofs+4]=(dr/R)*self.xyInv
				obs[ofs+4]=V/self.fgtrVelNormalizer
				obs[ofs+5:ofs+8]=(dv/V)*self.xyInv
				obs[ofs+8]=f["weapon"]["remMsls"]()
				ofs+=9
				idx+=1
		while(idx<self.maxTrackNum["Friend"]):#0埋め
			ofs+=9
			idx+=1
		#彼機(味方の誰かが探知しているものだけ諸元入り)
		#自分のtrackを近い方から順に読んで入れていく
		def distance(track):
			return np.linalg.norm(myMotion.pos-track.posI())
		self.lastTrackInfo=sorted([Track3D(t) for t in self.parent.observables["sensor"]["track"]],key=distance)
		idx=0
		for t in self.lastTrackInfo:
			if(idx>=self.maxTrackNum["Enemy"]):
				break
			dr=myMotion.absPtoH(t.posI())
			dv=myMotion.relPtoH(t.velI())
			R=np.linalg.norm(dr)
			V=np.linalg.norm(dv)
			obs[ofs+0]=R/self.rangeNormalizer
			obs[ofs+1:ofs+4]=(dr/R)*self.xyInv
			obs[ofs+4]=V/self.fgtrVelNormalizer
			obs[ofs+5:ofs+8]=(dv/V)*self.xyInv
			ofs+=8
			idx+=1
		while(idx<self.maxTrackNum["Enemy"]):#0埋め
			ofs+=8
			idx+=1
		#味方誘導弾(射撃時刻が古いものから最大N発分)
		def launchedT(m):
			return m["launchedT"]() if m["isAlive"]() and m["hasLaunched"]() else np.inf
		msls=[m for m in self.parent.observables["weapon"]["missiles"]]
		for n,f in self.parent.observables["shared"]["fighter"].items():
			if(n!=self.parent.getFullName()):
				msls.extend(f["weapon"]["missiles"])
		msls=sorted(msls,key=launchedT)
		idx=0
		for m in msls:
			if(idx>=self.maxMissileNum["Friend"] or not (m["isAlive"]() and m["hasLaunched"]())):
				break
			mm=MotionState(m["motion"])
			dr=myMotion.absPtoH(mm.pos)
			dv=myMotion.relPtoH(mm.vel)
			R=np.linalg.norm(dr)
			V=np.linalg.norm(dv)
			obs[ofs+0]=R/self.rangeNormalizer
			obs[ofs+1:ofs+4]=(dr/R)*self.xyInv
			obs[ofs+4]=V/self.mslVelNormalizer
			obs[ofs+5:ofs+8]=(dv/V)*self.xyInv
			if(m["mode"]()==Missile.Mode.GUIDED.name):
				obs[ofs+8:ofs+11]=np.array([1,0,0])
			elif(m["mode"]()==Missile.Mode.SELF.name):
				obs[ofs+8:ofs+11]=np.array([0,1,0])
			else:#if(m["mode"]()==Missile.Mode.MEMORY.name):
				obs[ofs+8:ofs+11]=np.array([0,0,1])
			ofs+=11
			idx+=1
		while(idx<self.maxMissileNum["Friend"]):#0埋め
			ofs+=11
			idx+=1
		#彼誘導弾(MWSで探知したもののうち自身の正面に近いものから最大N発)
		def angle(track):
			return -np.dot(track.dirI(),myMotion.relBtoP(np.array([1,0,0])))
		mws=sorted([Track2D(t) for t in self.parent.observables["sensor"]["mws"]["track"]],key=angle)
		idx=0
		for m in mws:
			if(idx>=self.maxMissileNum["Enemy"]):
				break
			obs[ofs+0:ofs+3]=m.dirI()*self.xyInv
			ofs+=3
			idx+=1
		while(idx<self.maxMissileNum["Enemy"]):#0埋め
			ofs+=3
			idx+=1
		return obs
	def action_space(self):
		self.turnTable=np.linspace(-self.turnScale,self.turnScale,self.turnDim)
		self.pitchTable=np.linspace(-self.pitchScale,self.pitchScale,self.pitchDim)
		self.accelTable=np.linspace(-self.accelScale,self.accelScale,self.accelDim)
		self.fireTable=list(range(-1,self.maxTrackNum["Enemy"]))
		if(self.turnDim%2!=0):self.turnTable[self.turnDim//2]=0.0#force center value strictly to be zero
		if(self.pitchDim%2!=0):self.pitchTable[self.pitchDim//2]=0.0#force center value strictly to be zero
		if(self.accelDim%2!=0):self.accelTable[self.accelDim//2]=0.0#force center value strictly to be zero
		nvec=np.array([len(self.turnTable),len(self.pitchTable),len(self.accelTable),len(self.fireTable)])
		return spaces.MultiDiscrete(nvec)
	def deploy(self,action):
		myMotion=MotionState(self.parent.observables["motion"])
		pAZ=myMotion.az
		turn=self.turnTable[action[0]]
		pitch=self.pitchTable[action[1]]
		self.dstDir=np.array([cos(pAZ+turn)*cos(pitch),sin(pAZ+turn)*cos(pitch),sin(pitch)])
		if(not(self.accelTable[self.lastAction[2]]==0.0 and self.accelTable[action[2]]==0.0)):
			self.baseV=np.linalg.norm(myMotion.vel)
		self.dstV=self.baseV+self.accelTable[action[2]]
		if(self.baseV<self.minimumV):
			self.velRecovery=True
		if(self.baseV>=self.minimumRecoveryV):
			self.velRecovery=False
		if(self.velRecovery):
			self.dstV=self.minimumRecoveryDstV
		shoot=int(action[3])-1
		flyingMsls=0
		for msl in self.parent.observables.at_p("/weapon/missiles"):
			if(msl.at("isAlive")() and msl.at("hasLaunched")()):
				flyingMsls+=1
		if(shoot>=len(self.lastTrackInfo) or flyingMsls>=self.maxSimulShot):
			shoot=-1
		if(shoot>=0):
			self.launchFlag=True
			self.target=self.lastTrackInfo[shoot]
		else:
			self.launchFlag=False
			self.target=Track3D()
		self.observables[self.parent.getFullName()]["decision"]={
			"Roll":("Don't care"),
			"Horizontal":("Az_BODY",turn),
			"Vertical":("El",-pitch),
			"Throttle":("Vel",self.dstV),
			"Fire":(self.launchFlag,self.target.to_json())
		}
		self.lastAction=action[:]
	def control(self):
		"""高度と南北方向位置について可動範囲を設定し、逸脱する場合は強制的に復元
		"""
		myMotion=MotionState(self.parent.observables["motion"])
		pos=myMotion.pos
		vel=myMotion.vel
		if(abs(pos[0])>=self.dOutLimitRatio*self.dOut):
			#戦域逸脱を避けるための方位補正
			#判定ラインの超過具合に応じて復帰角度を変化させる。(無限遠でラインに直交、ライン上でラインと平行)
			over=abs(pos[0])/self.dOut-self.dOutLimitRatio
			n=sqrt(self.dstDir[0]*self.dstDir[0]+self.dstDir[1]*self.dstDir[1])
			theta=atan(over)
			cs=cos(theta)
			sn=sin(theta)
			if(pos[0]>0):#北側
				if(self.dstDir[1]>0):#東向き
					if(atan2(-self.dstDir[0],self.dstDir[1])<theta):
						self.dstDir=np.array([-n*sn,n*cs,self.dstDir[2]])
				else:#西向き
					if(atan2(-self.dstDir[0],-self.dstDir[1])<theta):
						self.dstDir=np.array([-n*sn,-n*cs,self.dstDir[2]])
			else:#南側
				if(self.dstDir[1]>0):#東向き
					if(atan2(self.dstDir[0],self.dstDir[1])<theta):
						self.dstDir=np.array([n*sn,n*cs,self.dstDir[2]])
				else:#西向き
					if(atan2(self.dstDir[0],-self.dstDir[1])<theta):
						self.dstDir=np.array([n*sn,-n*cs,self.dstDir[2]])
		if(-pos[2]<self.hMin):
			#高度下限を下回った場合
			over=self.hMin+pos[2]
			n=sqrt(self.dstDir[0]*self.dstDir[0]+self.dstDir[1]*self.dstDir[1])
			theta=atan(over)
			cs=cos(theta)
			sn=sin(theta)
			self.dstDir=np.array([self.dstDir[0]/n*cs,self.dstDir[1]/n*cs,-sn])
		elif(-pos[2]>self.hMax):
			#高度上限を上回った場合
			over=-pos[2]-self.hMax
			n=sqrt(self.dstDir[0]*self.dstDir[0]+self.dstDir[1]*self.dstDir[1])
			theta=atan(over)
			cs=cos(theta)
			sn=sin(theta)
			self.dstDir=np.array([self.dstDir[0]/n*cs,self.dstDir[1]/n*cs,sn])
		if(self.parent.isinstance(CoordinatedFighter)):
			self.commands[self.parent.getFullName()]={
				"motion":{
					"dstDir":self.dstDir,
					"dstV":self.dstV
				},
				"weapon":{
					"launch":self.launchFlag,
					"target":self.target.to_json()
				}
			}
		elif(self.parent.isinstance(MassPointFighter)):
			#dstDirを角速度に変換
			V=np.linalg.norm(vel)
			Vn=vel/V
			cs=np.dot(Vn,self.dstDir)
			delta=np.cross(Vn,self.dstDir)
			sn=np.linalg.norm(delta)
			theta=atan2(sn,cs)
			if(theta<1e-6):#ほぼ0
				omegaI=np.array([0.,0.,0.])
			elif(theta>pi-1e-6):#ほぼ真後ろ
				pz=np.cross(Vn,np.cross(np.array([0.,0.,1.]),Vn))
				pz/=np.linalg.norm(pz)
				omegaI=pz*pi
			else:#平行でない
				omegaI=delta*(theta/sn)
			omegaB=myMotion.relPtoB(omegaI)
			self.commands[self.parent.getFullName()]={
				"motion":{
					"roll":omegaB[0]*self.omegaScale[0],
					"pitch":omegaB[1]*self.omegaScale[1],
					"yaw":omegaB[2]*self.omegaScale[2],
					"throttle":min(1.0,max(-1.0,(self.dstV-V)/self.accelScale*0.5+0.5))
				},
				"weapon":{
					"launch":self.launchFlag,
					"target":self.target.to_json()
				}
			}
	def convertActionFromAnother(self,decision,command):#摸倣対称の行動または制御出力と近い行動を計算する
		interval=self.manager.getAgentInterval()*self.manager.getBaseTimeStep()
		myMotion=MotionState(self.parent.observables["motion"])
		#ロールは無視
		#水平方向
		decisionType=decision[self.parent.getFullName()]["Horizontal"][0]
		value=decision[self.parent.getFullName()]["Horizontal"][1]
		dAZ=0.0
		if(decisionType=="Rate"):
			dAZ=value*interval
		elif(decisionType=="Az_NED"):
			dAZ=value-myMotion.az
		elif(decisionType=="Az_BODY"):
			dAZ=value
		turnIdx=np.argmin(abs(self.turnTable-atan2(sin(dAZ),cos(dAZ))))
		#垂直方向
		decisionType=decision[self.parent.getFullName()]["Vertical"][0]
		value=decision[self.parent.getFullName()]["Vertical"][1]
		el=0.0
		if(decisionType=="Rate"):
			el=value*interval
		elif(decisionType=="El"):
			el=value
		elif(decisionType=="Pos"):
			tau=10.0
			el=max(-self.pitchScale,min(self.pitchScale,deg2rad((value-myMotion.pos[2])/tau)))
		pitchIdx=np.argmin(abs(self.pitchTable-atan2(sin(el),cos(el))))
		#加減速
		decisionType=decision[self.parent.getFullName()]["Throttle"][0]
		value=decision[self.parent.getFullName()]["Throttle"][1]
		self.baseV=np.linalg.norm(myMotion.vel)
		if(self.parent.isinstance(CoordinatedFighter)):
			#decisionよりcommandを優先する
			m=command.at_p("/"+self.parent.getFullName()+"/motion")
			if(m.contains("dstV")):
				self.dstV=m.at("dstV")()
			else:
				if(decisionType=="Vel"):
					self.dstV=value
				elif(decisionType=="Throttle"):
					#0〜1のスロットルで指定していた場合は、0と1をそれぞれ加減速テーブルの両端とみなし線形変換する。
					self.dstV=self.baseV+self.accelScale*(2*value-1)
				else:#type=="Accel":
					#加速度ベースの指定だった場合は、機体性能や旋回状況に依存するうえ飛行制御則によっても変わり、正確な変換は難しいため符号が合っていればよいという程度で変換。
					self.dstV=self.baseV+min(self.accelScale,max(-self.accelScale,value*15))
		elif(self.parent.isinstance(MassPointFighter)):
			if(decisionType=="Vel"):
				self.dstV=value
			elif(decisionType=="Throttle"):
				#0〜1のスロットルで指定していた場合は、0と1をそれぞれ加減速テーブルの両端とみなし線形変換する。
				self.dstV=self.baseV+self.accelScale*(2*value-1)
			else:#type=="Accel":
				#加速度ベースの指定だった場合は、機体性能や旋回状況に依存するうえ飛行制御則によっても変わり、正確な変換は難しいため符号が合っていればよいという程度で変換。
				self.dstV=self.baseV+min(self.accelScale,max(-self.accelScale,value*15))
		accelIdx=np.argmin(abs(self.accelTable-(self.dstV-self.baseV)))
		#射撃
		if(decision[self.parent.getFullName()]["Fire"][0]):
			expertTarget=Track3D(decision[self.parent.getFullName()]["Fire"][1])
			fireIdx=0
			for i,t in enumerate(self.lastTrackInfo):
				if(t.isSame(expertTarget)):
					fireIdx=i+1
			if(fireIdx>=len(self.fireTable)):
				fireIdx=0
		else:
			fireIdx=0
		return np.array([turnIdx,pitchIdx,accelIdx,fireIdx])