# -*-coding:utf-8-*-
import bz2
import datetime
import numpy as np
import os
import pickle
from ASRCAISim1.libCore import *


class GodViewStateLogger(Callback):
    """GodViewと同等の表示に必要なシミュレーション情報をログとして保存するためのLogger。
    保存されたログをGodViewLoaderクラスで読み込むことで表示が可能。
    """

    def __init__(self, modelConfig, instanceConfig):
        super().__init__(modelConfig, instanceConfig)
        self.prefix = getValueFromJsonKRD(
            self.modelConfig, "prefix", self.randomGen, "")
        self.episodeInterval = getValueFromJsonKRD(
            self.modelConfig, "episodeInterval", self.randomGen, 1)
        self.innerInterval = getValueFromJsonKRD(
            self.modelConfig, "innerInterval", self.randomGen, 1)
        self.episodeCounter = 0
        self.innerCounter = 0
        self.timeStamp = datetime.datetime.now().strftime("%Y%m%d%H%M%S")
        self.fo = None
        self.pickler = None

    def onEpisodeBegin(self):
        """エピソードの開始時(reset関数の最後)に呼ばれる。
        """
        self.innerCounter = 0
        self.episodeCounter += 1
        path = self.prefix+"_"+self.timeStamp + \
            "_e{:04d}.dat".format(self.episodeCounter)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        self.fo = bz2.BZ2File(path, "wb", compresslevel=9)
        self.pickler = pickle.Pickler(self.fo, protocol=4)

    def onInnerStepEnd(self):
        """#インナーループの各ステップの最後(perceiveの後)に呼ばれる。
        """
        self.innerCounter += 1
        if(self.episodeCounter % self.episodeInterval == 0):
            if(self.innerCounter % self.innerInterval == 0):
                self.pickler.dump(self.makeFrame())
                self.pickler.clear_memo()

    def onEpisodeEnd(self):
        """エピソードの終了時(step関数の最後でdone==Trueの場合)に呼ばれる
        """
        self.fo.close()
        self.fo = None
        self.pickler = None

    def makeFrame(self):
        """1フレーム分の描画に必要なデータを集める
        """
        ret = {"time": self.manager.getTime(
        ), "tick": self.manager.getTickCount()}
        ruler = self.manager.getRuler()
        ret["ruler"] = {
            "dOut": ruler.dOut,
            "dLine": ruler.dLine,
            "hLim": ruler.hLim,
            "westSider": ruler.westSider,
            "eastSider": ruler.eastSider,
            "forwardAx": {k: v for k, v in ruler.forwardAx.items()}
        }
        ret["scores"] = {k: v for k, v in self.manager.scores.items()}
        ret["agents"] = {agent.getFullName(): {
            "team": agent.getTeam(),
            "repr": agent.__repr__(),
            "observables": agent.observables()
        } for agent in self.manager.getAgents()
        }
        ret["totalRewards"] = {k: v for k,
                               v in self.manager.totalRewards.items()}
        ret["fighters"] = {f.getFullName(): {
            "isAlive": f.isAlive(),
            "team": f.getTeam(),
            "name": f.getName(),
            "posI": f.posI(),
            "velI": f.velI(),
            "agent": f.agent.getFullName(),
            "remMsls": f.remMsls,
            "motion": f.motion.to_json()(),
            "ex": f.relBtoI(np.array([1., 0., 0.])),
            "ey": f.relBtoI(np.array([0., 1., 0.])),
            "ez": f.relBtoI(np.array([0., 0., 1.])),
            "radar": {
                "Lref": f.radar.Lref
            }
        } for f in self.manager.getAssets(lambda a: isinstance(a, Fighter))}
        ret["missiles"] = {m.getFullName(): {
            "isAlive": m.isAlive(),
            "team": m.getTeam(),
            "name": m.getName(),
            "posI": m.posI(),
            "velI": m.velI(),
            "motion": m.motion.to_json()(),
            "ex": m.relBtoI(np.array([1., 0., 0.])),
            "ey": m.relBtoI(np.array([0., 1., 0.])),
            "ez": m.relBtoI(np.array([0., 0., 1.])),
            "hasLaunched": m.hasLaunched,
            "mode": m.mode.name,
            "estTPos": m.estTPos.copy(),
            "sensor": {
                "isActive": m.sensor.isActive,
                "Lref": m.sensor.Lref,
                "thetaFOR": m.sensor.thetaFOR
            }
        } for m in self.manager.getAssets(lambda a: isinstance(a, Missile))}

        def getTypes(src):
            if(isinstance(src, dict)):
                return {k: getTypes(v) for k, v in src.items()}
            elif(isinstance(src, list) or isinstance(src, tuple)):
                return [getTypes(v) for v in src]
            else:
                return type(src)
        # print(getTypes(ret))

        def toList(src):
            if(isinstance(src, dict)):
                return {k: toList(v) for k, v in src.items()}
            elif(isinstance(src, list) or isinstance(src, tuple)):
                return [toList(v) for v in src]
            elif(isinstance(src, np.ndarray)):
                return src.tolist()
            else:
                return src
        return ret