#include "../../daemon/model/fat_data.hpp"
#include "../../daemon/utils/utils.hpp"
#include <algorithm>
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>
#include <ostream>
#include <string>

void write_file(const char *filepath, const char *text, FATData &data);
void prepare_FAT(FATData &data);
utils::Response utils::read_file(const char *filename, FATData &data,
                                 bool do_dir_check);
int delete_file(const char *filename, FATData &data);
int edit_file(const char *filename, const char *text, FATData &data,
              bool do_user_checks = true);
void create_directory(const char *dirname, FATData &data);

utils::Response utils::list_files(const char *filepath, FATData &data);

const char *file_path = "/f1";
const char *file_content = "f1content";
const char *new_content = "new_content";
const char *dirname1 = "/dir1";
const char *dirname2 = "/dir2";
const char *dirname3 = "/dir3";
const char *file_path2 = "/f2";

class FileSystemTest : public ::testing::Test {
protected:
  FATData data;
  void SetUp() override {
    data.empty_blocks.clear();
    data.files.clear();
    data.start_free_memory = 0;
    prepare_FAT(data);
    // создаём пустую фат-дату и пустую мемор
    system("dd if=/dev/zero bs=1 of=test_memory count=1024 status=none");
  }

  void TearDown() override {
    // чистим фат-дату и мемори
  }
};

TEST_F(FileSystemTest, CheckFATDataUpdated) {
  write_file(file_path, file_content, data);

  EXPECT_NE(data.files.find(file_path), data.files.end());
  FileInfo file_info = data.files[file_path];
  EXPECT_EQ(file_info.name, file_path);
  EXPECT_EQ(file_info.type, FileType::FILE);
}

TEST_F(FileSystemTest, CheckUpdateMemory) {
  write_file(file_path, file_content, data);
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

TEST_F(FileSystemTest, CheckFATDataUpdatedAndMemory) {
  write_file(file_path, file_content, data);
  utils::Response result = utils::read_file(file_path, data, false);
  ASSERT_EQ(OK, result.status);
  EXPECT_EQ(std::string(file_content), result.result);
}

TEST_F(FileSystemTest, EditFileTestMemory) {
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
  std::string file_path_and_new_content =
      std::string(file_path) + std::string(new_content);

  std::string sorted_result = result;
  std::string sorted_file_path_and_new_content = file_path_and_new_content;

  std::sort(sorted_result.begin(), sorted_result.end());
  std::sort(sorted_file_path_and_new_content.begin(),
            sorted_file_path_and_new_content.end());

  ASSERT_EQ(sorted_result, sorted_file_path_and_new_content);
  memory_test.close();
};

TEST_F(FileSystemTest, ListRootDirectory) {
  write_file(file_path, file_content, data);
  utils::Response response = utils::list_files("/", data);

  ASSERT_EQ(response.status, OK);
  std::string result = response.result;
  if (!result.empty() && result.back() == ' ') {
    result.pop_back();
  };
  std::string list_file = file_path;
  list_file.erase(0, 1);
  EXPECT_EQ(result, list_file);
};

TEST_F(FileSystemTest, ListNotExistingDirectory) {
  utils::Response response = utils::list_files("/not_existing_directory", data);
  ASSERT_EQ(response.status, LS_NO_EXISTING_DIR);
}

TEST_F(FileSystemTest, CheckDeleteFileFromFat) {
  delete_file(file_path, data);
  EXPECT_EQ(data.files.find(file_path), data.files.end());
};

TEST_F(FileSystemTest, MakeDirectory) {
  create_directory(dirname1, data);
  ASSERT_NE(data.files.find(dirname1), data.files.end());
  FileInfo dir_info = data.files[dirname1];

  EXPECT_EQ(dir_info.name, dirname1);
  EXPECT_EQ(dir_info.type, FileType::DIR);
};

TEST_F(FileSystemTest, DeleteDir) {
  delete_file(dirname1, data);
  ASSERT_EQ(data.files.find(dirname1), data.files.end());
}

TEST_F(FileSystemTest, LsTest1) {
  create_directory(dirname1, data);
  create_directory((std::string(dirname1) + std::string(dirname2)).c_str(),
                   data);
  create_directory((std::string(dirname1) + std::string(dirname3)).c_str(),
                   data);
  utils::Response response = utils::list_files(dirname1, data);
  std::string reformed_d2 =
      std::string(dirname2).substr(1, std::string(dirname2).size() - 1) + "/ ";
  std::string reformed_d3 =
      std::string(dirname3).substr(1, std::string(dirname3).size() - 1) + "/ ";
  EXPECT_EQ(response.result, reformed_d2 + reformed_d3);
}

TEST_F(FileSystemTest, LsTest2) {
  create_directory(dirname1, data);
  create_directory((std::string(dirname1) + std::string(dirname2)).c_str(),
                   data);
  create_directory((std::string(dirname1) + std::string(dirname3)).c_str(),
                   data);
  std::string reformed_d3 =
      std::string(dirname3).substr(1, std::string(dirname3).size() - 1) + "/ ";

  delete_file((std::string(dirname1) + std::string(dirname2)).c_str(), data);
  utils::Response response = utils::list_files(dirname1, data);
  EXPECT_EQ(response.result, reformed_d3);
}

TEST_F(FileSystemTest, LsTest3) {
  create_directory(dirname1, data);
  write_file((std::string(dirname1) + std::string(file_path)).c_str(),
             file_content, data);
  write_file((std::string(dirname1) + std::string(file_path2)).c_str(),
             file_content, data);
  utils::Response response = utils::list_files(dirname1, data);

  std::string reformed_f1 =
      std::string(file_path).substr(1, std::string(file_path).size() - 1) + " ";
  std::string reformed_f2 =
      std::string(file_path2).substr(1, std::string(file_path2).size() - 1) +
      " ";

  EXPECT_EQ(response.result, reformed_f1 + reformed_f2);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  int result = RUN_ALL_TESTS();
  return result;
}
