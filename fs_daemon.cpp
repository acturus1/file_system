#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

class Block {
public:
  long long start;
  long long end;
};

std::ostream &operator<<(std::ostream &os, const Block &block) {
  os << "[" << block.start << "-" << block.end << "]";
  return os;
}

class FileInfo {
public:
  std::string name;
  std::vector<Block> data;
};

struct FATData {
  long long total_blocks;
  std::vector<Block> empty_blocks;
  std::map<std::string, FileInfo> files;
};

FATData read_FAT_from_disk() {
  FATData result;
  std::ifstream FAT("FAT");
  if (!FAT) {
    throw std::runtime_error("Failed to open FAT file");
  }

  std::string line;
  if (std::getline(FAT, line)) {
    std::istringstream iss(line);
    char delim;

    if (!(iss >> result.total_blocks)) {
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

  while (std::getline(FAT, line)) {
    size_t open_br = line.find('[');
    if (open_br == std::string::npos)
      continue;

    FileInfo file;
    file.name = line.substr(0, open_br);

    size_t pos = open_br;
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

  return result;
}

int main() {
  FATData data = read_FAT_from_disk();

  std::cout << "Общее количество блоков: " << data.total_blocks << std::endl;

  if (!data.empty_blocks.empty()) {
    std::cout << "Первый свободный блок: " << data.empty_blocks[0] << std::endl;
  } else {
    std::cout << "Нет свободных блоков!" << std::endl;
  }

  for (const auto &[name, file] : data.files) {
    std::cout << "Файл: " << name << " | Блоки: ";
    for (const Block &block : file.data) {
      std::cout << block << " ";
    }
    std::cout << std::endl;
  }

  return 0;
}
