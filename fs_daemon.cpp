#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

const int BLK_SIZE = 2;
const char *FAT_PATH = "FAT";
const char *fifo_path = "./myfifo";
const char *fifo_path_client = "./server_fifo";

class Block {
public:
  long long start;
  long long end;
};

class FileInfo {
public:
  std::string name;
  std::vector<Block> data;
};

struct FATData {
  long long start_free_memory;
  std::vector<Block> empty_blocks;
  std::map<std::string, FileInfo> files;
};

void print_block(std::ofstream &FAT, Block &block) {
  FAT << "[" << block.start << "-" << block.end << "]";
}

void print_file_info(std::ofstream &FAT, FileInfo &file_info) {
  FAT << file_info.name;
  for (Block block : file_info.data) {
    print_block(FAT, block);
  }
  FAT << "\n";
}

void create_fifos() {
  mkfifo(fifo_path, 0666);
  mkfifo(fifo_path_client, 0666);
  std::cout << "FIFO созданы: " << fifo_path << " и " << fifo_path_client
            << std::endl;
}

void write_status_client(const std::string &message) {
  int fd = open(fifo_path_client, O_WRONLY);
  if (fd == -1) {
    perror("Ошибка открытия FIFO для записи клиенту");
    return;
  }

  std::string msg = message + '\n';
  write(fd, msg.c_str(), msg.length());
  close(fd);
}

FATData read_FAT_from_disk() {
  FATData result;
  std::ifstream FAT(FAT_PATH);
  if (!FAT) {
    throw std::runtime_error("Failed to open FAT file");
  }

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

void delete_block(Block &block, FATData &data) {
  data.empty_blocks.push_back(block);
}

void write_block(std::string block, long long pos_start) {
  std::cout << "Block: " << block << "\n";

  std::fstream file;
  file.open("memory", std::ios::in | std::ios::out | std::ios::binary);
  file.seekp(pos_start);
  file.write(block.c_str(), BLK_SIZE);
  file.close();
}

void write_file(const char *filename, const char *text, FATData &data) {
  if (filename == nullptr || filename[0] == '\0') {
    std::cerr << "Error no filename" << std::endl;
    return;
  }
  std::string whole_input = text;

  int blocks_cnt = whole_input.length() / BLK_SIZE;
  if (whole_input.length() % BLK_SIZE != 0) {
    blocks_cnt++;
  }

  long long current_pos = data.start_free_memory;
  FileInfo &fileinfo = data.files[filename];
  fileinfo.name = filename;
  fileinfo.data.clear();

  for (int i = 0; i < blocks_cnt; ++i) {
    std::string block_str = whole_input.substr(i * BLK_SIZE, BLK_SIZE);

    write_block(block_str, current_pos);

    Block block;
    block.start = current_pos;
    block.end = current_pos + BLK_SIZE - 1;
    fileinfo.data.push_back(block);

    current_pos += BLK_SIZE;
  }

  data.start_free_memory = current_pos;
  write_status_client("OK");
}

int delete_file(std::string &filename, FATData &data) {
  if (data.files.find(filename) == data.files.end()) {
    write_status_client("Файл с именем " + filename + " не существует");
    return 1;
  }
  FileInfo file_to_delete = data.files[filename];
  for (Block block : file_to_delete.data) {
    delete_block(block, data);
  }

  data.files.erase(filename);

  write_status_client("OK");
  return 0;
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

int main() {
  FATData data = read_FAT_from_disk();

  create_fifos();

  int fd = open(fifo_path, O_RDONLY);
  char buffer[1024];
  while (true) {
    ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    if (bytes_read == -1) {
      perror("Ошибка чтения");
      break;
    } else if (bytes_read == 0) {
      std::cout << "Клиент отключился" << std::endl;
      break;
    } else {
      buffer[bytes_read] = '\0';
      std::cout << "Получено от клиента: " << buffer;
      if (buffer[0] == 'w') {
        std::istringstream iss(buffer + 1);
        std::string filename;
        iss >> filename;
        std::string text;
        iss >> text;
        std::getline(iss, text);
        if (!text.empty())
          text.erase(0, text.find_first_not_of(' '));
        if (data.files.find(filename) != data.files.end()) {
          write_status_client("Файл с именем " + filename + " уже существует");
        } else {
          write_file(filename.c_str(), text.c_str(), data);
        }
      } else if (buffer[0] == 'd') {
        std::istringstream iss(buffer + 1);
        std::string filename;
        iss >> filename;
        delete_file(filename, data);
      }
      if (buffer[bytes_read - 1] != '\n') {
        std::cout << std::endl;
      }
    }
  }

  close(fd);
  unlink(fifo_path);
  unlink(fifo_path_client);
  std::cout << "Сервер завершает работу" << std::endl;

  dump_FAT_to_disk(data);

  return 0;
}
