# -*-coding:utf-8
import os
import time
import importlib
from ASRCAISim1.common import addPythonClass
from ASRCAISim1.GymManager import GymManager, SimpleEvaluator


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


def run(blueUserID, redUserID, seed=None):
    if(seed is None):
        import numpy as np
        seed = np.random.randint(2**31)
    # ユーザーモジュールの読み込み
    try:
        blueModule = importlib.import_module(blueUserID)
        assert hasattr(blueModule, "getUserAgentClass")
        assert hasattr(blueModule, "getUserAgentModelConfig")
        assert hasattr(blueModule, "isUserAgentSingleAsset")
        assert hasattr(blueModule, "getUserPolicy")
    except Exception as e:
        raise e  # 読み込み失敗時の扱いは要検討
    try:
        redModule = importlib.import_module(redUserID)
        assert hasattr(redModule, "getUserAgentClass")
        assert hasattr(redModule, "getUserAgentModelConfig")
        assert hasattr(redModule, "isUserAgentSingleAsset")
        assert hasattr(redModule, "getUserPolicy")
    except Exception as e:
        raise e  # 読み込み失敗時の扱いは要検討
    blueAgentClass = blueModule.getUserAgentClass()
    redAgentClass = redModule.getUserAgentClass()
    addPythonClass("Agent", "Agent_"+blueUserID, blueAgentClass)
    addPythonClass("Agent", "Agent_"+redUserID, redAgentClass)

    # コンフィグの生成
    agentConfig = {
        "Factory": {
            "Agent": {
                "Agent_"+blueUserID: {
                    "class": "Agent_"+blueUserID,
                    "config": blueModule.getUserAgentModelConfig()
                },
                "Agent_"+redUserID: {
                    "class": "Agent_"+redUserID,
                    "config": redModule.getUserAgentModelConfig()
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
        "Policy_"+blueUserID: blueModule.getUserPolicy(),
        "Policy_"+redUserID: redModule.getUserPolicy()
    }
    # policyMapperの定義(基本はデフォルト通り)

    def policyMapper(fullName):
        agentName, modelName, policyName = fullName.split(":")
        return policyName

    # 生成状況の確認
    observation_space = env.observation_space
    action_space = env.action_space
    print("=====Agent classes=====")
    print("Agent_"+blueUserID, " = ", blueAgentClass)
    print("Agent_"+redUserID, " = ", redAgentClass)
    print("=====Policies=====")
    for name, policy in policies.items():
        print(name, " = ", type(policy))
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
    numEpisodes = 1
    for episodeCount in range(numEpisodes):
        obs = env.reset()
        rewards = {k: 0.0 for k in obs.keys()}
        dones = {k: False for k in obs.keys()}
        infos = {k: None for k in obs.keys()}
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
        print("episode(", episodeCount+1, "/", numEpisodes, "), winner=",
              env.manager.getRuler().winner, ", scores=", {k: v for k, v in env.manager.scores.items()})


if __name__ == "__main__":
    run("User001", "User002")
