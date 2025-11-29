dd if=/dev/zero bs=1 of=test_memory count=1024 ;
touch FAT ; 
g++ daemon/fs_daemon.cpp tests/daemon/constants.cpp daemon/utils/read.cpp tests/daemon/write_test.cpp -lgtest -lgtest_main -pthread -o example_test ; 
./example_test
