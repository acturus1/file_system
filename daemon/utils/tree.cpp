#include "utils.hpp"
#include <iostream>
#include <sstream>
#include <string>

void search_tree_recursive(FATData &data, std::string dirpath,
                           utils::TreeResponse &result) {
  utils::ListResponse answer = utils::list_files(dirpath.c_str(), data);

  result.result[dirpath] = answer.result;

  for (std::string s : answer.result) {
    if (!s.empty()) {
      if (s[s.length() - 1] == '/') {
        std::string name_without_slash = s.substr(0, s.length() - 1);
        std::string new_path;
        if (dirpath == "/") {
          new_path = dirpath + name_without_slash;
        } else {
          new_path = dirpath + "/" + name_without_slash;
        }
        search_tree_recursive(data, new_path, result);
      }
    }
  }
}

utils::TreeResponse utils::tree(FATData &data, std::string dirpath) {
  utils::TreeResponse result;
  search_tree_recursive(data, dirpath, result);
  result.status = OK;
  std::cout << result.status << std::endl;
  for (auto [dir, content] : result.result) {
    std::cout << "Object " << dir << ": \n";
    for (std::string child : content) {
      std::cout << child << ", ";
    }
    std::cout << std::endl;
  }
  return result;
}
