dd if=/dev/zero bs=1 of=test_memory count=1024 status=none ;
touch FAT ; 
g++ ../../daemon/fs_daemon.cpp ../../tests/daemon/constants.cpp ../../daemon/utils/*.cpp ../../tests/daemon/tests.cpp -lgtest -lgtest_main -pthread -o example_test ; 
./example_test
