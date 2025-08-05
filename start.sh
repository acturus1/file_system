#! /bin/bash

g++ fs.cpp -o fs ; cat test/i_1.txt | ./fs write new_file ; rm fs
