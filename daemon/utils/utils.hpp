#include "../model/fat_data.hpp"
#include <string>

#include "../constants.hpp"

const int READ_DIR_ERR = 1;
const int READ_NO_EXISTING_FILE = 2;

namespace utils {
struct Response {
  int status;
  std::string result;
};

std::string read_block(Block block);

Response read_file(const char *filename, FATData &data,
                   bool do_dir_check = true);

} // namespace utils
