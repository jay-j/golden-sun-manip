#!/usr/bin/env python3
# manually play the game, via a zmq connection
# does not need to be run as sudo
# check your IP address


import time
import zmq
import pygame

# init pygame stuff
pygame.init()
window = pygame.display.set_mode((450, 60))
clock = pygame.time.Clock()

# init zmq comms
context = zmq.Context()
socket = context.socket(zmq.PUSH)
# socket.connect("tcp://192.168.50.119:5555")
socket.connect("tcp://192.168.50.207:5555")
print("connection opened")


# button / control scheme setup
def encode_button(buttons, value, index):
    buttons += (value << index)
    return buttons
buttonlist = {pygame.K_w:0, pygame.K_s:1, pygame.K_a:2, pygame.K_d:3, pygame.K_k:4, pygame.K_j:5, pygame.K_n:6, pygame.K_m:7, pygame.K_q:8, pygame.K_e:9, pygame.K_t:10, "anything":11}

run = True
while run:

    message = bytearray([0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0])

    events = pygame.event.get()
    anything_pushed = False

    for event in events:
        if event.type == pygame.QUIT:
            run = False 

        if event.type == pygame.KEYDOWN:
            anything_pushed = True
            # lookup dictionary, mark bits
            if event.key in buttonlist.keys():
                message[buttonlist[event.key]] = 1
        if event.type == pygame.KEYUP:
            if event.key in buttonlist.keys():
                message[buttonlist[event.key]] = 2

    if anything_pushed:
        message[buttonlist["anything"]] = 1
        print(f"New push, send message: {message}")

    socket.send(message)

    dt = clock.tick(30) # 30 = 30fps

pygame.quit()    
