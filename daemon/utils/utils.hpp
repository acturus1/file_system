#include "../model/fat_data.hpp"
#include <map>
#include <string>
#include <vector>

#include "../constants.hpp"

const int OK = 0;

// READ FILE
const int READ_DIR_ERR = 1;
const int READ_NO_EXISTING_FILE = 2;

// LIST FILES
const int LS_NO_EXISTING_DIR = 3;

// TREE FILES

namespace utils {
struct Response {
  int status;
  std::string result;
};

struct ListResponse {
  int status;
  std::vector<std::string> result;
};

struct TreeResponse {
  int status;
  std::map<std::string, std::vector<std::string>> result;
};

std::string read_block(Block block);

Response read_file(const char *filename, FATData &data,
                   bool do_dir_check = true);

ListResponse list_files(const char *filepath, FATData &data);

TreeResponse tree(FATData &data, std::string dirpath);

} // namespace utils
