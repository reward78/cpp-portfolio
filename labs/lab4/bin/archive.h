#pragma once

#include <cstdint>

struct FileInfo {
    const char* name;
    std::uint32_t size;
};

void CreateArchive(const char* archive_path, char** argv, int filecount);
void ListArchive(const char* archive_path);
void AppendArchive(const char* archive_path, char** argv, int filecount);
void DeleteArchive(const char* archive_path, char** argv, int delCount);
void ExtractArchive(const char* archive_path);
void ConcatenateArchives(const char* archive1,
                         const char* archive2,
                         const char* archive_out);