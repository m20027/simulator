import argparse
import json
from json.decoder import WHITESPACE
import base64
from torch.utils import data

from torch.utils.data.sampler import RandomSampler
import lz4.frame
import pickle
import glob
import torch
import torch.nn as nn
from torch.utils.data import DataLoader, Dataset

import numpy as np

parser = argparse.ArgumentParser()
parser.add_argument("-i", "--input_dir")
args = parser.parse_args()
config = {
    "epoch": 20,
    "batch_size": 128,
}


def loads_iter(s):
    """
    シュミレーションの各ゲームの結果を文字列から読み取る。

    s: シュミレーションデータの文字列が含まれる
    """
    size = len(s)
    decoder = json.JSONDecoder()
    end = 0
    while True:
        idx = WHITESPACE.match(s[end:]).end()
        i = end + idx
        if i >= size:
            break
        ob, end = decoder.raw_decode(s, i)
        yield ob


def load_simulation_data(input_dir):
    """
    初期行動判断モデルの出力をメモリ上に読み込む
    """
    observations = []
    actions = []
    # ディレクトリ一覧
    result_json_paths = glob.glob(f'{input_dir}/**/*.json', recursive=True)
    print(result_json_paths)
    for result_json_path in result_json_paths:
        f = open(result_json_path, "r")
        s = f.read()
        for elem in loads_iter(s):
            obs_all = base64.b64decode(elem["obs"])
            obs_all = lz4.frame.decompress(obs_all)
            obs_all = pickle.loads(obs_all)
            for obs, action in zip(obs_all, elem['actions']):
                observations.append(obs)
                actions.append(action)
    return np.array(observations), np.array(actions)


class FlightNetBase(nn.Module):
    def __init__(self) -> None:
        super().__init__()
        self.hidden = nn.Linear(80, 100)
        self.hidden1 = nn.Linear(100, 50)

        self.turn = nn.Linear(50, 7, bias=False)
        self.fire = nn.Linear(50, 3, bias=False)
        self.pitch = nn.Linear(50, 7, bias=False)
        self.accel = nn.Linear(50, 7, bias=False)

    def forward(self, x, _=None):
        h = torch.relu(self.hidden(x))
        h = torch.relu(self.hidden1(h))

        turn = self.turn(h)
        fire = self.fire(h)
        pitch = self.pitch(h)
        accel = self.accel(h)

        return {
            "turn_p": turn,
            "fire_p": fire,
            "pitch_p": pitch,
            "accel_p": accel,
        }


class ImitationDataset(Dataset):
    def __init__(self, observations, actions) -> None:
        super().__init__()
        self.observations = observations
        self.actions = actions

    def __getitem__(self, index):
        obs = self.observations[index]
        actions = self.actions[index]

        return {
            "x": torch.tensor(obs).float(),
            "turn": torch.tensor(actions[0]).long(),
            "pitch": torch.tensor(actions[1]).long(),
            "accel": torch.tensor(actions[2]).long(),
            "fire": torch.tensor(actions[3]).long()
        }

    def __len__(self):
        return len(self.actions)


# 模倣学習のファイルを読む。
observations, actions = load_simulation_data(args.input_dir)

train_observations = observations[:int(len(observations) * 0.8)]
train_actions = actions[:int(len(observations) * 0.8)]

valid_observations = observations[int(len(observations) * 0.8):]
valid_actions = actions[int(len(observations) * 0.8):]

# 模倣学習のTraining Data
dataloader = DataLoader(
    ImitationDataset(
        observations=train_observations,
        actions=train_actions
    ),
    shuffle=True,
    batch_size=config["batch_size"]
)

# 模倣学習のValidation Data
valid_dataloader = DataLoader(
    ImitationDataset(
        observations=valid_observations,
        actions=valid_actions
    ),
    shuffle=False,
    batch_size=config["batch_size"]
)

# ニューラルネットワークのモデルを取得
model = FlightNetBase()
model = model.cuda()

match_keys = [
    ("turn", "turn_p"),
    ("fire", "fire_p"),
    ("pitch", "pitch_p"),
    ("accel", "accel_p"),
]

optimizer = torch.optim.Adam(model.parameters())
criterion = nn.CrossEntropyLoss()

for epoch in range(config["epoch"]):
    model.train()
    # Training Step
    traning_total_loss = 0.0
    for elem in dataloader:
        x = elem["x"].cuda()
        outputs = model(x)

        loss = None
        for key, p_key in match_keys:
            if loss is None:
                loss = criterion(outputs[p_key], elem[key].cuda())
            else:
                loss += criterion(outputs[p_key], elem[key].cuda())

        loss.backward()
        optimizer.step()
        optimizer.zero_grad()

        traning_total_loss += loss

    model.eval()
    testing_total_loss = 0.0
    # Validation Step
    for elem in valid_dataloader:
        x = elem["x"].cuda()
        outputs = model(x)

        loss = None
        for key, p_key in match_keys:
            if loss is None:
                loss = criterion(outputs[p_key], elem[key].cuda())
            else:
                loss += criterion(outputs[p_key], elem[key].cuda())
        testing_total_loss += loss
    print(f"{epoch} Epoch End Training Loss: {traning_total_loss / len(dataloader)} Testing Loss: {testing_total_loss / len(valid_dataloader)}")

torch.save(model.state_dict(), "imitation_model.pth")
