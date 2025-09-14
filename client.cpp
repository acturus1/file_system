#include <cstdio>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <unistd.h>

int main() {
  const char *fifo_path = "./myfifo";
  const char *fifo_path_read = "./server_fifo";

  int fd = open(fifo_path, O_WRONLY);
  if (fd == -1) {
    puts("Ошибка открытия FIFO на запись");
    return 1;
  }

  std::cout
      << "Клиент подключился к серверу. Введите сообщения (Ctrl+D для выхода):"
      << std::endl;
  std::cout << "Введите команды (write, edit, read, delete, exit):"
            << std::endl;

  std::string message;
  while (std::getline(std::cin, message)) {
    if (message == "exit") {
      break;
    }

    if (message.find("write ") == 0) {
      message.replace(0, 5, "w");
    } else if (message.find("edit ") == 0) {
      message.replace(0, 4, "e");
    } else if (message.find("read ") == 0) {
      message.replace(0, 4, "r");
    } else if (message.find("dir ") == 0) {
      message.replace(0, 3, "d");
    } else if (message.find("delete ") == 0) {
      message.replace(0, 6, "x");
    } else if (message.find("ls") == 0) {
      message.replace(0, 2, "l");
      continue;
    } else {
      puts("Ошибка записи");
    }
    message += '\n';
    ssize_t bytes_written =
        write(fd, message.c_str(), message.length()); // while
    if (bytes_written == -1) {
      puts("Ошибка записи");
      break;
    }
    std::cout << "Команда отправлена. Ожидание ответа..." << std::endl;
    int read_fd = open(fifo_path_read, O_RDONLY);
    if (read_fd == -1) {
      puts("Ошибка открытия FIFO на чтение");
      close(read_fd);
      return 1;
    }

    char reply[1024];
    ssize_t bytes_read = read(read_fd, reply, sizeof(reply) - 1);
    if (bytes_read > 0) {
      reply[bytes_read] = '\0';
      std::cout << "Ответ сервера: " << reply;
    } else if (bytes_read == -1) {
      puts("Ошибка чтения ответа");
    }
  }
  close(fd);
  std::cout << "Клиент завершает работу" << std::endl;
  return 0;
}
