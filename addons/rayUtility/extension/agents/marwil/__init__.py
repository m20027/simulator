"""modification of RLlib's MARWIL for RNN policy
"""
from ASRCAISim1.addons.rayUtility.extension.agents.marwil.rnn_bc import RNNBCTrainer, BC_DEFAULT_CONFIG
from ASRCAISim1.addons.rayUtility.extension.agents.marwil.rnn_marwil import RNNMARWILTrainer, DEFAULT_CONFIG
from ASRCAISim1.addons.rayUtility.extension.agents.marwil.rnn_marwil_tf_policy import RNNMARWILTFPolicy
from ASRCAISim1.addons.rayUtility.extension.agents.marwil.rnn_marwil_torch_policy import RNNMARWILTorchPolicy

__all__ = [
    "RNNBCTrainer",
    "BC_DEFAULT_CONFIG",
    "DEFAULT_CONFIG",
    "RNNMARWILTFPolicy",
    "RNNMARWILTorchPolicy",
    "RNNMARWILTrainer",
]
