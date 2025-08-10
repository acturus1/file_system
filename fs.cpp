#include <cstring>
#include <fstream>
#include <ios>
#include <iostream>
#include <sstream>
#include <string>

const int BLK_SIZE = 2;

int write_block(std::string block, long long pos_start) {
  std::cout << "Block: " << block << "\n";
  // определить пустое место в memory
  // записать в него блок

  std::fstream file;
  file.open("memory", std::ios::in | std::ios::out | std::ios::binary);
  file.seekp(pos_start);
  file.write(block.c_str(), BLK_SIZE);
  file.close();
  return pos_start;
}

long long get_number_start_free_memory() {
  std::fstream FAT;
  FAT.open("FAT", std::ios::in | std::ios::binary);
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
  std::cout << "end write blocks" << std::endl;

  std::fstream FAT("FAT", std::ios::in | std::ios::out | std::ios::binary);
  if (!FAT.is_open()) {
    std::cerr << "Error: FAT file not opened!" << std::endl;
    return;
  }

  std::string first_line;
  std::getline(FAT, first_line);

  std::stringstream rest_of_fat;
  rest_of_fat << FAT.rdbuf();

  size_t comma_pos = first_line.find(',');
  if (comma_pos == std::string::npos) {
    std::cerr << "Error: No comma in first line!" << std::endl;
    FAT.close();
    return;
  }

  FAT.close();
  FAT.open("FAT", std::ios::out | std::ios::trunc | std::ios::binary);

  std::string new_first_line = std::to_string(number_start_free_memory) +
                               first_line.substr(comma_pos) + "\n";
  FAT.write(new_first_line.c_str(), new_first_line.size());

  FAT << rest_of_fat.str();

  std::string new_file_entry =
      file_name + "[" + std::to_string(first_position) + "," +
      std::to_string(first_position + blocks_cnt * BLK_SIZE) + "]" + "\n";
  FAT.write(new_file_entry.c_str(), new_file_entry.size());

  FAT.flush();
  FAT.close();

  std::cout << "FAT updated:" << std::endl;
  std::cout << new_first_line << std::endl;
  std::cout << new_file_entry << std::endl;
}

void write_file(std::string file_name) {
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
    if (i == 0) {
      first_position = write_block(whole_input.substr(i * BLK_SIZE, BLK_SIZE),
                                   number_start_free_memory);
      number_start_free_memory += BLK_SIZE;
    } else {
      None = write_block(whole_input.substr(i * BLK_SIZE, BLK_SIZE),
                         number_start_free_memory);
      number_start_free_memory += BLK_SIZE;
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
    std::ofstream FAT;
    FAT.open("FAT", std::ios::out | std::ios::binary);
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
    } else {
      std::cout << "Invalid usage, no such command" << std::endl;
    }
  } else {
    std::cerr << "Invalid usage, no such command" << std::endl;
    return 1;
  }

  return 0;
}
