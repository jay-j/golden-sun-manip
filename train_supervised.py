#!/usr/bin/env python3
# Trains a state->action network based on pre-recorded data
# Use a venv that installs tinygrad, with a few dependencies

from tinygrad.tensor import Tensor


# Use a RNN; has hidden state based on history of inputs
# Use a reinforcement-learning approach; two networks, one to calculate command, one to calculate value of current state
# w_ designates a weights matrix


class GSBot:
    
    
    # init with some size information? fresh variables or load?
    def __init__(self, size):
        

        # initialize with the zero vector
        state_hidden = Tensor.zero(size)
    

        # hidden to hidden. TRAINABLE
        w_hh = Tensor.randn(size)
        # state to hidden. TRAINABLE
        w_xh = Tensor.randn(size)
        
        

    # Command Network
    def network_command(state_input):
        """Command Network
           Input: the state
           Contains: hidden RNN layers
           Output: which button to press now
        
           Useage: when 
        """

        # update the hidden state
        self.state_hidden = 
        
        # compute the output vector, based on the hidden state

        
        
    




