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

long long get_number_start_free_memory() {
  std::fstream FAT("FAT");
  std::string first_line;
  std::getline(FAT, first_line);
  size_t commaPos = first_line.find(',');
  std::string number_start_free_memoryStr = first_line.substr(0, commaPos);
  long long number_start_free_memory = 0;
  std::istringstream iss(number_start_free_memoryStr);
  iss >> number_start_free_memory;
  std::cout << "first number in FAT:" << number_start_free_memory << std::endl;
  FAT.close();
  return number_start_free_memory;
}

void write_info_into_FAT(int first_position, long long number_start_free_memory,
                         std::string file_name, int blocks_cnt) {
  // std::cout << first_position << number_start_free_memory << file_name <<
  // blocks_cnt << std::endl;
  std::cout << "end write blocks" << std::endl;
  // для FAT
  std::fstream FAT;
  FAT.open("FAT");
  std::string line;
  std::getline(FAT, line);
  size_t comma_pos = line.find(',');
  if (comma_pos == std::string::npos || line.empty()) {
    // Обработка ошибки - запятая не найдена или строка пустая
    std::cerr << "Error: invalid format in FAT first line (no comma found or "
                 "empty line)"
              << std::endl;
    return;
  }
  std::string new_line =
      std::to_string(number_start_free_memory) + line.substr(comma_pos);
  FAT.seekp(0);
  FAT << new_line;
  std::cout << new_line << std::endl;
  std::cout << " " << first_position << " " << blocks_cnt << " "
            << first_position + blocks_cnt * BLK_SIZE;
  std::cout << "wtite new first line in FAT" << std::endl;

  std::string fat_write_first_line_text =
      "\n" + file_name + "[" + std::to_string(first_position) + "," +
      std::to_string(first_position + blocks_cnt * BLK_SIZE) + "]";
  FAT.seekp(0, std::ios::end);
  FAT.write(fat_write_first_line_text.c_str(),
            fat_write_first_line_text.length());
  FAT.close();
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

  long long number_start_free_memory = get_number_start_free_memory();

  int first_position, None;
  for (int i = 0; i < blocks_cnt; ++i) {
    // whole_input[i * BLK_SIZE:(i+1) * BLK_SIZE]
    if (i == 0) {
      first_position = write_block(whole_input.substr(i * BLK_SIZE, BLK_SIZE),
                                   number_start_free_memory);
      number_start_free_memory += BLK_SIZE;
      // std::cout << result << " ";
    } else {
      None = write_block(whole_input.substr(i * BLK_SIZE, BLK_SIZE),
                         number_start_free_memory);
      number_start_free_memory += BLK_SIZE;
      // std::cout << None << " ";
    }
  }
  write_info_into_FAT(first_position, number_start_free_memory, file_name,
                      blocks_cnt);
}

int main(int argc, char *argv[]) {
  std::cout << "start" << std::endl;
  std::string path = "FAT";
  std::ifstream file(path);
  if (file.is_open()) {
  } else {
    // создание файла если не найден
    std::ofstream FAT;
    FAT.open("FAT", std::ios::out);
    FAT.write("0,", 2);
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
