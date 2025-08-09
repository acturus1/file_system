#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

const int BLK_SIZE = 2;

int write_block(std::string block, long long pos_start) {
  std::cout << "Block: " << block << "\n";
  // определить пустое место в memory
  // записать в него блок

  std::fstream file;
  file.open("memory", std::ios::in | std::ios::out);
  file.seekp(pos_start);
  file.write(block.c_str(), BLK_SIZE);
  file.close();
  return pos_start;
}

void write_file(std::string file_name) {
  // Читает из stdin и записывает в свободное место в memory
  std::string input;
  std::string whole_input;
  std::getline(std::cin, whole_input);

  int blocks_cnt = whole_input.length() / BLK_SIZE;
  if (whole_input.length() % BLK_SIZE != 0) {
    blocks_cnt++;
  }

  std::fstream FAT_first_line("FAT");
  std::string first_line;
  std::getline(FAT_first_line, first_line);
  size_t commaPos = first_line.find(',');
  std::string firstNumberStr = first_line.substr(0, commaPos);
  long long firstNumber = 0;
  std::istringstream iss(firstNumberStr);
  iss >> firstNumber;
  std::cout << "first number in FAT:" << firstNumber << std::endl;

  int first_position, None;
  for (int i = 0; i < blocks_cnt; ++i) {
    // whole_input[i * BLK_SIZE:(i+1) * BLK_SIZE]
    if (i == 0) {
      first_position =
          write_block(whole_input.substr(i * BLK_SIZE, BLK_SIZE), firstNumber);
      firstNumber += BLK_SIZE;
      // std::cout << result << " ";
    } else {
      None =
          write_block(whole_input.substr(i * BLK_SIZE, BLK_SIZE), firstNumber);
      firstNumber += BLK_SIZE;
      // std::cout << None << " ";
    }
  }
  std::cout << "end write blocks" << std::endl;
  // для FAT
  std::string line;
  std::getline(FAT_first_line, line);
  size_t comma_pos = line.find(',');
  if (comma_pos == std::string::npos || line.empty()) {
    // Обработка ошибки - запятая не найдена или строка пустая
    std::cerr << "Error: invalid format in FAT first line (no comma found or "
                 "empty line)"
              << std::endl;
    // Можно выбросить исключение или выполнить другие действия по обработке
    // ошибки
    return; // или throw, или другое действие
  }
  std::string new_line = std::to_string(firstNumber) + line.substr(comma_pos);
  FAT_first_line.seekp(0);
  FAT_first_line << new_line;
  std::cout << " " << first_position << " " << blocks_cnt << " "
            << first_position + blocks_cnt * BLK_SIZE;
  std::cout << "wtite new first line in FAT" << std::endl;

  std::string fat_write_text =
      file_name + "[" + std::to_string(first_position) + "," +
      std::to_string(first_position + blocks_cnt * BLK_SIZE) + "]" + "\n";
  FAT_first_line.write(fat_write_text.c_str(), fat_write_text.length());
  FAT_first_line.close();
}

int main(int argc, char *argv[]) {
  std::cout << "start" << std::endl;
  std::string path = "FAT";
  std::ifstream file(path);
  if (file.is_open()) {
  } else {
    // создание файла если не найден
    std::ofstream FAT;
    FAT.open("FAT", std::ios::out | std::ios::trunc);
    FAT.write("0,", 3);
    std::cout << "FAT was created" << std::endl;
  }
  file.close();

  if (argc == 1) {
    std::cerr << "Invalid usage, no arguments provided";
    return 1;
  }

  if (!strcmp(argv[1], "write")) {
    if (argc == 3) {
      std::cout << "start writing" << std::endl;
      write_file(argv[2]);
    }
  } else {
    std::cerr << "Invalid usage, no such command" << std::endl;
    return 1;
  }

  return 0;
}
