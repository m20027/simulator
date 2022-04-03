# -*-coding:utf-8-*-
from ASRCAISim1.policy.StandalonePolicy import StandalonePolicy
import time


class PolicyDelegatee:
    """隔離されたユーザー環境側で動く方。
    """

    def __init__(self, policies, server, port):
        self.policies = policies
        self.socketServer = server
        self.socketPort = port
        self.isRunning = False
        self.process_time = 0
        # 計測対象のコマンド
        self.time_command = [
            "step",
        ]

    def kill(self, name, data):
        # 終了用(引数はダミー)
        self.isRunning = False
        return None

    def reset(self, policyName, data):
        self.policies[policyName].reset()
        return None

    def clear_turn(self, name, data):
        self.process_time = 0

    def get_turn_time(self, name, data):
        """
        ターンごとの時間を取得する。
        """
        return self.process_time

    def step(self, policyName, data):
        return self.policies[policyName].step(
            data["observation"],
            data["reward"],
            data["done"],
            data["info"],
            data["agentFullName"],
            data["observation_space"],
            data["action_space"]
        )

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
            policyName, command, data = pickle.loads(msg)
            funcs = {
                "reset": self.reset,
                "step": self.step,
                "kill": self.kill,
                "get_turn_time": self.get_turn_time,
                "clear_turn": self.clear_turn
            }
            if command in self.time_command:
                start_time = time.time()
                ret = pickle.dumps(funcs[command](policyName, data))
                end_time = time.time()
                self.process_time += end_time - start_time
            else:
                ret = pickle.dumps(funcs[command](policyName, data))
            header = "HEADER:{:16d}".format(len(ret)).encode("utf-8")
            conn.send(header)
            ack = conn.recv(bufferSize).decode("utf-8")
            assert(ack[4:] == "OK")
            conn.send(ret)
            conn.close()
        s.close()
