#-*-coding:utf-8-*-
import os,pickle
import ray
def loadWeights(path):
	with open(path,"rb") as f:
		return  pickle.load(f)
def saveWeights(weights,path):
	os.makedirs(os.path.dirname(path),exist_ok=True)
	with open(path,"wb") as f:
		pickle.dump(weights,f)
def loadPolicyWeights(policy,path):
	policy.set_weights(loadWeights(path))
def savePolicyWeights(policy,path):
	saveWeights(policy.get_weights(),path)
