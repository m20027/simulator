"""extention of RLlib's MARWIL for RNN policy
1. Adds RNN states into SampleBatch as postprocess.
2. Enables burn-in for replay.
"""
import gym
import numpy as np
from typing import Dict

import ray
from ray.rllib.agents.marwil.marwil_torch_policy import MARWILTorchPolicy
from ASRCAISim1.addons.rayUtility.extension.agents.marwil.rnn_marwil_tf_policy import postprocess_all
import ASRCAISim1

RNNMARWILTorchPolicy = MARWILTorchPolicy.with_updates(
    name="RNNMARWILTorchPolicy",
    get_default_config=lambda: ASRCAISim1.addons.rayUtility.extension.agents.marwil.rnn_marwil.DEFAULT_CONFIG,
    postprocess_fn=postprocess_all
)
