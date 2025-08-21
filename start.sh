#! /bin/bash

# g++ fs.cpp -o fs ; cat test/i_1.txt | ./fs write new_file ; rm fs

g++ fs_daemon.cpp -o server.out ; g++ client.cpp -o client.out ; alacritty -e bash -c "./client.out ; exit" & ./server.out
