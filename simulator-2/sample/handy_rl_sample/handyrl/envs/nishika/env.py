import time
import torch
import torch.nn as nn

from ASRCAISim1.GymManager import GymManager, getDefaultPolicyMapper
from ...environment import BaseEnvironment
import OriginalModelSample
import os


class FlightNetBase(nn.Module):
    def __init__(self) -> None:
        super().__init__()
        self.hidden = nn.Linear(80, 100)
        self.hidden1 = nn.Linear(100, 50)

        self.turn = nn.Linear(50, 7, bias=False)
        self.fire = nn.Linear(50, 7, bias=False)
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


class Environment(BaseEnvironment):
    def __init__(self, args={}):
        super().__init__()
        directory = os.path.dirname(__file__)
        configs = [
            # 基本的な戦闘場面(ルールや機体の配置等)を設定したファイル
            os.path.join(directory, "BVR2v2_rand.json"),
            # 報酬やAgentの割当等、行動判断とその学習に関する設定を記述したファイル
            os.path.join(directory, "ForSecondSample.json")
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
        print("=====Agent to Asset map=====")
        for agent in self.env.manager.getAgents():
            print(agent.getFullName(), " -> ", "{")
            for port, parent in agent.parents.items():
                print("  ", port, " : ", parent.getFullName())
            print("}")

        self.dones = {
            "__all__": False
        }
        self.player_list = []
        self.dones_list = [self.dones]

    def reset(self, args={}):
        obs = self.env.reset()
        self.obs_list = []
        self.obs_list.append(obs)
        if len(self.player_list) == 0:
            self.player_list = [key for key in obs.keys()]

        self.dones = {
            "__all__": False
        }

        self.dones_list = [
            {
                "__all__": False
            }]

    def observation(self, player=None):
        obs = self.obs_list[-1]
        return obs[self.player_list[player]]

    def update(self, info, reset):
        pass

    def step(self, actions):
        action_dict = {}
        for idx, player in enumerate(self.player_list):
            if idx in actions:
                action_dict[player] = actions[idx]
        obs, rewards, dones, infos = self.env.step(action_dict)
        self.dones = dones
        self.obs_list.append(obs)
        self.dones_list.append(dones)

    def reward(self):
        # ゲーム終了時の報酬
        if self.dones["__all__"]:
            return {0: 0, 1: 0, 2: 0, 3: 0}

        # 撃墜確認のReward、__done__がTrueで消滅するため、撃墜された場合は
        # False->Trueへの変更。そのため、2F前と1F前を比較して確認する。
        rewards = {}
        last_frame, last2_frame = self.dones_list[-1], self.dones_list[-2]
        for index, player in enumerate(self.player_list):
            # 撃墜確認用
            last_frame_state = last_frame[player]
            if player in last2_frame:
                last2_frame_state = last2_frame[player]
            else:
                last2_frame_state = False
            reward = float(last2_frame_state - last_frame_state)
            rewards[index] = reward
        return rewards

    def terminal(self):
        return self.dones["__all__"]

    def outcome(self):
        # デバッグしたくなった際にコメントアウトを戻すこと。
        print ({k:v for k,v in self.env.manager.scores.items()}, self.env.manager.getRuler().winner)
        if self.env.manager.getRuler().winner == "Blue":
            return {0: 1, 1: 1, 2: -1, 3: -1}
        else:
            return {0: -1, 1: -1, 2: 1, 3: 1}

    def action_length(self):
        return self.env.get_action_space()

    def legal_actions(self, player):
        return self.env.get_action_space()[self.player_list[player]]

    def players(self):
        return [0, 1, 2, 3]

    def turns(self):
        if len(self.dones) == 1:
            return [p for p in self.players()]
        return [p for p in self.players() if not self.dones[self.player_list[p]]]
        # return [p for p in self.players() if self.obs_list[-1][p]['status'] == 'ACTIVE']

    def net(self):
        return FlightNetBase
