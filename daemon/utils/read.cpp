#include "utils.hpp"
#include <fstream>

std::string utils::read_block(Block block) {
  std::ifstream file("memory", std::ios::binary);
  file.seekg(block.start);
  char block_str[BLK_SIZE + 1] = {0};
  file.read(block_str, BLK_SIZE);
  file.close();
  return block_str;
}

utils::Response utils::read_file(const char *filename, FATData &data,
                                 bool do_dir_check) {
  utils::Response response;
  auto file = data.files.find(filename);

  if (file == data.files.end()) { // нет такого файла
    response.status = READ_NO_EXISTING_FILE;
    return response;
  }

  if (do_dir_check && file->second.type != FileType::FILE) {
    response.status = READ_DIR_ERR;
    return response;
  }

  const FileInfo &fileInfo = file->second;
  for (Block block : fileInfo.data) {
    response.result += read_block(block);
  }
  return response;
}
