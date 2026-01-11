#include "utils.hpp"
#include <sstream>

utils::Response utils::list_files(const char *filepath, FATData &data) {
  utils::Response answer;
  if (data.files.empty()) {
    answer.result = "";
    answer.status = OK;
    return answer;
  }

  utils::Response response = utils::read_file(filepath, data, false);

  if (response.status == READ_NO_EXISTING_FILE) {
    std::string filename_str = filepath;
    answer.result = "";
    answer.status = LS_NO_EXISTING_DIR;
    return answer;
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
      file_list += file_system_object + " ";
    } else {
      dir_list += file_system_object + "/ ";
    }
  }

  std::string result;
  result += dir_list;
  result += file_list;

  answer.result = result;
  answer.status = OK;
  return answer;
}
