#-*-coding:utf-8-*-
import json
from ASRCAISim1.common import addPythonClass


#Callbackの登録
from ASRCAISim1.callback.EpisodeMonitor import EpisodeMonitor
addPythonClass('Callback','EpisodeMonitor',EpisodeMonitor)

