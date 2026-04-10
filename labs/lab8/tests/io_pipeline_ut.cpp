#include <processing.h>

#include <gtest/gtest.h>

#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

TEST(IOPipelineTest, DirOpenFilesAndOutWorkTogether) {
    const auto base_dir = std::filesystem::temp_directory_path() / "labwork8_io_pipeline_test";
    const auto nested_dir = base_dir / "nested";

    std::filesystem::remove_all(base_dir);
    std::filesystem::create_directories(nested_dir);

    {
        std::ofstream(base_dir / "a.txt") << "Alpha beta";
        std::ofstream(nested_dir / "b.txt") << "Gamma";
        std::ofstream(base_dir / "skip.md") << "ignored";
    }

    std::stringstream output;

    Dir(base_dir, true)
        | Filter([](const std::filesystem::path& path) { return path.extension() == ".txt"; })
        | OpenFiles()
        | Split(" ")
        | Transform([](std::string token) {
            for (char& ch : token) {
                ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            }
            return token;
        })
        | Out(output);

    EXPECT_EQ(output.str(), "alpha\nbeta\ngamma\n");

    std::filesystem::remove_all(base_dir);
}
