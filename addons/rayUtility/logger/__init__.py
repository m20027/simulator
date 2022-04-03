#-*-coding:utf-8-*-
import json
from ASRCAISim1.common import addPythonClass

#Loggerの登録
from ASRCAISim1.addons.rayUtility.logger.ExpertTrajectoryWriter import ExpertTrajectoryWriter
addPythonClass('Callback','ExpertTrajectoryWriter',ExpertTrajectoryWriter)

from ASRCAISim1.addons.rayUtility.logger.RayMultiEpisodeLogger import RayMultiEpisodeLogger
addPythonClass('Callback','RayMultiEpisodeLogger',RayMultiEpisodeLogger)
