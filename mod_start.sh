#!/bin/bash

g++ fs_daemon.cpp -o server.out
g++ client.cpp -o client.out

cleanup() {
    pkill -f "watch bat -A memory"
    pkill -f "watch bat -A FAT"
    pkill -f "./server.out"
    pkill -P $$ alacritty 2>/dev/null || true
    rm client.out 
    rm server.out
    rm FAT
    rm memory
    exit 0
}

dd if=/dev/zero bs=1 of=memory count=1024
touch FAT

trap cleanup SIGINT SIGTERM EXIT
i3-msg split v

alacritty -e bash -c "trap 'exit 0' SIGINT SIGTERM; watch bat -A memory" &
MEMORY_PID=$!
sleep 0.3
i3-msg split v
alacritty -e bash -c "trap 'exit 0' SIGINT SIGTERM; watch bat -A FAT" &
FAT_PID=$!
sleep 0.3

i3-msg focus down
i3-msg split h

./server.out &
SERVER_PID=$!

echo "🚀 Запуск клиента..."
alacritty -e bash -c "./client.out; echo 'Клиент завершен'; sleep 2" &
CLIENT_PID=$!
sleep 0.3
i3-msg move right

wait $CLIENT_PID

cleanup
