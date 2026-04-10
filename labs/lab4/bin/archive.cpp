#include "archive.h"

#include <cstdint>
#include <cstring>
#include <fstream>
#include <ios>
#include <iostream>
    
#include "hamming_codec.h"

void CreateArchive(const char* archive_path, char** argv, int file_count) {
    std::ofstream out(archive_path, std::ios::binary);
    if (!out) {
        return;
    }

    out.put('H');
    out.put('A');
    out.put('F');
    out.put('1');

    out.write(reinterpret_cast<const char*>(&file_count), sizeof(file_count));

    FileInfo* infos = new FileInfo[file_count];

    for (int i = 0; i < file_count; ++i) {
        const char* path = argv[i];
        std::ifstream in(path, std::ios::binary);
        std::uint32_t size = 0;

        if (in) {
            in.seekg(0, std::ios::end);
            std::streampos pos = in.tellg();
            size = static_cast<std::uint32_t>(pos);
            in.close();
        }

        const char* base = path;
        for (const char* p = path; *p; ++p) {
            if (*p == '/' || *p == '\\') {
                base = p + 1;
            }
        }

        infos[i].name = base;
        infos[i].size = size;
    }

    for (int i = 0; i < file_count; ++i) {
        const char* name = infos[i].name;
        std::uint32_t name_len = std::strlen(name);
        std::uint32_t size = infos[i].size;

        out.write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
        out.write(name, name_len);
        out.write(reinterpret_cast<const char*>(&size), sizeof(size));
    }

    HammingCodec codec;
    for (int i = 0; i < file_count; ++i) {
        const char* path = argv[i];
        std::ifstream in(path, std::ios::binary);
        if (!in) {
            continue;
        }
        std::uint8_t b;
        while (in.read(reinterpret_cast<char*>(&b), 1)) {
            std::uint16_t code = codec.EncodeByte(b);
            out.write(reinterpret_cast<char*>(&code), sizeof(code));
        }
    }

    delete[] infos;
}

void ListArchive(const char* archive_path) {
    std::ifstream in(archive_path, std::ios::binary);
    if (!in) {
        return;
    }

    char h = in.get();
    char a = in.get();
    char f = in.get();
    char v = in.get();

    if (h != 'H' || a != 'A' || f != 'F' || v != '1') {
        return;
    }

    std::uint32_t file_count = 0;
    in.read(reinterpret_cast<char*>(&file_count), sizeof(file_count));

    for (std::uint32_t i = 0; i < file_count; ++i) {
        std::uint32_t name_len = 0;
        in.read(reinterpret_cast<char*>(&name_len), sizeof(name_len));

        char* name = new char[name_len + 1];
        in.read(name, name_len);
        name[name_len] = '\0';

        std::uint32_t size = 0;
        in.read(reinterpret_cast<char*>(&size), sizeof(size));

        std::cout << name << " " << size << "\n";

        delete[] name;
    }
}

void AppendArchive(const char* archive_path, char** argv, int file_count) {
    std::ifstream in(archive_path, std::ios::binary);
    if (!in) {
        return;
    }

    char h = in.get();
    char a = in.get();
    char f = in.get();
    char v = in.get();
    if (h != 'H' || a != 'A' || f != 'F' || v != '1') {
        return;
    }

    std::uint32_t file_count_old = 0;
    in.read(reinterpret_cast<char*>(&file_count_old), sizeof(file_count_old));
    if (!in) {
        return;
    }

    FileInfo* old_infos = new FileInfo[file_count_old];
    for (std::uint32_t i = 0; i < file_count_old; ++i) {
        std::uint32_t name_len = 0;
        in.read(reinterpret_cast<char*>(&name_len), sizeof(name_len));
        if (!in) {
            delete[] old_infos;
            return;
        }

        char* name = new char[name_len + 1];
        in.read(name, name_len);
        if (!in) {
            delete[] name;
            delete[] old_infos;
            return;
        }
        name[name_len] = '\0';

        std::uint32_t size = 0;
        in.read(reinterpret_cast<char*>(&size), sizeof(size));
        if (!in) {
            delete[] name;
            delete[] old_infos;
            return;
        }

        old_infos[i].name = name;
        old_infos[i].size = size;
    }

    std::streampos data_start = in.tellg();
    in.seekg(0, std::ios::end);
    std::streampos data_end = in.tellg();
    std::streamsize data_size = data_end - data_start;

    char* old_data = nullptr;
    if (data_size > 0) {
        old_data = new char[data_size];
        in.seekg(data_start);
        in.read(old_data, data_size);
        if (!in) {
            delete[] old_data;
            for (std::uint32_t i = 0; i < file_count_old; ++i) {
                delete[] const_cast<char*>(old_infos[i].name);
            }
            delete[] old_infos;
            return;
        }
    }

    in.close();

    FileInfo* new_infos = new FileInfo[file_count];
    for (int i = 0; i < file_count; ++i) {
        const char* path = argv[i];
        std::ifstream file(path, std::ios::binary);

        std::uint32_t size = 0;
        if (file) {
            file.seekg(0, std::ios::end);
            size = static_cast<std::uint32_t>(file.tellg());
            file.close();
        }

        const char* base = path;
        for (const char* p = path; *p; ++p) {
            if (*p == '/' || *p == '\\') {
                base = p + 1;
            }
        }

        new_infos[i].name = base;
        new_infos[i].size = size;
    }

    std::ofstream out(archive_path, std::ios::binary);
    if (!out) {
        delete[] new_infos;
        for (std::uint32_t i = 0; i < file_count_old; ++i) {
            delete[] const_cast<char*>(old_infos[i].name);
        }
        delete[] old_infos;
        delete[] old_data;
        return;
    }

    out.put('H');
    out.put('A');
    out.put('F');
    out.put('1');

    std::uint32_t file_count_new =
        file_count_old + static_cast<std::uint32_t>(file_count);
    out.write(reinterpret_cast<const char*>(&file_count_new),
              sizeof(file_count_new));

    for (std::uint32_t i = 0; i < file_count_old; ++i) {
        const char* name = old_infos[i].name;
        std::uint32_t name_len = std::strlen(name);
        std::uint32_t size = old_infos[i].size;

        out.write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
        out.write(name, name_len);
        out.write(reinterpret_cast<const char*>(&size), sizeof(size));
    }

    for (int i = 0; i < file_count; ++i) {
        const char* name = new_infos[i].name;
        std::uint32_t name_len = std::strlen(name);
        std::uint32_t size = new_infos[i].size;

        out.write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
        out.write(name, name_len);
        out.write(reinterpret_cast<const char*>(&size), sizeof(size));
    }

    if (old_data && data_size > 0) {
        out.write(old_data, data_size);
    }

    HammingCodec codec;
    for (int i = 0; i < file_count; ++i) {
        const char* path = argv[i];
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            continue;
        }

        std::uint8_t b;
        while (file.read(reinterpret_cast<char*>(&b), 1)) {
            std::uint16_t code = codec.EncodeByte(b);
            out.write(reinterpret_cast<char*>(&code), sizeof(code));
        }
    }

    delete[] new_infos;
    for (std::uint32_t i = 0; i < file_count_old; ++i) {
        delete[] const_cast<char*>(old_infos[i].name);
    }
    delete[] old_infos;
    delete[] old_data;
}

void DeleteArchive(const char* archive_path, char** argv, int delCount) {
    std::ifstream in(archive_path, std::ios::binary);
    if (!in) {
        return;
    }

    char h = in.get();
    char a = in.get();
    char f = in.get();
    char v = in.get();
    if (h != 'H' || a != 'A' || f != 'F' || v != '1') {
        return;
    }

    std::uint32_t file_count_old = 0;
    in.read(reinterpret_cast<char*>(&file_count_old), sizeof(file_count_old));
    if (!in) {
        return;
    }

    FileInfo* old_infos = new FileInfo[file_count_old];
    for (std::uint32_t i = 0; i < file_count_old; ++i) {
        std::uint32_t name_len = 0;
        in.read(reinterpret_cast<char*>(&name_len), sizeof(name_len));
        if (!in) {
            delete[] old_infos;
            return;
        }

        char* name = new char[name_len + 1];
        in.read(name, name_len);
        if (!in) {
            delete[] name;
            delete[] old_infos;
            return;
        }
        name[name_len] = '\0';

        std::uint32_t size = 0;
        in.read(reinterpret_cast<char*>(&size), sizeof(size));
        if (!in) {
            delete[] name;
            delete[] old_infos;
            return;
        }

        old_infos[i].name = name;
        old_infos[i].size = size;
    }

    std::streampos data_start = in.tellg();
    in.seekg(0, std::ios::end);
    std::streampos data_end = in.tellg();
    std::streamsize data_size = data_end - data_start;

    char* old_data = nullptr;
    if (data_size > 0) {
        old_data = new char[data_size];
        in.seekg(data_start);
        in.read(old_data, data_size);
        if (!in) {
            delete[] old_data;
            for (std::uint32_t i = 0; i < file_count_old; ++i) {
                delete[] const_cast<char*>(old_infos[i].name);
            }
            delete[] old_infos;
            return;
        }
    }

    in.close();

    bool* keep = new bool[file_count_old];
    for (std::uint32_t i = 0; i < file_count_old; ++i) {
        keep[i] = true;
        for (int j = 0; j < delCount; ++j) {
            if (std::strcmp(old_infos[i].name, argv[j]) == 0) {
                keep[i] = false;
                break;
            }
        }
    }

    std::uint32_t file_count_new = 0;
    for (std::uint32_t i = 0; i < file_count_old; ++i) {
        if (keep[i]) {
            ++file_count_new;
        }
    }

    std::ofstream out(archive_path, std::ios::binary);
    if (!out) {
        delete[] old_data;
        for (std::uint32_t i = 0; i < file_count_old; ++i) {
            delete[] const_cast<char*>(old_infos[i].name);
        }
        delete[] old_infos;
        delete[] keep;
        return;
    }

    out.put('H');
    out.put('A');
    out.put('F');
    out.put('1');
    out.write(reinterpret_cast<const char*>(&file_count_new),
              sizeof(file_count_new));

    for (std::uint32_t i = 0; i < file_count_old; ++i) {
        if (!keep[i]) {
            continue;
        }
        const char* name = old_infos[i].name;
        std::uint32_t name_len = std::strlen(name);
        std::uint32_t size = old_infos[i].size;

        out.write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
        out.write(name, name_len);
        out.write(reinterpret_cast<const char*>(&size), sizeof(size));
    }

    if (old_data && data_size > 0) {
        std::uint32_t offset = 0;
        for (std::uint32_t i = 0; i < file_count_old; ++i) {
            std::uint32_t bytes_for_file =
                old_infos[i].size * sizeof(std::uint16_t);
            if (keep[i] && bytes_for_file > 0) {
                out.write(old_data + offset, bytes_for_file);
            }
            offset += bytes_for_file;
        }
    }

    delete[] old_data;
    for (std::uint32_t i = 0; i < file_count_old; ++i) {
        delete[] const_cast<char*>(old_infos[i].name);
    }
    delete[] old_infos;
    delete[] keep;
}

void ExtractArchive(const char* archive_path) {
    std::ifstream in(archive_path, std::ios::binary);
    if (!in) {
        return;
    }

    char h = in.get();
    char a = in.get();
    char f = in.get();
    char v = in.get();

    if (h != 'H' || a != 'A' || f != 'F' || v != '1') {
        return;
    }

    std::uint32_t file_count = 0;
    in.read(reinterpret_cast<char*>(&file_count), sizeof(file_count));
    if (!in) {
        return;
    }

    FileInfo* infos = new FileInfo[file_count];

    for (std::uint32_t i = 0; i < file_count; ++i) {
        std::uint32_t name_len = 0;
        in.read(reinterpret_cast<char*>(&name_len), sizeof(name_len));
        if (!in) {
            delete[] infos;
            return;
        }

        char* name = new char[name_len + 1];
        in.read(name, name_len);
        if (!in) {
            delete[] infos;
            delete[] name;
            return;
        }
        name[name_len] = '\0';

        std::uint32_t size = 0;
        in.read(reinterpret_cast<char*>(&size), sizeof(size));
        if (!in) {
            delete[] infos;
            delete[] name;
            return;
        }

        infos[i].name = name;
        infos[i].size = size;
    }

    HammingCodec codec;

    for (std::uint32_t i = 0; i < file_count; ++i) {
        const char* name = infos[i].name;
        std::uint32_t size = infos[i].size;

        std::ofstream out_file(name, std::ios::binary);
        if (!out_file) {
            continue;
        }

        for (std::uint32_t j = 0; j < size; ++j) {
            std::uint16_t code = 0;
            in.read(reinterpret_cast<char*>(&code), sizeof(code));
            if (!in) {
                break;
            }

            bool corrected = false;
            bool uncorrected = false;
            std::uint8_t b = codec.DecodeCodeWord(code, corrected, uncorrected);

            out_file.write(reinterpret_cast<char*>(&b), 1);
        }
    }

    for (std::uint32_t i = 0; i < file_count; ++i) {
        delete[] infos[i].name;
    }
    delete[] infos;
}

void ConcatenateArchives(const char* archive1,
                         const char* archive2,
                         const char* archive_out) {
    std::ifstream in1(archive1, std::ios::binary);
    if (!in1) {
        return;
    }

    char h = in1.get();
    char a = in1.get();
    char f = in1.get();
    char v = in1.get();
    if (h != 'H' || a != 'A' || f != 'F' || v != '1') {
        return;
    }

    std::uint32_t file_count_1 = 0;
    in1.read(reinterpret_cast<char*>(&file_count_1), sizeof(file_count_1));
    if (!in1) {
        return;
    }

    FileInfo* infos1 = new FileInfo[file_count_1];
    for (std::uint32_t i = 0; i < file_count_1; ++i) {
        std::uint32_t name_len = 0;
        in1.read(reinterpret_cast<char*>(&name_len), sizeof(name_len));
        if (!in1) {
            delete[] infos1;
            return;
        }

        char* name = new char[name_len + 1];
        in1.read(name, name_len);
        if (!in1) {
            delete[] name;
            delete[] infos1;
            return;
        }
        name[name_len] = '\0';

        std::uint32_t size = 0;
        in1.read(reinterpret_cast<char*>(&size), sizeof(size));
        if (!in1) {
            delete[] name;
            delete[] infos1;
            return;
        }

        infos1[i].name = name;
        infos1[i].size = size;
    }

    std::streampos data_start1 = in1.tellg();
    in1.seekg(0, std::ios::end);
    std::streampos data_end1 = in1.tellg();
    std::streamsize data_size1 = data_end1 - data_start1;

    char* data1 = nullptr;
    if (data_size1 > 0) {
        data1 = new char[data_size1];
        in1.seekg(data_start1);
        in1.read(data1, data_size1);
        if (!in1) {
            delete[] data1;
            for (std::uint32_t i = 0; i < file_count_1; ++i) {
                delete[] const_cast<char*>(infos1[i].name);
            }
            delete[] infos1;
            return;
        }
    }
    in1.close();

    std::ifstream in2(archive2, std::ios::binary);
    if (!in2) {
        delete[] data1;
        for (std::uint32_t i = 0; i < file_count_1; ++i) {
            delete[] const_cast<char*>(infos1[i].name);
        }
        delete[] infos1;
        return;
    }

    h = in2.get();
    a = in2.get();
    f = in2.get();
    v = in2.get();
    if (h != 'H' || a != 'A' || f != 'F' || v != '1') {
        delete[] data1;
        for (std::uint32_t i = 0; i < file_count_1; ++i) {
            delete[] const_cast<char*>(infos1[i].name);
        }
        delete[] infos1;
        return;
    }

    std::uint32_t file_count_2 = 0;
    in2.read(reinterpret_cast<char*>(&file_count_2), sizeof(file_count_2));
    if (!in2) {
        delete[] data1;
        for (std::uint32_t i = 0; i < file_count_1; ++i) {
            delete[] const_cast<char*>(infos1[i].name);
        }
        delete[] infos1;
        return;
    }

    FileInfo* infos2 = new FileInfo[file_count_2];
    for (std::uint32_t i = 0; i < file_count_2; ++i) {
        std::uint32_t name_len = 0;
        in2.read(reinterpret_cast<char*>(&name_len), sizeof(name_len));
        if (!in2) {
            delete[] infos2;
            delete[] data1;
            for (std::uint32_t k = 0; k < file_count_1; ++k) {
                delete[] const_cast<char*>(infos1[k].name);
            }
            delete[] infos1;
            return;
        }

        char* name = new char[name_len + 1];
        in2.read(name, name_len);
        if (!in2) {
            delete[] name;
            delete[] infos2;
            delete[] data1;
            for (std::uint32_t k = 0; k < file_count_1; ++k) {
                delete[] const_cast<char*>(infos1[k].name);
            }
            delete[] infos1;
            return;
        }
        name[name_len] = '\0';

        std::uint32_t size = 0;
        in2.read(reinterpret_cast<char*>(&size), sizeof(size));
        if (!in2) {
            delete[] name;
            delete[] infos2;
            delete[] data1;
            for (std::uint32_t k = 0; k < file_count_1; ++k) {
                delete[] const_cast<char*>(infos1[k].name);
            }
            delete[] infos1;
            return;
        }

        infos2[i].name = name;
        infos2[i].size = size;
    }

    std::streampos data_start2 = in2.tellg();
    in2.seekg(0, std::ios::end);
    std::streampos data_end2 = in2.tellg();
    std::streamsize data_size2 = data_end2 - data_start2;

    char* data2 = nullptr;
    if (data_size2 > 0) {
        data2 = new char[data_size2];
        in2.seekg(data_start2);
        in2.read(data2, data_size2);
        if (!in2) {
            delete[] data2;
            delete[] infos2;
            delete[] data1;
            for (std::uint32_t k = 0; k < file_count_1; ++k) {
                delete[] const_cast<char*>(infos1[k].name);
            }
            delete[] infos1;
            return;
        }
    }
    in2.close();

    std::ofstream out(archive_out, std::ios::binary);
    if (!out) {
        delete[] data1;
        delete[] data2;
        for (std::uint32_t i = 0; i < file_count_1; ++i) {
            delete[] const_cast<char*>(infos1[i].name);
        }
        for (std::uint32_t i = 0; i < file_count_2; ++i) {
            delete[] const_cast<char*>(infos2[i].name);
        }
        delete[] infos1;
        delete[] infos2;
        return;
    }

    out.put('H');
    out.put('A');
    out.put('F');
    out.put('1');

    std::uint32_t file_count_new = file_count_1 + file_count_2;
    out.write(reinterpret_cast<const char*>(&file_count_new),
              sizeof(file_count_new));

    for (std::uint32_t i = 0; i < file_count_1; ++i) {
        const char* name = infos1[i].name;
        std::uint32_t name_len = std::strlen(name);
        std::uint32_t size = infos1[i].size;

        out.write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
        out.write(name, name_len);
        out.write(reinterpret_cast<const char*>(&size), sizeof(size));
    }

    for (std::uint32_t i = 0; i < file_count_2; ++i) {
        const char* name = infos2[i].name;
        std::uint32_t name_len = std::strlen(name);
        std::uint32_t size = infos2[i].size;

        out.write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
        out.write(name, name_len);
        out.write(reinterpret_cast<const char*>(&size), sizeof(size));
    }

    if (data1 && data_size1 > 0) {
        out.write(data1, data_size1);
    }
    if (data2 && data_size2 > 0) {
        out.write(data2, data_size2);
    }

    delete[] data1;
    delete[] data2;

    for (std::uint32_t i = 0; i < file_count_1; ++i) {
        delete[] const_cast<char*>(infos1[i].name);
    }
    for (std::uint32_t i = 0; i < file_count_2; ++i) {
        delete[] const_cast<char*>(infos2[i].name);
    }
    delete[] infos1;
    delete[] infos2;
}