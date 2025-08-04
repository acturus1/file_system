#! /bin/bash

g++ fs.cpp -o fs ; cat test/i_1.txt | ./fs write ; rm fs
