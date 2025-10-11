

DEBUG=0
CLEAN=0

[[ "$*" == *d* ]] && DEBUG=1
[[ "$*" == *c* ]] && CLEAN=1

cd daemon
if (( DEBUG )); then
  g++ fs_daemon.cpp -o ../server.out -g
else
  g++ fs_daemon.cpp -o ../server.out
fi
cd ..

cd client
g++ client.cpp -o ../client.out
cd ..

cleanup() {
    pkill -f "watch bat" 2>/dev/null
    pkill -f "./server.out" 2>/dev/null
    pkill -P $$ alacritty 2>/dev/null
    (( CLEAN )) && rm -f client.out server.out FAT memory
    exit
}

trap cleanup SIGINT SIGTERM EXIT

i3-msg split v
alacritty -e watch bat -A memory &
sleep 0.3

i3-msg split v
alacritty -e watch bat -A FAT &
sleep 0.3

i3-msg focus down
i3-msg split h

if (( DEBUG )); then
    # alacritty -e gdb -ex run ./server.out &
    alacritty -e gdb ./server.out &
else
    ./server.out &
fi
SERVER_PID=$!

sleep 1

alacritty -e ./client.out &
CLIENT_PID=$!

if (( DEBUG )); then
    wait $CLIENT_PID
    echo "Нажмите Enter для выхода..."
    read
fi

wait
