#include <cstdlib>
#include <cstring>
#include <fcntl.h>
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
  // std::cout << "Block: " << block << "\n";

  std::fstream file;
  file.open(MEMORY_PATH, std::ios::in | std::ios::out | std::ios::binary);
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

std::string get_parent_dir_path(std::string &filepath) {
  size_t slash_pos = filepath.rfind('/');
  std::string dirpath = filepath.substr(0, slash_pos);
  if (dirpath.empty()) {
    dirpath = "/";
  }
  return dirpath;
}

std::string get_basename(std::string &filepath) {
  size_t slash_pos = filepath.rfind('/');
  std::string dirpath = filepath.substr(slash_pos + 1, filepath.npos);
  return dirpath;
}

void update_parent_dir_content(std::string filepath, FATData &data) {
  std::string parent_dir = get_parent_dir_path(filepath);
  std::string file_basename = get_basename(filepath);

  std::string parent_dir_content;
  if (data.files.find(parent_dir) == data.files.end()) {
    write_status_client("Внутренняя ошибка: не удаётся получить содержимое "
                        "родительской директории.");
    return;
  }
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

void delete_parent_dir_content(std::string filepath, FATData &data) {
  std::string parent_dir = get_parent_dir_path(filepath);
  std::string file_basename = get_basename(filepath);

  std::string parent_dir_content;
  if (data.files.find(parent_dir) == data.files.end()) {
    write_status_client("Внутренняя ошибка: не удаётся получить содержимое "
                        "родительской директории.");
    return;
  }
  auto file = data.files.find(parent_dir);
  const FileInfo &parent_dir_info = file->second;
  for (Block block : parent_dir_info.data) {
    parent_dir_content += utils::read_block(block);
  }

  int pos = parent_dir_content.find("/" + file_basename);
  if (parent_dir_content.find("/" + file_basename + "/") != std::string::npos) {
    pos = parent_dir_content.find("/" + file_basename + "/");
  }
  parent_dir_content.replace(pos, file_basename.size() + 1, "");

  for (Block block : parent_dir_info.data) {
    delete_block(block, data);
  }
  data.files.erase(parent_dir);

  _write_file(parent_dir, parent_dir_content, data, FileType::DIR);
};

void create_directory(const char *dirpath, FATData &data) {
  if (dirpath == nullptr || dirpath[0] == '\0') {
    write_status_client("Ошибка: не указано имя директории");
    return;
  }

  if (dirpath[std::strlen(dirpath) - 1] == '/') {
    write_status_client("Ошибка: имя дериктории не должно заканчиваться на /");
    return;
  }

  std::string dirpath_str = dirpath;
  if (file_already_exists(dirpath_str, data, FileType::DIR)) {
    write_status_client(std::string("Ошибка: директория '") + dirpath +
                        std::string("' уже существует"));
    return;
  }

  if (!all_filepath_unit_exist(data, dirpath)) {
    write_status_client(
        std::string("Ошибка: не существует какого-то из звеньев пути ") +
        dirpath);
    return;
  }

  update_parent_dir_content(dirpath, data);

  FileInfo &dir_info = data.files[dirpath_str];
  dir_info.name = dirpath_str;
  dir_info.type = FileType::DIR;
  dir_info.data.clear();

  write_status_client("OK");
}

void write_file(const char *filepath, const char *text, FATData &data) {
  if (filepath == nullptr || filepath[0] == '\0') {
    std::cerr << "Error no filename" << std::endl;
    write_status_client("Ошибка: не указано имя файла");
    return;
  }
  if (filepath[std::strlen(filepath) - 1] == '/') {
    write_status_client("Ошибка: имя дериктории не должно заканчиваться на /");
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

std::string recursive_find_all_files_to_delete(std::string dirpath,
                                               FATData &data) {

  utils::Response response = utils::read_file(dirpath.c_str(), data, false);
  std::string file_list;
  std::string dir_list;
  std::string dir_path = dirpath;
  std::stringstream ss(response.result);
  std::string file_system_object; // file, directory, etc.
  std::string result;

  if (dir_path.back() != '/') {
    dir_path += '/';
  }

  while (std::getline(ss, file_system_object, '/')) {
    if (file_system_object.empty()) {
      continue;
    }

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
      result += dir_path + file_system_object + "|";
    } else {
      result += dir_path + file_system_object + "|";
      result += recursive_find_all_files_to_delete(
          dir_path + file_system_object, data);
    }
  }

  return result;
}

// delete /d1
int delete_file(const char *filename, FATData &data) {
  std::string filename_str = filename;
  if (data.files.find(filename_str) == data.files.end()) {
    write_status_client("Файл/директория с именем " + filename_str +
                        " не существует");
    return 1;
  }

  FileInfo file_info = data.files[filename];
  std::vector<std::string> files_to_delete;
  files_to_delete.push_back(filename); // сам файл

  if (!file_info.data.empty() && file_info.type == FileType::DIR) {
    std::string input = recursive_find_all_files_to_delete(filename_str, data);
    std::cout << input << std::endl;
    std::stringstream ss(input);
    // |d1|f1|f2|
    std::string token;
    while (std::getline(ss, token, '|')) {
      if (token != std::string(filename)) { // добавляю все нижние файлы
        files_to_delete.push_back(token);
      }
    }
  }

  for (int i = files_to_delete.size() - 1; i >= 0; i--) {
    std::string file_to_delete_name = files_to_delete[i];
    FileInfo file_to_delete_info = data.files[file_to_delete_name];
    for (Block block : file_to_delete_info.data) {
      delete_block(block, data);
    }
    delete_parent_dir_content(file_to_delete_name, data);
    data.files.erase(file_to_delete_name);
  }

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

  delete_parent_dir_content(filename, data);
  write_file(filename, text, data);
  return 0;
}

void list_files(const char *filepath, FATData &data) {
  utils::Response answer = utils::list_files(filepath, data);
  if (answer.status == OK && answer.result == "") {
    write_status_client("Пусто!");
  }

  if (answer.status == LS_NO_EXISTING_DIR && answer.result == "") {
    std::string filename_str = filepath;
    write_status_client("Файл с именем " + filename_str + " не существует");
  }

  write_status_client(answer.result);
}

std::string cout_recursive_(FATData &data, std::string dirpath, int depth) {
  // if (dirpath == "/") {
  //   return "";
  // }
  std::string result;
  std::string indent(depth * 2, ' ');

  for (const auto &[key, fileinfo] : data.files) {
    size_t last_slash = key.find_last_of('/');
    std::string parent;

    if (last_slash == 0) {
      parent = "/";
    } else if (last_slash != std::string::npos) {
      parent = key.substr(0, last_slash);
    } else {
      parent = "";
    }

    if (parent == dirpath) {
      std::string name = key.substr(last_slash + 1);

      if (fileinfo.type == FileType::DIR) {
        result += indent + name + " D\n";
        result += cout_recursive_(data, key, depth + 1);
      } else {
        result += indent + name + " F\n";
      }
    }
  }
  return result;
}

void cout_recursive(FATData &data, std::string dirpath, int depth = 0) {
  auto file = data.files.find(dirpath);

  if (file == data.files.end()) {
    write_status_client(std::string("Ошибка: нет такой дериктории ") + dirpath);
    return;
  }
  if (data.files[dirpath].type != FileType::DIR) {
    write_status_client("Ошибка: " + dirpath + " не является директорией");
    return;
  }

  if (!all_filepath_unit_exist(data, dirpath)) {
    write_status_client(
        std::string("Ошибка: не существует какого-то из звеньев пути ") +
        dirpath);
    return;

    std::string result = "\n" + cout_recursive_(data, dirpath, depth);
    if (result.empty()) {
      write_status_client("пусто");
    } else {
      write_status_client(result);
    }
  }
}

void prepare_FAT(FATData &data) {
  std::string root = "/";
  if (data.files.find(root) == data.files.end()) {
    FileInfo &fileinfo = data.files[root];
    fileinfo.name = root;
    fileinfo.type = FileType::DIR;
    fileinfo.data.clear();
  }
}
