#!/usr/bin/env python3

import argparse
import sys
import matplotlib.pyplot as plt
from math import ceil, e
import numpy as np
<<<<<<< HEAD
import re

_HEIGHT = 60
_WIDTH = 120

_LAST_REF = []

def _draw_group(g, time):
    global _LAST_REF
    lines = g.strip().split('\n')
    layers = []
    for l in range(5):
        layers.append([float(i) for i in lines[l].split()[:-1]])

    if len(_LAST_REF) == len(layers[4]) and all([x == y for x, y in zip(layers[4], _LAST_REF)]):
        return
    _LAST_REF = layers[4][:]

    _WIDTH = max([len(l) for l in layers])
    YALL = np.arange(0, _HEIGHT, _HEIGHT / len(layers))
    color = 'go'
    scales = []
    for i, layer in enumerate(layers):
        scale = 10/max([abs(i) for i in layer])
        scales.append(scale)
=======

def _draw_norm(sects, axs):
    midx = int(sects[_MIDX])
    mref = int(sects[_MREF])
    mweight = e**-float(sects[_MW])
    means = [float(x) for x in sects[_MR][1:-1].split(',')[:-1]]
    vref = int(sects[_VREF])
    vidx = int(sects[_VIDX])
    vs = [float(x) for x in sects[_VR][1:-1].split(',')[:-1]]
    color = 'go'
    if mref == 0:
        color = 'ro'
    x = means[0];
    y = means[1];
    axs.plot(x, y, color, alpha=0.3, markersize=20*mweight+0.1)
    axs.text(x, y, f'{midx}')
    normed1 = 0
    if vs[0] > 0:
        normed1 = 1/vs[0]**0.5
    normed2 = 0
    if vs[1] > 0:
        normed2 = 1/vs[1]**0.5
    a = [-normed1 + x, normed1 + x]
    b = [-normed2 + y, normed2 + y]
    vcolor = 'g'
    if mref == 0:
        vcolor = 'r'
    axs.plot(a, [y, y], vcolor, markersize=0.2, alpha=0.5)
    axs.plot([x, x], b, vcolor, markersize=0.2, alpha=0.5)

_HEIGHT = 60
_WIDTH = 200

def _draw_group(g, epoch):
    lines = g.strip().split('\n')
    layers = []
    for l in range(4):
        layers.append([float(i) for i in lines[l].split(' ')[:-1]])

    print([len(l) for l in layers])
    YALL = np.arange(0, _HEIGHT, _HEIGHT / len(layers))
    color = 'go'
    for i, layer in enumerate(layers):
        scale = 1
        if (i == len(layers)-1):
            scale = 10
>>>>>>> exercise3-zrz
        X = np.arange(0, _WIDTH, _WIDTH / len(layer))
        print(len(X))
        Y = np.zeros((len(X)))
        Y.fill(YALL[i])
        plt.plot([X[0], X[-1]], [Y[0], Y[-1]], markersize=0.1, alpha=0.2)
        for j, v in enumerate(layer):
            dv = np.sign(v) * np.log(np.abs(v))
            plt.plot([X[j], X[j]], [Y[j], Y[j] + scale*v], markersize=0.3)

<<<<<<< HEAD
    plt.title(f'Time {time}, Scales {scales}')
    margin = [_WIDTH/20, _HEIGHT/20]
    plt.axis([-margin[0], _WIDTH+margin[0], -margin[1] - 10, _HEIGHT - margin[1]])
    plt.show()

def _draw_seq_single(ax, seq):
    _WIDTH = len(seq)
    _HEIGHT = len(seq[0])
    X = np.arange(0, _WIDTH, _WIDTH / len(seq))
    Y = np.arange(0, _HEIGHT, _HEIGHT / len(seq[0]))
    scatter = np.array((1, 0))
    for row in seq:
        scatter = np.concatenate((scatter, row))
    print(len(scatter))

    xx, yy = np.meshgrid(X, Y, indexing='ij')
    print(len(yy[0]) * len(yy))
    ax.scatter(xx, yy, s=scatter[:-2] * 10)
    #  for i, x in enumerate(X):
        #  for j, y in enumerate(Y):
            #  ax.plot(x, y)

def _draw_seq(seq):
    groups = seq.split('==')[:-1]
    outputs = []
    targets = []
    for i, g in enumerate(groups):
        lines = g.strip().split('\n')
        outputs.append(np.array([float(i) for i in lines[3].split()[:-1]]))
        targets.append(np.array([float(i) for i in lines[4].split()[:-1]]))
    fig, axs = plt.subplots(2, 1)
    _draw_seq_single(axs[0], outputs)
    _draw_seq_single(axs[1], targets)
=======
    plt.title(f'Epoch {epoch}')
    plt.axis([-1, _WIDTH+1, -20, _HEIGHT - 10])
>>>>>>> exercise3-zrz
    plt.show()

def _main():
    parser = argparse.ArgumentParser(prog = 'amscore_plot', description = 'plots linear segmentation')
    parser.add_argument('file')
    args = parser.parse_args()
    with open(args.file, 'r') as f:
        file = f.read()
<<<<<<< HEAD
    _draw_seq(file)
    #  groups = file.split('==')[:-1]
    #  for i, group in enumerate(groups[:]):
        #  _draw_group(group, i)
=======
    groups = file.split('==')[1:]
    for i, group in enumerate(groups[:]):
        _draw_group(group, i)
>>>>>>> exercise3-zrz

if __name__ == '__main__':
    _main()
