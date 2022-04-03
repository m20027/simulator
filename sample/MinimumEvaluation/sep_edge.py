# -*-coding:utf-8
import os
import time
import importlib
import ray
from ASRCAISim1.common import addPythonClass
from ASRCAISim1.GymManager import GymManager, SimpleEvaluator
from ASRCAISim1.addons.AgentIsolation import AgentDelegatee, PolicyDelegatee, SimulationManagerForIsolation


def agentServer(userID, server, port):
    try:
        userModule = importlib.import_module(userID)
        assert hasattr(userModule, "getUserAgentClass")
        assert hasattr(userModule, "getUserAgentModelConfig")
    except Exception as e:
        raise e  # 読み込み失敗時の扱いは要検討

    start_time = time.time()
    userAgentClass = userModule.getUserAgentClass()
    addPythonClass("Agent", "Agent_"+userID, userAgentClass)

    # コンフィグの生成
    agentConfig = {
        "Factory": {
            "Agent": {
                "Agent_"+userID: {
                    "class": "Agent_"+userID,
                    "config": userModule.getUserAgentModelConfig()
                }
            }
        }
    }

    end_time = time.time()
    process_time = end_time - start_time

    manager = SimulationManagerForIsolation(agentConfig)
    delegatee = AgentDelegatee(manager, server, port)
    delegatee.process_time = process_time

    print("=====Agent class=====")
    print("Agent_"+userID, " = ", userAgentClass)
    delegatee.run()


def policyServer(userID, server, port):
    try:
        userModule = importlib.import_module(userID)
        assert hasattr(userModule, "getUserPolicy")
    except Exception as e:
        raise e  # 読み込み失敗時の扱いは要検討

    start_time = time.time()
    policies = {
        "Policy_"+userID: userModule.getUserPolicy()
    }
    end_time = time.time()
    process_time = end_time - start_time
    print("=====Policy=====")
    for name, policy in policies.items():
        print(name, " = ", type(policy))
    delegatee = PolicyDelegatee(policies, server, port)
    delegatee.process_time = process_time
    delegatee.run()


def run(config):
    userID = config["userID"]
    server = config["server"]
    agentPort = config["agentPort"]
    policyPort = config["policyPort"]
    import multiprocessing
    ctx = multiprocessing.get_context("spawn")
    agentProcess = ctx.Process(
        target=agentServer, args=(userID, server, agentPort))
    policyProcess = ctx.Process(
        target=policyServer, args=(userID, server, policyPort))
    agentProcess.start()
    policyProcess.start()
    agentProcess.join()
    policyProcess.join()


if __name__ == "__main__":
    import sys
    import json
    config = json.load(open("sep_config.json", "r"))
    assert(len(sys.argv) > 1 and sys.argv[1] in ["blue", "red"])
    run(config[sys.argv[1]])
