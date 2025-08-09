#include <cstring>
#include <fstream>
#include <ios>
#include <iostream>
#include <sstream>
#include <string>

const int BLK_SIZE = 2;

int write_block(std::string block, long long pos_start) {
  std::cout << "Block: " << block << "\n";

  std::fstream file;
  file.open("memory", std::ios::in | std::ios::out | std::ios::binary);
  if (!file.is_open()) {
    // Если файл не существует, создаем его
    file.open("memory", std::ios::out | std::ios::binary);
    file.close();
    file.open("memory", std::ios::in | std::ios::out | std::ios::binary);
  }

  if (!file.is_open()) {
    std::cerr << "Error: Failed to open memory file" << std::endl;
    return -1;
  }

  file.seekp(pos_start);
  file.write(block.c_str(), BLK_SIZE);
  if (!file.good()) {
    std::cerr << "Error: Failed to write to memory file" << std::endl;
  }
  file.close();
  return pos_start;
}

long long get_number_start_free_memory() {
  std::fstream FAT;
  FAT.open("FAT", std::ios::in | std::ios::binary);
  if (!FAT.is_open()) {
    std::cerr << "Error: Failed to open FAT file" << std::endl;
    return -1;
  }

  std::string first_line;
  std::getline(FAT, first_line);
  size_t commaPos = first_line.find(',');
  if (commaPos == std::string::npos) {
    std::cerr << "Error: Invalid FAT format (no comma found)" << std::endl;
    FAT.close();
    return -1;
  }

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
  std::cout << "end write blocks" << std::endl;
  std::fstream FAT;
  FAT.open("FAT", std::ios::in | std::ios::out | std::ios::binary);
  if (!FAT.is_open()) {
    std::cerr << "Error: Failed to open FAT file for writing" << std::endl;
    return;
  }

  std::string first_line;
  std::getline(FAT, first_line);

  size_t comma_pos = first_line.find(',');
  if (comma_pos == std::string::npos || first_line.empty()) {
    std::cerr << "Error: invalid format in FAT first line" << std::endl;
    FAT.close();
    return;
  }

  std::string new_first_line =
      std::to_string(number_start_free_memory) + first_line.substr(comma_pos);

  FAT.seekp(0);
  FAT.write(new_first_line.c_str(), new_first_line.size());

  std::string new_entry =
      "\n" + file_name + "[" + std::to_string(first_position) + "," +
      std::to_string(first_position + blocks_cnt * BLK_SIZE) + "]";

  FAT.seekp(0, std::ios::end);

  FAT.seekg(-1, std::ios::end);
  char last_char;
  FAT.get(last_char);
  if (last_char != '\n') {
    FAT.seekp(0, std::ios::end);
    FAT.write("\n", 1);
  }

  FAT.seekp(0, std::ios::end);
  FAT.write(new_entry.c_str(), new_entry.size());

  FAT.flush();
  FAT.close();

  std::cout << "Updated FAT:" << std::endl;
  std::cout << new_first_line << new_entry << std::endl;
}

void write_file(std::string file_name) {
  std::string whole_input;
  std::cout << "Enter text to write: ";
  std::getline(std::cin, whole_input);

  int blocks_cnt = whole_input.length() / BLK_SIZE;
  if (whole_input.length() % BLK_SIZE != 0) {
    blocks_cnt++;
  }

  long long number_start_free_memory = get_number_start_free_memory();
  if (number_start_free_memory == -1) {
    return;
  }

  int first_position = -1;
  for (int i = 0; i < blocks_cnt; ++i) {
    int pos = write_block(whole_input.substr(i * BLK_SIZE, BLK_SIZE),
                          number_start_free_memory);
    if (pos == -1) {
      std::cerr << "Error: Failed to write block" << std::endl;
      return;
    }
    if (i == 0) {
      first_position = pos;
    }
    number_start_free_memory += BLK_SIZE;
  }

  if (first_position != -1) {
    write_info_into_FAT(first_position, number_start_free_memory, file_name,
                        blocks_cnt);
  }
}

int main(int argc, char *argv[]) {
  std::cout << "start" << std::endl;

  // Инициализация FAT
  std::fstream FAT("FAT", std::ios::in | std::ios::out | std::ios::binary);
  if (!FAT.is_open()) {
    FAT.open("FAT", std::ios::out | std::ios::binary);
    FAT.write("0,", 2);
    std::cout << "FAT was created" << std::endl;
    FAT.close();
  } else {
    FAT.close();
  }

  if (argc == 1) {
    std::cerr << "Invalid usage, no arguments provided";
    return 1;
  }

  if (!strcmp(argv[1], "write")) {
    if (argc == 3) {
      std::cout << "start writing" << std::endl;
      write_file(argv[2]);
    } else {
      std::cout << "Invalid usage, no such command" << std::endl;
    }
  } else {
    std::cerr << "Invalid usage, no such command" << std::endl;
    return 1;
  }

  return 0;
}
