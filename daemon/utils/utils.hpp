#include "../model/fat_data.hpp"
#include <string>

#include "../constants.hpp"

const int OK = 0;

// READ FILE
const int READ_DIR_ERR = 1;
const int READ_NO_EXISTING_FILE = 2;

// LIST FILES
const int LS_NO_EXISTING_DIR = 3;

namespace utils {
struct Response {
  int status;
  std::string result;
};

std::string read_block(Block block);

Response read_file(const char *filename, FATData &data,
                   bool do_dir_check = true);

Response list_files(const char *filepath, FATData &data);

} // namespace utils
