# -*-coding:utf-8
import os
import time
import importlib
import ray
from ASRCAISim1.common import addPythonClass
from ASRCAISim1.GymManager import GymManager, SimpleEvaluator
from ASRCAISim1.addons.AgentIsolation import PolicyDelegator


def agentConfigMaker(userID: str, isSingle: bool) -> dict:
    # 青と赤のAgent指定部分のコンフィグを生成
    if(isSingle):
        # 1個のインスタンスで1機なので異なるnameを割り当てることで2個のインスタンスとし、portはどちらも"0"
        return {
            "type": "group",
            "order": "fixed",
            "elements": [
                {"type": "External", "model": "Agent_"+userID, "policy": "Policy_" +
                    userID, "name": "Agent_"+userID+"_1", "port": "0"},
                {"type": "External", "model": "Agent_"+userID, "policy": "Policy_" +
                    userID, "name": "Agent_"+userID+"_2", "port": "0"}
            ]
        }
    else:
        # 1個のインスタンスで2機分なので同じnameを割り当てることで1個のインスタンスとし、それぞれ異なるport("0"と"1")を割り当てる
        return {
            "type": "group",
            "order": "fixed",
            "elements": [
                {"type": "External", "model": "Agent_"+userID,
                    "policy": "Policy_"+userID, "name": "Agent_"+userID, "port": "0"},
                {"type": "External", "model": "Agent_"+userID,
                    "policy": "Policy_"+userID, "name": "Agent_"+userID, "port": "1"}
            ]
        }


class TimeController(object):
    """
    時間制約を計算する。
    """

    def __init__(self):
        self.turn_timer = 1
        self.cummulatiton_time = 65

    def update(self, second):
        over_time = self.turn_timer - second
        if over_time < 0:
            self.cummulatiton_time = self.cummulatiton_time + over_time

    def update_cummulatiton_time(self, second):
        self.cummulatiton_time = self.cummulatiton_time - second

    def get_status(self):
        if self.cummulatiton_time < 0:
            return False
        else:
            return True


def postGlobalCommand(command, server, port):
    # 終了処理(kill)や次エピソードへの準備(clear)のため
    import socket
    import pickle
    bufferSize = 4100
    conn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    conn.connect((server, port))
    msg = pickle.dumps(["", command, None])
    header = "HEADER:{:16d}".format(len(msg)).encode("utf-8")
    conn.send(header)
    ack = conn.recv(bufferSize).decode("utf-8")
    assert(ack[4:] == "OK")
    conn.send(msg)
    header = conn.recv(bufferSize).decode("utf-8")
    msgLen = int(header[7:])
    conn.send("ACK:OK".encode("utf-8"))
    received = 0
    ret = b""
    while received < msgLen:
        part = conn.recv(bufferSize)
        received += len(part)
        if(len(part) > 0):
            ret += part
    assert(received == msgLen)
    conn.close()
    return pickle.loads(ret)


def run(config):
    blueUserID = config["blue"]["userID"]
    redUserID = config["red"]["userID"]
    seed = config.get("seed", None)
    blueServer = config["blue"]["server"]
    blueAgentPort = config["blue"]["agentPort"]
    bluePolicyPort = config["blue"]["policyPort"]
    redServer = config["red"]["server"]
    redAgentPort = config["red"]["agentPort"]
    redPolicyPort = config["red"]["policyPort"]
    if(seed is None):
        import numpy as np
        seed = np.random.randint(2**31)
    # ユーザーモジュールの読み込み
    try:
        blueModule = importlib.import_module(blueUserID)
        assert hasattr(blueModule, "isUserAgentSingleAsset")
    except Exception as e:
        raise e  # 読み込み失敗時の扱いは要検討
    try:
        redModule = importlib.import_module(redUserID)
        assert hasattr(redModule, "isUserAgentSingleAsset")
    except Exception as e:
        raise e  # 読み込み失敗時の扱いは要検討

    # コンフィグの生成
    agentConfig = {
        "Factory": {
            "Agent": {
                "Agent_"+blueUserID: {
                    "class": "AgentDelegator",
                    "config": {
                        "socketServer": blueServer,
                        "socketPort": blueAgentPort
                    }
                },
                "Agent_"+redUserID: {
                    "class": "AgentDelegator",
                    "config": {
                        "socketServer": redServer,
                        "socketPort": redAgentPort
                    }
                }
            }
        },
        "Manager": {
            "AgentConfigDispatcher": {
                "BlueAgents": agentConfigMaker(blueUserID, blueModule.isUserAgentSingleAsset()),
                "RedAgents": agentConfigMaker(redUserID, redModule.isUserAgentSingleAsset())
            }
        }
    }
    configs = [
        os.path.join(os.path.dirname(__file__), "common/BVR2v2_rand.json"),
        agentConfig,
        {
            "Manager": {
                "Rewards": [],
                "seed":seed,
                "ViewerType":"None",
                "Loggers":{
                }
            }
        }
    ]
    context = {
        "config": configs,
        "worker_index": 0,
        "vector_index": 0
    }
    # 環境の生成
    env = GymManager(context)

    # StandalonePolicyの生成
    policies = {
        "Policy_"+blueUserID: PolicyDelegator("Policy_"+blueUserID, blueServer, bluePolicyPort),
        "Policy_"+redUserID: PolicyDelegator("Policy_"+redUserID, redServer, redPolicyPort)
    }
    # policyMapperの定義(基本はデフォルト通り)

    def policyMapper(fullName):
        agentName, modelName, policyName = fullName.split(":")
        return policyName

    # 生成状況の確認
    observation_space = env.observation_space
    action_space = env.action_space
    print("=====Policy Map (at reset)=====")
    for fullName in action_space:
        print(fullName, " -> ", policyMapper(fullName))
    print("=====Agent to Asset map=====")
    for agent in env.manager.getAgents():
        print(agent.getFullName(), " -> ", "{")
        for port, parent in agent.parents.items():
            print("  ", port, " : ", parent.getFullName())
        print("}")

    # シミュレーションの実行
    print("=====running simulation(s)=====")
    numEpisodes = 1  # 現時点では1エピソードのみ
    for episodeCount in range(numEpisodes):
        obs = env.reset()
        rewards = {k: 0.0 for k in obs.keys()}
        dones = {k: False for k in obs.keys()}
        infos = {k: None for k in obs.keys()}
        time_manager = [
            TimeController(),
            TimeController()
        ]
        fail_red_flg = False
        fail_blue_flg = False

        # 時間取得
        red_time = postGlobalCommand(
            "get_turn_time", redServer, redPolicyPort)
        red_time += postGlobalCommand("get_turn_time",
                                      redServer, redAgentPort)

        blue_time = postGlobalCommand(
            "get_turn_time", blueServer, blueAgentPort)
        blue_time += postGlobalCommand("get_turn_time",
                                       blueServer, bluePolicyPort)
        # 時間取得
        postGlobalCommand("clear_turn", blueServer, blueAgentPort)
        postGlobalCommand("clear_turn", blueServer, bluePolicyPort)
        postGlobalCommand("clear_turn", redServer, redPolicyPort)
        postGlobalCommand("clear_turn", redServer, redAgentPort)

        # 初期化の時刻になるため、累積時間側の制限を削る。
        time_manager[0].update_cummulatiton_time(red_time)
        time_manager[1].update_cummulatiton_time(blue_time)

        for p in policies.values():
            p.reset()
        dones["__all__"] = False
        while not dones["__all__"]:
            observation_space = env.get_observation_space()
            action_space = env.get_action_space()
            actions = {k: policies[policyMapper(k)].step(
                o,
                rewards[k],
                dones[k],
                infos[k],
                k,
                observation_space[k],
                action_space[k]
            ) for k, o in obs.items() if policyMapper(k) in policies}
            obs, rewards, dones, infos = env.step(actions)

            # 時間取得
            red_time = postGlobalCommand(
                "get_turn_time", redServer, redPolicyPort)
            red_time += postGlobalCommand("get_turn_time",
                                          redServer, redAgentPort)

            blue_time = postGlobalCommand(
                "get_turn_time", blueServer, blueAgentPort)
            blue_time += postGlobalCommand("get_turn_time",
                                           blueServer, bluePolicyPort)

            # 時間取得
            postGlobalCommand("clear_turn", blueServer, blueAgentPort)
            postGlobalCommand("clear_turn", blueServer, bluePolicyPort)
            postGlobalCommand("clear_turn", redServer, redPolicyPort)
            postGlobalCommand("clear_turn", redServer, redAgentPort)

            time_manager[0].update(red_time)
            time_manager[1].update(blue_time)

            # print(time_manager[0].cummulatiton_time, red_time,
            #       time_manager[1].cummulatiton_time, blue_time)

            finish = False
            # REDの失格判定
            if time_manager[0].get_status() is False:
                fail_red_flg = True
                finish = True
            # BLUEの失格判定
            if time_manager[1].get_status() is False:
                fail_blue_flg = True
                finish = True
            # 失格になる場合には終了する。
            if finish:
                break

        postGlobalCommand("clear", blueServer, blueAgentPort)
        postGlobalCommand("clear", redServer, redAgentPort)

        # 仮置きの判定、失格の場合と正常終了で表示を分岐させている。
        if fail_red_flg is False and fail_blue_flg is False:
            print("episode(", episodeCount+1, "/", numEpisodes, "), winner=",
                  env.manager.getRuler().winner, ", scores=", {k: v for k, v in env.manager.scores.items()})
        else:
            if fail_red_flg and fail_blue_flg:
                print("DRAW")
            elif fail_blue_flg:
                print("Blue:DQ")
            elif fail_red_flg:
                print("RED: DQ")
        # TODO: 失格判定を踏まえた勝者判定
    # 終了処理
    postGlobalCommand("kill", blueServer, blueAgentPort)
    postGlobalCommand("kill", blueServer, bluePolicyPort)
    postGlobalCommand("kill", redServer, redAgentPort)
    postGlobalCommand("kill", redServer, redPolicyPort)


if __name__ == "__main__":
    import json
    config = json.load(open("sep_config.json", "r"))
    run(config)
