#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "constants.cpp"

#include "model/fat_data.hpp"

void get_file_name(std::string &line, size_t &pos, FileInfo &file) {
  file.name = line.substr(0, pos);
  pos++;
}

void get_file_type(std::string &line, size_t &pos, FileInfo &file) {
  char type_char = line.substr(pos, pos + 1)[0];
  switch (type_char) {
  case 'f':
    file.type = FileType::FILE;
    break;
  case 'd':
    file.type = FileType::DIR;
    break;
  default:
    puts(file.name.c_str());
    puts("Unknown file type while reading FAT");
    break;
  }
  pos += 2;
}

void print_block(std::ofstream &FAT, Block &block) {
  FAT << "[" << block.start << "-" << block.end << "]";
}

void print_file_info(std::ofstream &FAT, FileInfo &file_info) {
  char type = file_info.type == FileType::FILE ? 'f' : 'd';
  FAT << file_info.name << ";" << type << ";";
  for (Block block : file_info.data) {
    print_block(FAT, block);
  }
  FAT << "\n";
}

void read_FAT_free_memory_info_from_disk(FATData &result, std::ifstream &FAT) {
  std::string line;
  if (std::getline(FAT, line)) {
    std::istringstream iss(line);
    char delim;

    if (!(iss >> result.start_free_memory)) {
      throw std::runtime_error("Invalid format: expected total blocks count");
    }

    iss >> delim;
    if (delim != ',') {
      throw std::runtime_error(
          "Invalid format: expected comma after total blocks");
    }

    std::string range;
    while (std::getline(iss, range, ',')) {
      range.erase(0, range.find_first_not_of(" \t"));
      if (range.empty())
        continue;

      if (range.front() == '[' && range.back() == ']') {
        range = range.substr(1, range.size() - 2);
        size_t dash_pos = range.find('-');
        if (dash_pos != std::string::npos) {
          Block block;
          block.start = std::stoll(range.substr(0, dash_pos));
          block.end = std::stoll(range.substr(dash_pos + 1));
          result.empty_blocks.push_back(block);
        }
      }
    }
  }
}

void read_FAT_files_info_from_disk(FATData &result, std::ifstream &FAT) {
  std::string line;
  while (std::getline(FAT, line)) {
    size_t pos = line.find(';');
    if (pos == std::string::npos)
      continue;

    FileInfo file;
    get_file_name(line, pos, file);
    get_file_type(line, pos, file);

    while (true) {
      size_t close_br = line.find(']', pos);
      if (close_br == std::string::npos)
        break;

      std::string range = line.substr(pos + 1, close_br - pos - 1);
      size_t dash_pos = range.find('-');
      if (dash_pos != std::string::npos) {
        Block block;
        block.start = std::stoll(range.substr(0, dash_pos));
        block.end = std::stoll(range.substr(dash_pos + 1));
        file.data.push_back(block);
      }
      pos = close_br + 1;
      if (pos < line.size() && line[pos] == ',')
        pos++;
    }

    result.files[file.name] = file;
  }
}

FATData read_FAT_from_disk() {
  FATData result;
  std::ifstream FAT(FAT_PATH);
  if (!FAT) {
    throw std::runtime_error("Failed to open FAT file");
  }

  read_FAT_free_memory_info_from_disk(result, FAT);
  read_FAT_files_info_from_disk(result, FAT);

  return result;
}

void dump_FAT_to_disk(FATData &data) {
  std::ofstream FAT(FAT_PATH);
  if (!FAT) {
    throw std::runtime_error("Failed to open FAT file");
  }

  FAT << data.start_free_memory << ",";
  for (Block empty_block : data.empty_blocks) {
    print_block(FAT, empty_block);
  }

  FAT << "\n";

  for (auto &[filename, file_info] : data.files) {
    print_file_info(FAT, file_info);
  }
}

void debug_print_FAT(FATData &data) {
  std::cout << data.start_free_memory << ",";
  for (Block empty_block : data.empty_blocks) {
    std::cout << "[" << empty_block.start << "-" << empty_block.end << "]";
  }

  std::cout << "\n";

  for (auto &[filename, file_info] : data.files) {
    char type = file_info.type == FileType::FILE ? 'f' : 'd';
    std::cout << file_info.name << ";" << type << ";";
    for (Block block : file_info.data) {
      std::cout << "[" << block.start << "-" << block.end << "]";
    }
    std::cout << "\n";
  }
}
