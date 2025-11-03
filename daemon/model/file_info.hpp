#include <string>
#include <vector>

#include "block.hpp"

#ifndef FILE_TYPE_MODEL_HPP
#define FILE_TYPE_MODEL_HPP

enum class FileType { FILE, DIR };

#endif

#ifndef FILE_INFO_MODEL_HPP
#define FILE_INFO_MODEL_HPP

class FileInfo {
public:
  std::string name;
  std::vector<Block> data;
  FileType type = FileType::FILE;
};

#endif
