"""extention of RLlib's MARWIL for RNN policy
1. Adds RNN states into SampleBatch as postprocess.
2. Enables burn-in for replay.
"""
import numpy as np
from typing import Optional, Dict

import ray
from ray.rllib.agents.marwil.marwil_tf_policy import postprocess_advantages
from ray.rllib.agents.marwil.marwil_tf_policy import MARWILTFPolicy
from ray.rllib.policy.sample_batch import SampleBatch
from ray.rllib.policy.policy import Policy
from ray.rllib.utils.typing import PolicyID
import ASRCAISim1

def postprocess_for_rnn(
        policy: Policy,
        sample_batch: SampleBatch,
        other_agent_batches: Optional[Dict[PolicyID, SampleBatch]] = None,
        episode=None) -> SampleBatch:
    """Add seq_lens and states into the offline trajectory data without RNN states.

    Args:
        policy (Policy): The Policy used to generate the trajectory
            (`sample_batch`)
        sample_batch (SampleBatch): The SampleBatch to postprocess.
        other_agent_batches (Optional[Dict[PolicyID, SampleBatch]]): Optional
            dict of AgentIDs mapping to other agents' trajectory data (from the
            same episode). NOTE: The other agents use the same policy.
        episode (Optional[MultiAgentEpisode]): Optional multi-agent episode
            object in which the agents operated.

    Returns:
        SampleBatch: The postprocessed, modified SampleBatch (or a new one).
    """
    if policy.is_recurrent():
        states=policy.get_initial_state()
        len_batch=len(sample_batch[SampleBatch.CUR_OBS])
        for i in range(policy.num_state_tensors()):
            sample_batch["state_in_{}".format(i)]=np.zeros_like(states[i],shape=[len_batch]+list(states[i].shape))
            sample_batch["state_out_{}".format(i)]=np.zeros_like(states[i],shape=[len_batch]+list(states[i].shape))
            sample_batch["state_in_{}".format(i)][0]=states[i]
        for t in range(len_batch):
            _,state_outs,_=policy.compute_single_action(
                sample_batch[SampleBatch.CUR_OBS][t],
                states,
                prev_action = sample_batch[SampleBatch.PREV_ACTIONS][t],
                prev_reward = sample_batch[SampleBatch.PREV_REWARDS][t],
                explore = False
            )
            for i in range(policy.num_state_tensors()):
                states[i]=state_outs[i]
                if(t<len_batch-1):
                    sample_batch["state_in_{}".format(i)][t+1]=states[i]
                sample_batch["state_out_{}".format(i)][t]=states[i]
        seq_lens=[]
        max_seq_len=policy.config["model"]["max_seq_len"]
        count=sample_batch.count
        while count>0:
            seq_lens.append(min(count,max_seq_len))
            count-=max_seq_len
        if ray.__version__ >= "1.4.0":
            return SampleBatch(sample_batch,seq_lens=seq_lens)
        else:
            ret=SampleBatch(**sample_batch,_seq_lens=np.array(seq_lens,dtype=np.int32),_dont_check_lens=True)
            return ret
    else:
        return sample_batch
    
def postprocess_all(policy,
                           sample_batch,
                           other_agent_batches=None,
                           episode=None):
        sample_batch=postprocess_for_rnn(policy,sample_batch,other_agent_batches,episode)
        return postprocess_advantages(policy,sample_batch,other_agent_batches,episode)

RNNMARWILTFPolicy = MARWILTFPolicy.with_updates(
    name="RNNMARWILTFPolicy",
    get_default_config=lambda: ASRCAISim1.addons.rayUtility.extension.agents.marwil.rnn_marwil.DEFAULT_CONFIG,
    postprocess_fn=postprocess_all
)
