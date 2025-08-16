#include <map>
#include <string>
#include <vector>

class Block {
  int start;
  int end;
};

class FileInfo {
  std::string name;
  std::vector<Block> data;
};

void read_FAT_from_disk(std::vector<Block> &empty_blocks,
                        std::map<std::string, FileInfo> &FAT_files) {
  // read from disk appropriate info
}

int main() {

  std::vector<Block> empty_blocks;
  std::map<std::string, FileInfo> FAT_files;

  read_FAT_from_disk(empty_blocks, FAT_files);

  while (true) {
    // читаем из пайпа
    // если приходит запрос на запись нового файла
    // ищем предварительно его по имени в FAT
    // FAT_files.find(new_file_name)
    // если существует, то алёртим
    // если нет, то записываем файл

    // если приходит запрос на завершение демона
    // дампимся на диск в файл FAT
  }
}
