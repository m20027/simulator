#-*-coding:utf-8-*-
import os,json
from ASRCAISim1.libCore import Factory,Viewer
from ASRCAISim1.common import addPythonClass
try:
	from ASRCAISim1.viewer.GodView import GodView

	addPythonClass('Viewer','GodView',GodView)
except Exception as ex:
	ex_var=ex
	class DummyGodView(Viewer):
		def __init__(self,modelConfig,instanceConfig):
			super().__init__(modelConfig,instanceConfig)
			self.exception=ex_var
			print("==============================")
			print("Warning: Godview was not imported collectly. The exception occured is,")
			print(self.exception)
			print("To continue the simulation, a dummy no-op Viewer will be used instead.")
			print("==============================")
	addPythonClass('Viewer','GodView',DummyGodView)
