#-*-coding:utf-8-*-
import os,json
import ASRCAISim1
from ASRCAISim1.libCore import Factory
from ASRCAISim1.common import addPythonClass
if(os.name=="nt"):
    __coreDir=os.path.dirname(ASRCAISim1.libCore.__file__)
    try: # Python 3.8's DLL handling
        os.add_dll_directory(__coreDir)
    except AttributeError:  # <3.8, use PATH
        os.environ['PATH'] += os.pathsep + __coreDir
    libName="OriginalModelSample"
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
    from OriginalModelSample.libOriginalModelSample import *
except ImportError as e:
    if(os.name=="nt"):
        print("Failed to import the module. If you are using Windows, please make sure that: ")
        print('(1) If you are using conda, CONDA_DLL_SEARCH_MODIFICATION_ENABLE should be set to 1.')
        print('(2) dll dependencies (such as nlopt) are located appropriately.')
    raise e

from OriginalModelSample.R3PyAgentSample01 import R3PyAgentSample01
from OriginalModelSample.R3PyAgentSample02 import R3PyAgentSample02
from OriginalModelSample.R3PyRewardSample01 import R3PyRewardSample01

addPythonClass('Agent','R3PyAgentSample01',R3PyAgentSample01)
addPythonClass('Agent','R3PyAgentSample02',R3PyAgentSample02)
addPythonClass('Reward','R3PyRewardSample01',R3PyRewardSample01)

Factory.addDefaultModelsFromJsonFile(os.path.join(os.path.dirname(__file__),"./config/R3SampleConfig01.json"))
