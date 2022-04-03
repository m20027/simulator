# Copyright (c) 2020 DeNA Co., Ltd.
# Licensed under The MIT License [see LICENSE for details]

# episode generation

import random
import bz2
import pickle

import numpy as np

from .util import softmax


class Generator:
    def __init__(self, env, args):
        self.env = env
        self.args = args

    def generate(self, models, args):
        # episode generation
        moments = []
        hidden = {}
        for player in self.env.players():
            hidden[player] = models[player].init_hidden()

        err = self.env.reset()
        if err:
            return None

        basename_list = ['turn', 'pitch', 'accel', 'fire']
        policy_list = ['turn_p', 'pitch_p', 'accel_p', 'fire_p']
        action_list = ['turn_action', 'pitch_action',
                       'accel_action', 'fire_action']
        while not self.env.terminal():
            moment_keys = ['observation', 'policy', 'turn_action_mask', 'pitch_action_mask', 'accel_action_mask', 'fire_action_mask',
                           'value', 'reward', 'return', 'action'] + policy_list + action_list
            moment = {key: {p: None for p in self.env.players()}
                      for key in moment_keys}

            turn_players = self.env.turns()
            for player in self.env.players():
                if player in turn_players or self.args['observation']:
                    obs = self.env.observation(player)
                    model = models[player]
                    outputs = model.inference(obs, hidden[player])
                    hidden[player] = outputs.get('hidden', None)
                    v = outputs.get('value', None)

                    moment['observation'][player] = obs
                    moment['value'][player] = v

                    if player in turn_players:
                        legal_actions = list(self.env.legal_actions(player))
                        for legal_action, policy_column, basename in zip(legal_actions, policy_list, basename_list):
                            p_ = outputs[policy_column]
                            action_mask = np.ones_like(p_) * 1e32
                            legal_actions_per_policy = range(legal_action.n)
                            action_mask[legal_actions_per_policy] = 0
                            p = p_ - action_mask

                            if args["determistics"][player]:
                                action = np.argmax(p[:])
                            else:
                                action = random.choices(
                                    legal_actions_per_policy, weights=softmax(p[:]))[0]

                            moment[policy_column][player] = p
                            moment[basename + '_action_mask'][player] = action_mask
                            #print (policy_column, action_mask, p)
                            moment[basename + "_action"][player] = action
                        moment['action'][player] = [
                            moment[action][player] for action in action_list]

            err = self.env.step(moment['action'])
            if err:
                return None

            reward = self.env.reward()
            for player in self.env.players():
                moment['reward'][player] = reward.get(player, None)
            moment['turn'] = turn_players
            moments.append(moment)
        if len(moments) < 1:
            return None
        for player in self.env.players():
            ret = 0
            for i, m in reversed(list(enumerate(moments))):
                ret = (m['reward'][player] or 0) + self.args['gamma'] * ret
                moments[i]['return'][player] = ret
        episode = {
            'args': args, 'steps': len(moments),
            'outcome': self.env.outcome(),
            'moment': [
                bz2.compress(pickle.dumps(
                    moments[i:i+self.args['compress_steps']]))
                for i in range(0, len(moments), self.args['compress_steps'])
            ]
        }

        return episode

    def execute(self, models, args):
        episode = self.generate(models, args)
        if episode is None:
            print('None episode in generation!')
        return episode
