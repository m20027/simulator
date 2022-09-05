from posixpath import join
import time
import torch.nn as nn
import os

from ASRCAISim1.GymManager import GymManager, getDefaultPolicyMapper
from ...environment import BaseEnvironment


class FightNet(nn.Module):
    def __init__(self) -> None:
        super().__init__()
        self.dense = nn.Linear(100, 10)

        self.turn = nn.Linear(10, 2)
        self.pitch = nn.Linear(10, 2)
        self.accel = nn.Linear(10, 2)
        self.fire = nn.Linear(10, 2)

        self.value = nn.Linear(10, 1)

    def forward(self, x, _=None):
        x = self.dense(x)

        turn = self.turn(x)
        pitch = self.pitch(x)
        accel = self.accel(x)
        fire = self.fire(x)

        value = self.value(x)

        return {
            "turn": turn,
            "pitch": pitch,
            "accel": accel,
            "fire": fire,
            "value": value
        }


class Environment(BaseEnvironment):
    def __init__(self, args={}):
        super().__init__()
        abs_dir = os.path.dirname(os.path.abspath(__file__))
        configs = [
            # 基本的な戦闘場面(ルールや機体の配置等)を設定したファイル
            os.path.join(abs_dir, "BVR2v2_rand.json"),
            # 報酬やAgentの割当等、行動判断とその学習に関する設定を記述したファイル
            os.path.join(abs_dir, "ForSecondSample.json")
        ]
        context = {
            "config": configs+[{"Manager": {
                "ViewerType": "None",
                "seed": 123456,
                "Loggers": {
                }
            }}],
            "worker_index": 0,
            "vector_index": 0
        }
        self.env = GymManager(context)
        self.obs_list = []
        self.reward_list = []
        self.player_index = None

    def reset(self, args={}):
        obs = self.env.reset()
        self.obs_list.append(obs)

    def update(self, info, reset):
        pass

    def step(self, actions):
        # 加工処理？
        obs, rewards, dones, infos = self.env.step(actions)
        self.dones = dones
        self.reward_list.append(rewards)
        self.obs_list.append(obs)

    def observation(self, player):
        key = self.player_index[player]
        return self.obs_list[-1][key]

    def terminal(self):
        return self.dones["__all__"]

    def outcome(self):
        return self.env.manager.getRuler().winner

    def players(self):
        # 未実装
        return [p for p in self.players() if self.obs_list[-1][p]['status'] == 'ACTIVE']

    def net(self):
        FightNet

    def turns(self):
        pass
