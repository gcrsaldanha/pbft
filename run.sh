#!/bin/bash
mpicc -o main.out main.c && mpirun -np $1 main.out | grep -Fw "[0]"
