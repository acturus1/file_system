#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

const int BLK_SIZE = 2;

int write_block(std::string block) {
  // std::cout << "Block: " << block << "\n";
  // определить пустое место в memory
  // записать в него блок
  std::fstream file;
  file.open("memory", std::ios::in | std::ios::out);

  char bufer[BLK_SIZE];
  file.seekg(0, std::ios::beg);

  while (file.read(bufer, BLK_SIZE)) {
    bool is_empty = true;

    for (int i = 0; i < BLK_SIZE; i++) {
      if (bufer[i] != 0) {
        is_empty = false;
        break;
      }
    }

    if (is_empty) {
      int pos = (int)file.tellg() - BLK_SIZE;
      file.seekp(pos);
      file.write(block.c_str(), BLK_SIZE);
      file.close();
      return pos;
    };
  };
  return 0;
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

  int first_position, None;
  for (int i = 0; i < blocks_cnt; ++i) {
    // whole_input[i * BLK_SIZE:(i+1) * BLK_SIZE]
    if (i == 0) {
      first_position = write_block(whole_input.substr(i * BLK_SIZE, BLK_SIZE));
      // std::cout << result << " ";
    } else {
      None = write_block(whole_input.substr(i * BLK_SIZE, BLK_SIZE));
      // std::cout << None << " ";
    }
  }
  // для FAT
  std::cout << " " << first_position << " " << blocks_cnt << " "
            << first_position + blocks_cnt * BLK_SIZE;

  std::fstream FAT;

  FAT.open("FAT", std::ios::out | std::ios::app);

  std::string fat_write_text =
      file_name + "[" + std::to_string(first_position) + "," +
      std::to_string(first_position + blocks_cnt * BLK_SIZE) + "]" + "\n";
  FAT.write(fat_write_text.c_str(), fat_write_text.length());
}

int main(int argc, char *argv[]) {
  if (argc == 1) {
    std::cerr << "Invalid usage, no arguments provided";
    return 1;
  }

  if (!strcmp(argv[1], "write")) {
    if (argc == 3) {
      write_file(argv[2]);
    }
  } else {
    std::cerr << "Invalid usage, no such command" << std::endl;
    return 1;
  }

  return 0;
}
