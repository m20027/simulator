# Copyright (c) 2020 DeNA Co., Ltd.
# Licensed under The MIT License [see LICENSE for details]

# neural nets

from .util import map_r
import torch.nn.functional as F
import torch.nn as nn
import torch
import numpy as np
import os
os.environ['OMP_NUM_THREADS'] = '1'

torch.set_num_threads(1)


def to_torch(x):
    return map_r(x, lambda x: torch.from_numpy(np.array(x)).contiguous() if x is not None else None)


def to_numpy(x):
    return map_r(x, lambda x: x.detach().numpy() if x is not None else None)


def to_gpu(data):
    return map_r(data, lambda x: x.cuda() if x is not None else None)


# model wrapper class

class ModelWrapper(nn.Module):
    def __init__(self, model):
        super().__init__()
        self.model = model

    def init_hidden(self, batch_size=None):
        if hasattr(self.model, 'init_hidden'):
            return self.model.init_hidden(batch_size)
        return None

    def forward(self, *args, **kwargs):
        return self.model.forward(*args, **kwargs)

    def inference(self, x, hidden, **kwargs):
        # numpy array -> numpy array
        if hasattr(self.model, 'inference'):
            return self.model.inference(x, hidden, **kwargs)

        self.eval()
        with torch.no_grad():
            xt = map_r(x, lambda x: torch.from_numpy(
                np.array(x)).contiguous().unsqueeze(0) if x is not None else None)
            ht = map_r(hidden, lambda h: torch.from_numpy(
                np.array(h)).contiguous().unsqueeze(0) if h is not None else None)
            outputs = self.forward(xt, ht, **kwargs)
        return map_r(outputs, lambda o: o.detach().numpy().squeeze(0) if o is not None else None)


# simple model

class RandomModel(nn.Module):
    def __init__(self, env):
        super().__init__()
        self.action_length = env.action_length()

    def inference(self, x=None, hidden=None):
        return {'policy': np.zeros(self.action_length, dtype=np.float32), 'value': np.zeros(1, dtype=np.float32)}


class RandomFlightModel(nn.Module):
    def __init__(self, env) -> None:
        super().__init__()
        self.action_length = list(
            [value for value in env.action_length().values()][0])

    def inference(self, x=None, hidden=None):
        policy_list = ['turn_p', 'pitch_p', 'accel_p', 'fire_p']
        returns = {}
        for action_list, policy in zip(self.action_length, policy_list):
            returns[policy] = np.zeros(action_list.n, dtype=np.float32)
        returns['value'] = np.zeros(1, dtype=np.float32)
        return returns
