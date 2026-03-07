#include "./fs_daemon.cpp"
#include <iostream>
#include <string.h>
#include <vector>

bool is_beginning_matches(char *target, const char *pattern) {
  for (int i = 0; i < strlen(pattern); i++) {
    std::cout << target[i] << "-" << pattern[i] << std::endl;
    if (target[i] != pattern[i]) {
      return false;
    }
  }
  return true;
}

int main() {
  FATData data = read_FAT_from_disk();
  prepare_FAT(data);
  debug_print_FAT(data);

  create_fifos();

  int fd = open(fifo_path, O_RDONLY);
  if (fd == -1) {
    perror("Ошибка открытия FIFO для записи серверу");
    return 1;
  }
  char buffer[1024];
  while (true) {
    int bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    if (bytes_read == -1) {
      perror("Ошибка чтения");
      break;
    }
    if (bytes_read == 0) {
      std::cout << "Клиент отключился" << std::endl;
      break;
    } else {
      buffer[bytes_read] = '\0';
      std::istringstream iss(buffer + 1);
      std::vector<std::string> user_input;
      std::string word;
      while (iss >> word) {
        user_input.push_back(word);
      }

      if (is_beginning_matches(buffer, "write")) {
        write_file(user_input[1].c_str(), user_input[2].c_str(), data);
      }
      if (is_beginning_matches(buffer, "mkdir")) {
        create_directory(user_input[1].c_str(), data);
      }
      if (is_beginning_matches(buffer, "delete")) {
        delete_file(user_input[1].c_str(), data);
      }
      if (is_beginning_matches(buffer, "edit")) {
        edit_file(user_input[2].c_str(), user_input[2].c_str(), data);
      }
      if (is_beginning_matches(buffer, "read")) {
        read_file(user_input[1].c_str(), data);
      }
      if (is_beginning_matches(buffer, "ls")) {
        list_files(user_input[1].c_str(), data);
      }
      if (is_beginning_matches(buffer, "tree")) {
        cout_recursive(data, user_input[1].c_str(), 0);
      }
      if (is_beginning_matches(buffer, "move")) {
        move_file(data, user_input[1].c_str(), user_input[2].c_str());
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
