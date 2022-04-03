#-*-coding:utf-8-*-
import os,glob,importlib
addonList=glob.glob(os.path.dirname(__file__)+"/*/")
loaded=[]
for a in addonList:
    name=os.path.basename(a[:-1])
    if(name != "__pycache__"):
        try:
            importlib.import_module("ASRCAISim1.addons."+name,name)
            loaded.append(name)
        except:
            pass
if(len(loaded)==0):
    print("No addons loaded.")
else:
    print("Loaded addons = ",loaded)