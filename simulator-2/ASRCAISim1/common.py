#-*-coding:utf-8-*-
import os,json
from ASRCAISim1.libCore import *

def addPythonClass(baseName,className,clsObj):
    def creator(modelConfig,instanceConfig):
        ret=clsObj(modelConfig,instanceConfig)
        return Factory.keepAlive(ret)
    Factory.addClass(baseName,className,creator)

