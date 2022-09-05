"""extention of RLlib's MARWIL for RNN policy
1. Adds RNN states into SampleBatch as postprocess.
2. Enables burn-in for replay.
"""
from typing import Optional, Type

from ray.rllib.agents import marwil
from ASRCAISim1.addons.rayUtility.extension.agents.marwil.rnn_marwil_tf_policy import RNNMARWILTFPolicy
from ray.rllib.execution.replay_ops import Replay, StoreToReplayBuffer
from ray.rllib.execution.replay_buffer import LocalReplayBuffer
from ray.rllib.execution.rollout_ops import ParallelRollouts, ConcatBatches
from ray.rllib.execution.concurrency_ops import Concurrently
from ray.rllib.execution.train_ops import TrainOneStep
from ray.rllib.execution.metric_ops import StandardMetricsReporting
from ray.rllib.utils.typing import TrainerConfigDict
from ray.rllib.evaluation.worker_set import WorkerSet
from ray.util.iter import LocalIterator
from ray.rllib.policy.policy import Policy

DEFAULT_CONFIG = marwil.MARWILTrainer.merge_trainer_configs(
    marwil.DEFAULT_CONFIG,
    {
        #additional flags for burn-in
        "replay_burn_in": 0,
        "replay_zero_init_states": True,
    },
    _allow_unknown_configs=True,
)

def get_policy_class(config: TrainerConfigDict) -> Optional[Type[Policy]]:
    """Policy class picker function. Class is chosen based on DL-framework.
    MARWIL/BC have both TF and Torch policy support.

    Args:
        config (TrainerConfigDict): The trainer's configuration dict.

    Returns:
        Optional[Type[Policy]]: The Policy class to use with DQNTrainer.
            If None, use `default_policy` provided in build_trainer().
    """
    if config["framework"] == "torch":
        from ASRCAISim1.addons.rayUtility.extension.agents.marwil.rnn_marwil_torch_policy import \
            RNNMARWILTorchPolicy
        return RNNMARWILTorchPolicy


def execution_plan(workers: WorkerSet, config: TrainerConfigDict,
                   **kwargs) -> LocalIterator[dict]:
    """Execution plan of the MARWIL/BC algorithm. Defines the distributed
    dataflow.

    Args:
        workers (WorkerSet): The WorkerSet for training the Polic(y/ies)
            of the Trainer.
        config (TrainerConfigDict): The trainer's configuration dict.

    Returns:
        LocalIterator[dict]: A local iterator over training metrics.
    """
    assert len(kwargs) == 0, (
        "Marwill execution_plan does NOT take any additional parameters")

    rollouts = ParallelRollouts(workers, mode="bulk_sync")
    if(config["replay_buffer_size"]>0):
        replay_sequence_length=max(config.get("replay_sequence_length", 1),config["model"].get("max_seq_len",1)+config.get("replay_burn_in",0))
        replay_buffer = LocalReplayBuffer(
            learning_starts=config["learning_starts"],
            buffer_size=config["replay_buffer_size"],
            replay_batch_size=config["train_batch_size"],
            replay_sequence_length=replay_sequence_length,
            replay_burn_in=config["replay_burn_in"],
            replay_zero_init_states=config["replay_zero_init_states"]
        )

        store_op = rollouts \
            .for_each(StoreToReplayBuffer(local_buffer=replay_buffer))

        replay_op = Replay(local_buffer=replay_buffer) \
            .combine(
                ConcatBatches(
                    min_batch_size=config["train_batch_size"],
                    count_steps_by=config["multiagent"]["count_steps_by"],
                )) \
            .for_each(TrainOneStep(workers))

        train_op = Concurrently(
            [store_op, replay_op], mode="round_robin", output_indexes=[1])
    else:
        train_op = rollouts \
            .combine(
                ConcatBatches(
                    min_batch_size=config["train_batch_size"],
                    count_steps_by=config["multiagent"]["count_steps_by"],
                )) \
            .for_each(TrainOneStep(workers))

    return StandardMetricsReporting(train_op, workers, config)

RNNMARWILTrainer = marwil.MARWILTrainer.with_updates(
    name="RNNMARWIL",
    default_config=DEFAULT_CONFIG,
    default_policy=RNNMARWILTFPolicy,
    get_policy_class=get_policy_class,
    execution_plan=execution_plan)