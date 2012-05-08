#!/bin/sh
g++ `pkg-config --cflags opencv` camera.cpp -o camera `pkg-config --libs opencv`
