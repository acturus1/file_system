#include <map>
#include <string>
#include <vector>

#include "block.hpp"
#include "file_info.hpp"

#ifndef FAT_DATA_MODEL
#define FAT_DATA_MODEL

struct FATData {
  long long start_free_memory = 0;
  std::vector<Block> empty_blocks;
  std::map<std::string, FileInfo> files;
};

#endif
