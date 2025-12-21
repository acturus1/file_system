#include "../../daemon/model/fat_data.hpp"
#include "../../daemon/utils/utils.hpp"
#include <algorithm>
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>
#include <string>

void write_file(const char *filepath, const char *text, FATData &data);
void prepare_FAT(FATData &data);
utils::Response utils::read_file(const char *filename, FATData &data,
                                 bool do_dir_check);
int delete_file(const char *filename, FATData &data);
int edit_file(const char *filename, const char *text, FATData &data,
              bool do_user_checks = true);

const char *file_path = "/f1";
const char *file_content = "f1content";
FATData data;

TEST(WriteFileTest, CheckFATDataUpdated) {
  prepare_FAT(data); // add root and maybe do some other preparations
  write_file(file_path, file_content, data);

  EXPECT_NE(data.files.find(file_path), data.files.end());
  FileInfo file_info = data.files[file_path];
  EXPECT_EQ(file_info.name, file_path);
  EXPECT_EQ(file_info.type, FileType::FILE);
}

TEST(WriteFileTest, CheckUpdateMemory) {
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
  memory_test.close();
};

TEST(ReadFileTest, CheckFATDataUpdatedAndMemory) {
  utils::Response result = utils::read_file(file_path, data, false);
  ASSERT_EQ(OK, result.status);
  EXPECT_EQ(std::string(file_content), result.result);
}

TEST(DeleteFileTest, CheckDeleteFileFromFat) {
  delete_file(file_path, data);
  EXPECT_EQ(data.files.find(file_path), data.files.end());
};

TEST(EditFileTest, EditFileTestFat) {
  const char *new_content = "new_content";
  write_file(file_path, file_content, data);
  edit_file(file_path, "new_content", data);
  std::ifstream memory_test("./test_memory", std::ios::binary);

  char output[100];
  memory_test.read(output, sizeof(output));
  std::string result;
  for (int i = 0; i < 100; i++) {
    if (output[i] != '\0') {
      result += output[i];
    }
  }
  std::string combined = std::string(file_path) + std::string(new_content);

  std::string sorted_result = result;
  std::string sorted_combined = combined;

  std::sort(sorted_result.begin(), sorted_result.end());
  std::sort(sorted_combined.begin(), sorted_combined.end());

  ASSERT_EQ(sorted_result, sorted_combined);
  memory_test.close();
};

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  int result = RUN_ALL_TESTS();
  return result;
}
