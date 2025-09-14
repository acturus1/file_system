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

enum class FileType { FILE, DIR };

class FileInfo {
public:
  std::string name;
  std::vector<Block> data;
  FileType type = FileType::FILE;
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
  char type = file_info.type == FileType::FILE ? 'f' : 'd';
  FAT << file_info.name << ";" << type << ";";
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
    size_t pos = line.find(';');
    if (pos == std::string::npos)
      continue;

    FileInfo file;
    get_file_name(line, pos, file);
    get_file_type(line, pos, file);

    while (true) {
      size_t close_br = line.find(']', pos);
      if (close_br == std::string::npos) // for empty files
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

void write_file(const char *filename, const char *text, FATData &data,
                FileType filetype = FileType::FILE) {
  if (filename == nullptr || filename[0] == '\0') {
    std::cerr << "Error no filename" << std::endl;
    return;
  }
  std::string whole_input = text;

  int blocks_cnt = whole_input.length() / BLK_SIZE;
  if (whole_input.length() % BLK_SIZE != 0) {
    blocks_cnt++;
  }

  FileInfo &fileinfo = data.files[filename];
  fileinfo.name = filename;
  fileinfo.type = filetype;
  fileinfo.data.clear();

  for (int i = 0; i < blocks_cnt; i++) {
    std::string block_str = whole_input.substr(i * BLK_SIZE, BLK_SIZE);

    if (data.empty_blocks.size() > 0) {
      long long start = data.empty_blocks[0].start;
      write_block(block_str, start);

      Block block{start, start + BLK_SIZE - 1};
      fileinfo.data.push_back(block);

      data.empty_blocks.erase(data.empty_blocks.begin());
    } else {
      long long start = data.start_free_memory;
      write_block(block_str, start);

      Block block{start, start + BLK_SIZE - 1};
      fileinfo.data.push_back(block);

      data.start_free_memory += BLK_SIZE;
    }
  }

  write_status_client("OK");
}

int delete_file(const char *filename, FATData &data) {
  if (data.files.find(filename) == data.files.end()) {
    std::string filename_string = filename;
    write_status_client("Файл с именем " + filename_string + " не существует");
    return 1;
  }

  FileInfo file_to_delete = data.files[filename];
  for (Block block : file_to_delete.data) {
    delete_block(block, data);
  }
  data.files.erase(filename);
  return 0;
}

int edit_file(const char *filename, const char *text, FATData &data) {
  if (delete_file(filename, data) != 0) {
    return 1;
  }
  write_file(filename, text, data);
  return 0;
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

std::string read_block(Block block) {
  std::ifstream file("memory", std::ios::binary);
  file.seekg(block.start);
  char block_str[BLK_SIZE + 1] = {0};
  file.read(block_str, BLK_SIZE);
  file.close();
  return block_str;
}

void read_file(const char *filename, FATData &data) {
  std::string result;
  auto file = data.files.find(filename);
  if (file != data.files.end()) {
    const FileInfo &fileInfo = file->second;
    for (Block block : fileInfo.data)
      result += read_block(block);
    write_status_client(result);
  } else {
    std::string filename_str = filename;
    write_status_client("Файл с именем " + filename_str + " не существует");
  }
}

void list_files(FATData &data) {
  if (data.files.empty()) {
    write_status_client("");
    return;
  }
  std::string file_name_list;
  for (auto &[filename, file_info] : data.files)
    file_name_list += filename + " ";
  write_status_client(file_name_list);
}

int main() {
  FATData data = read_FAT_from_disk();

  debug_print_FAT(data);

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
      std::istringstream iss(buffer + 1);
      std::string filename;
      iss >> filename;
      if (buffer[0] == 'w') {
        std::string text;
        iss >> text;
        if (data.files.find(filename) != data.files.end()) {
          write_status_client("Файл с именем " + filename + " уже существует");
        } else {
          write_file(filename.c_str(), text.c_str(), data);
        }
      } else if (buffer[0] == 'd') {
        if (data.files.find(filename) != data.files.end()) {
          write_status_client("Директория с именем " + filename +
                              " уже существует");
        } else {
          write_file(filename.c_str(), "", data, FileType::DIR);
        }
      } else if (buffer[0] == 'x') {
        if (delete_file(filename.c_str(), data) == 0) {
          write_status_client("OK");
        }
      } else if (buffer[0] == 'e') {
        std::string text;
        iss >> text;
        if (edit_file(filename.c_str(), text.c_str(), data) == 0) {
          write_status_client("OK");
        }
      } else if (buffer[0] == 'r') {
        read_file(filename.c_str(), data);
      } else if (buffer[0] == 'l') {
        list_files(data);
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
