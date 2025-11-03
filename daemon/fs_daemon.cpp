#include <cstdlib>
#include <fcntl.h>
#include <filesystem>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include "model/block.hpp"
#include "model/fat_data.hpp"
#include "model/file_info.hpp"

#include "constants.hpp"
#include "fat_funcs.cpp"

#include "utils/utils.hpp"

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

void read_file(const char *filename, FATData &data, bool do_dir_check = true) {
  utils::Response response = utils::read_file(filename, data, do_dir_check);

  if (response.status == READ_DIR_ERR) {
    write_status_client("Ошибка: " + std::string(filename) +
                        " является директорией");
  } else if (response.status == READ_NO_EXISTING_FILE) {
    std::string filename_str = filename;
    write_status_client("Файл с именем " + filename_str + " не существует");
  } else {
    write_status_client(response.result);
  }
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

bool file_already_exists(const std::string &filename, FATData &data,
                         FileType requested_type) {
  auto it = data.files.find(filename);
  if (it != data.files.end()) {
    FileType existing_type = it->second.type;
    if (existing_type != requested_type) {
      std::string existing_type_str =
          (existing_type == FileType::FILE) ? "файл" : "директория";
      std::string requested_type_str =
          (requested_type == FileType::FILE) ? "файл" : "директория";
    }
    return true;
  }
  return false;
}

bool is_valid_name(std::string &filename) {
  if (filename[0] != '/') {
    return false;
  }
  return true;
}

bool all_filepath_unit_exist(FATData &data, std::string filepath) {
  std::string full_unit_path;
  filepath.erase(0, 1); // delete first '/'
  while (filepath.find('/') != filepath.npos) {
    size_t slash_pos = filepath.find('/');
    std::string filepath_unit = filepath.substr(0, slash_pos);
    filepath.erase(0, slash_pos + 1);
    full_unit_path += "/" + filepath_unit;
    if (!file_already_exists(full_unit_path, data, FileType::DIR)) {
      return false;
    }
  }
  return true;
}

void create_directory(const char *dirname, FATData &data) {
  if (dirname == nullptr || dirname[0] == '\0') {
    std::cerr << "Error no directory name" << std::endl;
    write_status_client("Ошибка: не указано имя директории");
    return;
  }

  std::string dirname_str = dirname;
  if (file_already_exists(dirname_str, data, FileType::DIR)) {
    write_status_client(std::string("Ошибка: директория '") + dirname +
                        std::string("' уже существует"));
    return;
  }

  FileInfo &dir_info = data.files[dirname_str];
  dir_info.name = dirname_str;
  dir_info.type = FileType::DIR;
  dir_info.data.clear();

  write_status_client("OK");
}

std::string get_parent_dir_path(std::string &filepath) {
  size_t slash_pos = filepath.rfind('/');
  std::string dirpath = filepath.substr(0, slash_pos);
  return dirpath;
}

std::string get_basename(std::string &filepath) {
  size_t slash_pos = filepath.rfind('/');
  std::string dirpath = filepath.substr(slash_pos + 1, filepath.npos);
  return dirpath;
}

void _write_file(std::string filepath_str, std::string text, FATData &data,
                 FileType type = FileType::FILE) {
  std::string whole_input = text;

  int blocks_cnt = whole_input.length() / BLK_SIZE;
  if (whole_input.length() % BLK_SIZE != 0) {
    blocks_cnt++;
  }

  FileInfo &fileinfo = data.files[filepath_str];
  fileinfo.name = filepath_str;
  fileinfo.type = type;
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
}

void update_parent_dir_content(std::string filepath, FATData &data) {
  std::string parent_dir = get_parent_dir_path(filepath);
  std::string file_basename = get_basename(filepath);

  std::string parent_dir_content;
  auto file = data.files.find(parent_dir);
  const FileInfo &parent_dir_info = file->second;
  for (Block block : parent_dir_info.data) {
    parent_dir_content += utils::read_block(block);
  }

  parent_dir_content += "/" + file_basename;

  for (Block block : parent_dir_info.data) {
    delete_block(block, data);
  }
  data.files.erase(parent_dir);

  _write_file(parent_dir, parent_dir_content, data, FileType::DIR);
}

void write_file(const char *filepath, const char *text, FATData &data) {
  if (filepath == nullptr || filepath[0] == '\0') {
    std::cerr << "Error no filename" << std::endl;
    write_status_client("Ошибка: не указано имя файла");
    return;
  }

  std::string filepath_str = filepath;
  if (file_already_exists(filepath_str, data, FileType::FILE)) {
    write_status_client("Ошибка: файл '" + filepath_str + "' уже существует");
    return;
  };

  if (!all_filepath_unit_exist(data, filepath)) {
    write_status_client(
        std::string("Ошибка: не существует какого-то из звеньев пути ") +
        filepath);
    return;
  }

  update_parent_dir_content(filepath_str, data);
  _write_file(filepath_str, text, data);

  write_status_client("OK");
}

int delete_file(const char *filename, FATData &data) {
  std::string filename_str = filename;
  if (data.files.find(filename_str) == data.files.end()) {
    write_status_client("Файл/директория с именем " + filename_str +
                        " не существует");
    return 1;
  }

  FileInfo file_to_delete = data.files[filename_str];
  for (Block block : file_to_delete.data) {
    delete_block(block, data);
  }
  data.files.erase(filename_str);
  return 0;
}

int edit_file(const char *filename, const char *text, FATData &data,
              bool do_user_checks = true) {
  std::string filename_str = filename;

  auto it = data.files.find(filename_str);
  if (it == data.files.end()) {
    write_status_client("Файл с именем " + filename_str + " не существует");
    return 1;
  }

  if (do_user_checks && it->second.type != FileType::FILE) {
    write_status_client("Ошибка: " + filename_str +
                        " является директорией, а не файлом");
    return 1;
  }

  FileInfo file_to_delete = data.files[filename_str];
  for (Block block : file_to_delete.data) {
    delete_block(block, data);
  }
  data.files.erase(filename_str);

  write_file(filename, text, data);
  return 0;
}

void list_files(const char *filepath, FATData &data) {
  if (data.files.empty()) {
    write_status_client("");
    return;
  }

  utils::Response response = utils::read_file(filepath, data, false);

  if (response.status == READ_DIR_ERR) {
    write_status_client("Ошибка: " + std::string(filepath) +
                        " является директорией");
    return;
  } else if (response.status == READ_NO_EXISTING_FILE) {
    std::string filename_str = filepath;
    write_status_client("Файл с именем " + filename_str + " не существует");
    return;
  }

  std::string file_list;
  std::string dir_list;

  std::stringstream ss(response.result);
  std::string file_system_object; // file, directory, etc.

  std::string dir_path = filepath;
  if (dir_path.back() != '/') {
    dir_path += '/';
  }

  while (std::getline(ss, file_system_object, '/')) {
    if (file_system_object.empty())
      continue;

    std::string full_path = dir_path + file_system_object;

    if (data.files.find(full_path) == data.files.end()) {
      full_path = dir_path + file_system_object + "/";
      if (data.files.find(full_path) == data.files.end()) {
        continue;
      }
    }

    auto it = data.files.find(full_path);
    FileType obj_type = it->second.type;

    if (obj_type == FileType::FILE) {
      file_list += file_system_object + " ";
    } else {
      dir_list += file_system_object + "/ ";
    }
  }

  std::string result;
  if (!dir_list.empty()) {
    result += "Директории: " + dir_list + "\n";
  }
  if (!file_list.empty()) {
    result += "Файлы: " + file_list;
  }

  write_status_client(result);
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
      std::string absolute_file_path;
      iss >> absolute_file_path;

      if (buffer[0] == 'l') {
        if (!absolute_file_path.empty() && !is_valid_name(absolute_file_path)) {
          write_status_client("Ошибка: имя '" + absolute_file_path +
                              "' не является абсолютным путём");
        }
      } else if (!is_valid_name(absolute_file_path)) {
        write_status_client("Ошибка: имя '" + absolute_file_path +
                            "' не является абсолютным путём");
        continue;
      }

      if (buffer[0] == 'w') {
        std::string text;
        iss >> text;
        write_file(absolute_file_path.c_str(), text.c_str(), data);
      } else if (buffer[0] == 'm') {
        create_directory(absolute_file_path.c_str(), data);
      } else if (buffer[0] == 'x') {
        if (delete_file(absolute_file_path.c_str(), data) == 0) {
          write_status_client("OK");
        }
      } else if (buffer[0] == 'e') {
        std::string text;
        iss >> text;
        if (edit_file(absolute_file_path.c_str(), text.c_str(), data) == 0) {
        }
      } else if (buffer[0] == 'r') {
        read_file(absolute_file_path.c_str(), data);
      } else if (buffer[0] == 'l') {
        list_files(absolute_file_path.c_str(), data);
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
