#include "./fs_daemon.cpp"

int main() {
  FATData data = read_FAT_from_disk();
  prepare_FAT(data);
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
