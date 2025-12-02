#include "../../daemon/model/fat_data.hpp"
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>
#include <string>

void write_file(const char *filepath, const char *text, FATData &data);
void prepare_FAT(FATData &data);

TEST(WriteFileTest, CheckFATDataUpdated) {
  FATData data;
  prepare_FAT(data); // add root and maybe do some other preparations
  std::string file_path = "/f1";
  const char *file_content = "f1content";
  write_file(file_path.c_str(), file_content, data);

  EXPECT_NE(data.files.find(file_path), data.files.end());
  FileInfo file_info = data.files[file_path];
  EXPECT_EQ(file_info.name, file_path);
  EXPECT_EQ(file_info.type, FileType::FILE);
  std::ifstream memory_test("./test_memory", std::ios::binary);
  char output[100];
  memory_test.read(output, sizeof(output));
  std::string result;
  for (int i = 0; i < 100; i++) {
    if (output[i] != '\0') {
      result += output[i];
    }
  }
  ASSERT_EQ(result, std::string(file_path) + std::string(file_content));
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
