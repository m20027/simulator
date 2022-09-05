"""Behavioral Cloning (derived from MARWIL) is also extended for RNN Policy.
1. Adds RNN states into SampleBatch as postprocess.
2. Enables burn-in for replay.
"""
from ASRCAISim1.addons.rayUtility.extension.agents.marwil.rnn_marwil import RNNMARWILTrainer, \
    DEFAULT_CONFIG as RNNMARWIL_CONFIG
from ray.rllib.utils.typing import TrainerConfigDict

# yapf: disable
# __sphinx_doc_begin__
BC_DEFAULT_CONFIG = RNNMARWILTrainer.merge_trainer_configs(
    RNNMARWIL_CONFIG, {
        # No need to calculate advantages (or do anything else with the
        # rewards).
        "beta": 0.0,
        # Advantages (calculated during postprocessing) not important for
        # behavioral cloning.
        "postprocess_inputs": False,
        # No reward estimation.
        "input_evaluation": [],
    })
# __sphinx_doc_end__
# yapf: enable


def validate_config(config: TrainerConfigDict) -> None:
    if config["beta"] != 0.0:
        raise ValueError(
            "For behavioral cloning, `beta` parameter must be 0.0!")


RNNBCTrainer = RNNMARWILTrainer.with_updates(
    name="RNNBC",
    default_config=BC_DEFAULT_CONFIG,
    validate_config=validate_config,
)
