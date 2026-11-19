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
      std::istringstream iss(buffer);
      std::vector<std::string> user_input;
      std::string word;
      while (iss >> word) {
        user_input.push_back(word);
      }

      if (user_input.empty()) {
        write_status_client("Ошибка: пустая команда");
        continue;
      }

      const std::string &cmd = user_input[0];

      if (cmd == "write") {
        if (user_input.size() < 3) {
          write_status_client("Ошибка: использование: write <путь> <текст>");
        } else {
          std::string text;
          for (size_t i = 2; i < user_input.size(); ++i) {
            if (i > 2)
              text += ' ';
            text += user_input[i];
          }
          write_file(user_input[1].c_str(), text.c_str(), data);
        }

      } else if (cmd == "mkdir") {
        if (user_input.size() < 2) {
          write_status_client("Ошибка: использование: mkdir <путь>");
        } else {
          create_directory(user_input[1].c_str(), data);
        }

      } else if (cmd == "delete") {
        if (user_input.size() < 2) {
          write_status_client("Ошибка: использование: delete <путь>");
        } else {
          delete_file(user_input[1].c_str(), data);
        }

      } else if (cmd == "edit") {
        if (user_input.size() < 3) {
          write_status_client("Ошибка: использование: edit <путь> <текст>");
        } else {
          std::string text;
          for (size_t i = 2; i < user_input.size(); ++i) {
            if (i > 2)
              text += ' ';
            text += user_input[i];
          }
          edit_file(user_input[1].c_str(), text.c_str(), data);
        }

      } else if (cmd == "read") {
        if (user_input.size() < 2) {
          write_status_client("Ошибка: использование: read <путь>");
        } else {
          read_file(user_input[1].c_str(), data);
        }

      } else if (cmd == "ls") {
        if (user_input.size() < 2) {
          write_status_client("Ошибка: использование: ls <путь>");
        } else {
          list_files(user_input[1].c_str(), data);
        }

      } else if (cmd == "tree") {
        if (user_input.size() < 2) {
          write_status_client("Ошибка: использование: tree <путь>");
        } else {
          cout_recursive(data, user_input[1].c_str(), 0);
        }

      } else if (cmd == "move") {
        if (user_input.size() < 3) {
          write_status_client(
              "Ошибка: использование: move <новый_путь> <старый_путь>");
        } else {
          move_file(data, user_input[1].c_str(), user_input[2].c_str());
        }

      } else {
        write_status_client("Неизвестная команда!");
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
