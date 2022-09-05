#-*-coding:utf-8-*-
import os,json
if(os.name=="nt"):
    libName="Core"
    pyd=os.path.join(os.path.dirname(__file__),"lib"+libName+".pyd")
    if(not os.path.exists(pyd) or os.path.getsize(pyd)==0):
        print("Info: Maybe the first run after install. A hardlink to a dll will be created.")
        if(os.path.exists(pyd)):
            os.remove(pyd)
        dll=os.path.join(os.path.dirname(__file__),"lib"+libName+".dll")
        if(not os.path.exists(dll)):
            dll=os.path.join(os.path.dirname(__file__),""+libName+".dll")
        if(not os.path.exists(dll)):
            raise FileNotFoundError("There is no lib"+libName+".dll or "+libName+".dll.")
        import subprocess
        subprocess.run([
            "fsutil",
            "hardlink",
            "create",
            pyd,
            dll
        ])
try:
    from ASRCAISim1.libCore import *
except ImportError as e:
    if(os.name=="nt"):
        print("Failed to import the module. If you are using Windows, please make sure that: ")
        print('(1) If you are using conda, CONDA_DLL_SEARCH_MODIFICATION_ENABLE should be set to 1.')
        print('(2) dll dependencies (such as nlopt) are located appropriately.')
    raise e
from ASRCAISim1.common import *
from ASRCAISim1.GymManager import *

Factory.addDefaultModelsFromJsonFile(os.path.join(os.path.dirname(__file__),"./config/Agent.json"))
Factory.addDefaultModelsFromJsonFile(os.path.join(os.path.dirname(__file__),"./config/Asset.json"))
Factory.addDefaultModelsFromJsonFile(os.path.join(os.path.dirname(__file__),"./config/Controller.json"))
Factory.addDefaultModelsFromJsonFile(os.path.join(os.path.dirname(__file__),"./config/RulerAndReward.json"))
Factory.addDefaultModelsFromJsonFile(os.path.join(os.path.dirname(__file__),"./config/Viewer.json"))
from ASRCAISim1.callback import *
from ASRCAISim1.logger import *
from ASRCAISim1.utility import *
from ASRCAISim1.viewer import *
from ASRCAISim1.policy import *
from ASRCAISim1.addons import *
