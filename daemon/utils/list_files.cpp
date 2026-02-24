#include "utils.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

utils::ListResponse utils::list_files(const char *filepath, FATData &data) {
  utils::ListResponse answer;
  if (data.files.empty()) {
    answer.status = OK;
    return answer;
  }

  utils::Response response = utils::read_file(filepath, data, false);

  if (response.status == READ_NO_EXISTING_FILE) {
    std::string filename_str = filepath;
    answer.status = LS_NO_EXISTING_DIR;
    return answer;
  }

  std::vector<std::string> file_list;
  std::vector<std::string> dir_list;

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
      file_list.push_back(file_system_object);
    } else {
      dir_list.push_back(file_system_object + "/");
    }
  }

  std::vector<std::string> result;
  result.insert(result.end(), file_list.begin(), file_list.end());
  result.insert(result.end(), dir_list.begin(), dir_list.end());

  answer.result = result;
  answer.status = OK;
  for (std::string x : answer.result)
    std::cout << x << std::endl;
  return answer;
}
