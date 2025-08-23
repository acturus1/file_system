#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <unistd.h>

int main() {
  const char *fifo_path = "./myfifo";

  int fd = open(fifo_path, O_WRONLY);
  if (fd == -1) {
    perror("Ошибка открытия FIFO на запись");
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
    } else if (message.find("delete ") == 0) {
      message.replace(0, 6, "d");
    }

    message += '\n';
    ssize_t bytes_written =
        write(fd, message.c_str(), message.length()); // while
    if (bytes_written == -1) {
      perror("Ошибка записи");
      break;
    }
  }

  close(fd);
  std::cout << "Клиент завершает работу" << std::endl;
  return 0;
}
