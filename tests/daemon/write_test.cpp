#include <gtest/gtest.h>

#include "../../daemon/model/fat_data.hpp"

void write_file(const char *filepath, const char *text, FATData &data);
void prepare_FAT(FATData &data);

TEST(WriteFileTest, CheckFATDataUpdated) {
  FATData data;
  prepare_FAT(data); // add root and maybe do some other preparations
  std::string file_path = "/f1";
  write_file(file_path.c_str(), "f1content", data);

  EXPECT_NE(data.files.find(file_path), data.files.end());
  FileInfo file_info = data.files[file_path];
  EXPECT_EQ(file_info.name, file_path);
  EXPECT_EQ(file_info.type, FileType::FILE);
  // добавить проверку на контент
}
