# -*-coding:utf-8-*-
from math import *
from gym import spaces
import numpy as np
import sys
from ASRCAISim1.libCore import *
from OriginalModelSample.libOriginalModelSample import *
import time


class AgentDelegatee:
    """隔離されたユーザー環境側で動く方。
    """

    def __init__(self, manager, server, port):
        self.manager = manager
        self.socketServer = server
        self.socketPort = port
        self.isRunning = False
        self.process_time = 0
        # 時間計測の対象にする関数名
        self.time_command = [
            "observation_space",
            "makeObs",
            "deploy",
            "validate",
            "perceive",
            "control",
            "behave",
            "initialize"
        ]

    def kill(self, name, data):
        # 終了用(引数はダミー)
        self.isRunning = False
        return None

    def clear(self, name, data):
        # 次エピソードへの準備用(引数はダミー)
        self.manager.clear()
        return None

    def clear_turn(self, name, data):
        self.process_time = 0

    def get_turn_time(self, name, data):
        """
        ターンごとの時間を取得する。
        """
        return self.process_time

    def run(self):
        """コマンドとデータを待ち受けて、処理して返事を返す。
        """
        import socket
        import pickle
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.bind((self.socketServer, self.socketPort))
        s.listen(1)
        self.isRunning = True
        bufferSize = 4096
        while self.isRunning:
            conn, addr = s.accept()
            header = conn.recv(bufferSize).decode("utf-8")
            msgLen = int(header[7:])
            conn.send("ACK:OK".encode("utf-8"))
            received = 0
            msg = b""
            while received < msgLen:
                part = conn.recv(bufferSize)
                received += len(part)
                if(len(part) > 0):
                    msg += part
            assert(received == msgLen)
            agentFullName, command, data = pickle.loads(msg)
            funcs = {
                "clear": self.clear,
                "initialize": self.manager.initialize,
                "action_space": self.manager.action_space,
                "observation_space": self.manager.observation_space,
                "makeObs": self.manager.makeObs,
                "deploy": self.manager.deploy,
                "validate": self.manager.validate,
                "perceive": self.manager.perceive,
                "control": self.manager.control,
                "behave": self.manager.behave,
                "kill": self.kill,
                "get_turn_time": self.get_turn_time,
                "clear_turn": self.clear_turn
            }

            if command in self.time_command:
                start_time = time.time()
                ret = pickle.dumps(funcs[command](agentFullName, data))
                end_time = time.time()
                self.process_time += end_time - start_time
            else:
                ret = pickle.dumps(funcs[command](agentFullName, data))
            header = "HEADER:{:16d}".format(len(ret)).encode("utf-8")
            conn.send(header)
            ack = conn.recv(bufferSize).decode("utf-8")
            assert(ack[4:] == "OK")
            conn.send(ret)
            conn.close()
        s.close()
