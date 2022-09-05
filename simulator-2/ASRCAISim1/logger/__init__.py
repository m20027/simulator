#-*-coding:utf-8-*-
import json
from ASRCAISim1.common import addPythonClass

#Loggerの登録

from ASRCAISim1.logger.LoggerSample import BasicLogger
addPythonClass('Callback','BasicLogger',BasicLogger)

from ASRCAISim1.logger.MultiEpisodeLogger import MultiEpisodeLogger
addPythonClass('Callback','MultiEpisodeLogger',MultiEpisodeLogger)

try:
	from ASRCAISim1.logger.GodViewLogger import GodViewLogger
	addPythonClass('Callback','GodViewLogger',GodViewLogger)
except:
	pass

from ASRCAISim1.logger.GodViewStateLogger import GodViewStateLogger
addPythonClass('Callback','GodViewStateLogger',GodViewStateLogger)