#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

const int BLK_SIZE = 2;

int write_block(std::string block) {
  std::fstream file("memory", std::ios::in | std::ios::out);

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

  file.close();
  return 0;
}

int get_blocks_cnt(const std::string &str) {
  int blocks_cnt = str.length() / BLK_SIZE;
  if (str.length() % BLK_SIZE != 0) {
    blocks_cnt++;
  }
  return blocks_cnt;
}

void update_FAT(int blocks_cnt, int first_position,
                const std::string &file_name) {
  // для FAT
  std::cout << " " << first_position << " " << blocks_cnt << " "
            << first_position + blocks_cnt * BLK_SIZE;

  std::fstream FAT("FAT", std::ios::out | std::ios::app);

  std::string fat_write_text =
      file_name + "[" + std::to_string(first_position) + "," +
      std::to_string(first_position + blocks_cnt * BLK_SIZE) + "]" + "\n";
  FAT.write(fat_write_text.c_str(), fat_write_text.length());
}

void write_file(const std::string &file_name) {
  // Читает из stdin и записывает в свободное место в memory
  std::string input;
  std::string whole_input;
  while (std::getline(std::cin, input)) {
    if (!whole_input.empty()) {
      whole_input += "\n";
    }
    whole_input += input;
  }

  int blocks_cnt = get_blocks_cnt(whole_input);

  int first_position, None;
  for (int i = 0; i < blocks_cnt; ++i) {
    // whole_input[i * BLK_SIZE:(i+1) * BLK_SIZE]
    if (i == 0) {
      first_position = write_block(whole_input.substr(i * BLK_SIZE, BLK_SIZE));
    } else {
      None = write_block(whole_input.substr(i * BLK_SIZE, BLK_SIZE));
    }
  }

  update_FAT(blocks_cnt, first_position, file_name);
}

int main(int argc, char *argv[]) {
  if (argc == 1) {
    std::cerr << "Invalid usage, no arguments provided";
    return 1;
  }

  if (!strcmp(argv[1], "write")) {
    if (argc == 3) {
      write_file(argv[2]);
    } else {
      std::cerr << "Invalid usage, give a name of the file" << std::endl;
      return 1;
    }
  } else {
    std::cerr << "Invalid usage, no such command\n"
              << "Possible commands are: write <file_name>" << std::endl;
    return 1;
  }

  return 0;
}
