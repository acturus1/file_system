#! /bin/bash

# g++ fs.cpp -o fs ; cat test/i_1.txt | ./fs write new_file ; rm fs

if [ "$#" -eq 1 ] && [ "$1" = "d" ] ; then
  g++ fs_daemon.cpp -g -o server.out 
  g++ client.cpp -o client.out 
  alacritty -e bash -c "./client.out ; exit" &
  gdb server.out
else
  g++ fs_daemon.cpp -o server.out 
  g++ client.cpp -o client.out 
  alacritty -e bash -c "./client.out ; exit" &
  ./server.out
fi
