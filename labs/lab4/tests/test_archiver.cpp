#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <utility>

namespace fs = std::filesystem;

// === Helper Functions ========================================================

std::string QuotePath(const fs::path& path) {
  fs::path native = path;
  native.make_preferred();
  std::string s = native.string();
  return "\"" + s + "\"";
}

bool FilesEqual(const fs::path& a, const fs::path& b) {
  if (!fs::exists(a) || !fs::exists(b)) return false;
  if (fs::file_size(a) != fs::file_size(b)) return false;

  std::ifstream fa(a, std::ios::binary);
  std::ifstream fb(b, std::ios::binary);
  if (!fa || !fb) return false;

  constexpr std::streamsize kBufferSize = 1 << 20;
  std::vector<char> buf_a(kBufferSize);
  std::vector<char> buf_b(kBufferSize);

  while (fa && fb) {
    fa.read(buf_a.data(), kBufferSize);
    fb.read(buf_b.data(), kBufferSize);

    std::streamsize ca = fa.gcount();
    std::streamsize cb = fb.gcount();

    if (ca != cb) return false;
    if (std::memcmp(buf_a.data(), buf_b.data(), static_cast<size_t>(ca)) != 0) {
      return false;
    }
  }

  return fa.eof() && fb.eof();
}

// === Test ====================================================================

TEST(HamArcCliTest, CreateExtractAndCompare) {
  // RESOURCES_DIR is passed from CMake
  fs::path resources_dir = fs::path(RESOURCES_DIR);

  fs::path file1 = resources_dir / "BjarneStroustrup.jpg";
  fs::path file2 = resources_dir / "Book.pdf";

  ASSERT_TRUE(fs::exists(file1));
  ASSERT_TRUE(fs::exists(file2));

  // Temporary working directory
  auto now = std::chrono::steady_clock::now().time_since_epoch().count();
  fs::path work_dir =
      fs::temp_directory_path() / ("hamarc_test_" + std::to_string(now));
  fs::path out_dir = work_dir / "out";
  ASSERT_TRUE(fs::create_directories(out_dir));

  fs::path archive = work_dir / "archive.haf";

  // Path to executable
  std::string hamarc = HAMARC_EXE_PATH;

  // -------------------- CREATE ----------------------------------------------
  {
    std::ostringstream cmd;
    cmd << QuotePath(hamarc)
        << " --create"
        << " --file=" << QuotePath(archive)
        << " " << QuotePath(file1)
        << " " << QuotePath(file2);

    std::cout << "Create: " << cmd.str() << std::endl;

    int rc = std::system(cmd.str().c_str());
    ASSERT_EQ(rc, 0);
    ASSERT_TRUE(fs::exists(archive));
  }

  // -------------------- CORRUPT 3 BYTES -------------------------------------
  {
    std::uintmax_t archive_size = fs::file_size(archive);
    std::fstream file(archive, std::ios::in | std::ios::out | std::ios::binary);
    ASSERT_TRUE(file.is_open());

    std::vector<std::pair<std::uintmax_t, int>> flips = {
        {100, 0},
        {archive_size / 2, 0},
        {archive_size - 1, 0},
    };

    for (auto [byte_pos, bit] : flips) {
      file.seekg(static_cast<std::streamoff>(byte_pos));
      char ch;
      file.read(&ch, 1);
      if (file.gcount() != 1) continue;

      ch ^= (1 << bit);
      file.seekp(static_cast<std::streamoff>(byte_pos));
      file.write(&ch, 1);
    }
  }

  // -------------------- EXTRACT ---------------------------------------------
  {
    fs::path original = fs::current_path();
    fs::current_path(out_dir);

    std::ostringstream cmd;
    cmd << QuotePath(hamarc)
        << " --extract"
        << " --file=" << QuotePath(work_dir / "archive.haf");

    int rc = std::system(cmd.str().c_str());
    fs::current_path(original);

    ASSERT_EQ(rc, 0);
  }

  fs::path out1 = out_dir / file1.filename();
  fs::path out2 = out_dir / file2.filename();

  ASSERT_TRUE(fs::exists(out1));
  ASSERT_TRUE(fs::exists(out2));

  EXPECT_TRUE(FilesEqual(file1, out1));
  EXPECT_TRUE(FilesEqual(file2, out2));
}