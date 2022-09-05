# -*-coding:utf-8-*-
import os
import json
import ASRCAISim1
from ASRCAISim1.libCore import Factory
from ASRCAISim1.common import addPythonClass
from ASRCAISim1.policy import StandalonePolicy

import torch
import torch.nn as nn
import numpy as np


# ①Agentクラスオブジェクトを返す関数を定義
"""
以下はサンプルのAgentクラスを借りてくる場合の例
"""


def getUserAgentClass():
    from OriginalModelSample import R3PyAgentSample01
    return R3PyAgentSample01


# ②Agentモデル登録用にmodelConfigを表すjsonを返す関数を定義
"""
なお、modelConfigとは、Agentクラスのコンストラクタに与えられる二つのjson(dict)のうちの一つであり、設定ファイルにおいて
{
	"Factory":{
		"Agent":{
			"modelName":{
				"class":"className",
				"config":{...}
			}
		}
	}
}の"config"の部分に記載される{...}のdictをが該当する。
"""


def getUserAgentModelConfig():
    return json.load(open(os.path.join(os.path.dirname(__file__), "config.json"), "r"))


# ③Agentの種類(一つのAgentインスタンスで1機を操作するのか、2機両方を操作するのか)を返す関数を定義
"""AgentがAssetとの紐付けに使用するportの名称は本来任意であるが、
　簡単のために1機を操作する場合は"0"、2機を操作する場合は"0"と"1"で固定とする。
"""


def isUserAgentSingleAsset():
    # 1機だけならばTrue,2機両方ならばFalseを返すこと。
    return False


class FlightNetBase(nn.Module):
    def __init__(self) -> None:
        super().__init__()
        self.hidden = nn.Linear(80, 100)
        self.hidden1 = nn.Linear(100, 50)

        self.turn = nn.Linear(50, 7, bias=False)
        self.fire = nn.Linear(50, 3, bias=False)
        self.pitch = nn.Linear(50, 7, bias=False)
        self.accel = nn.Linear(50, 7, bias=False)

        self.v = nn.Linear(50, 1)

    def forward(self, x, _=None):
        h = torch.relu(self.hidden(x))
        h = torch.relu(self.hidden1(h))

        turn = self.turn(h)
        fire = self.fire(h)
        pitch = self.pitch(h)
        accel = self.accel(h)

        v = torch.tanh(self.v(h))

        return {
            "turn_p": turn,
            "fire_p": fire,
            "pitch_p": pitch,
            "accel_p": accel,
            'value': v
        }

def get_model():
    return FlightNetBase()


class NeuralNetworkPolicy(StandalonePolicy):
    """
    HandyRLで学習させたモデルを利用するポリシー
    """

    def __init__(self) -> None:
        super().__init__()

        # Initialize Neural Network
        self.model = get_model()
        self.model.load_state_dict(torch.load(os.path.join(
            os.path.dirname(__file__), "model.pth"), map_location=torch.device('cpu')))
        self.model.eval()
        self.output_list = [
            ("turn_p", "turn"),
            ("fire_p", "fire"),
            ("pitch_p", "pitch"),
            ("accel_p", "accel"),
            ("fire_p", "fire")
        ]

    def step(self, observation, reward, done, info, agentFullName, observation_space, action_space):
        observation = torch.tensor(observation)
        output = self.model(observation)
        action = [] 
        for (policy_key, action_key) in self.output_list:
            action.append(np.argmax(
                output[policy_key].detach().numpy()))

        return action


def getUserPolicy():
    return NeuralNetworkPolicy()
