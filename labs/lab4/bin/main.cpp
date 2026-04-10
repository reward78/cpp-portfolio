#include <cstring>
#include <iostream>

#include "archive.h"
#include "cli.h"

int main(int argc, char* argv[]) {
    const char* command = nullptr;
    const char* archive1 = nullptr;
    const char* archive2 = nullptr;
    const char* archive3 = nullptr;
    int first_extra = argc; 

    Parse(argc, argv, command, archive1, archive2, archive3, first_extra);

    std::cout << (command ? command : "(no cmd)") << ' '
              << (archive1 ? archive1 : " ") << ' '
              << (archive2 ? archive2 : " ") << ' '
              << (archive3 ? archive3 : " ") << '\n';

    if (command &&
        (std::strcmp(command, "c") == 0 ||
         std::strcmp(command, "create") == 0)) {
        CreateArchive(archive3, argv + first_extra, argc - first_extra);
    }

    if (command &&
        (std::strcmp(command, "l") == 0 ||
         std::strcmp(command, "list") == 0)) {
        if (archive3) {
            ListArchive(archive3);
        }
    }

    if (command &&
        (std::strcmp(command, "x") == 0 ||
         std::strcmp(command, "extract") == 0)) {
        ExtractArchive(archive3);
    }

    if (command &&
        (std::strcmp(command, "a") == 0 ||
         std::strcmp(command, "append") == 0)) {
        AppendArchive(archive3, argv + first_extra, argc - first_extra);
    }

    if (command &&
        (std::strcmp(command, "d") == 0 ||
         std::strcmp(command, "delete") == 0)) {
        DeleteArchive(archive3, argv + first_extra, argc - first_extra);
    }

    if (command &&
        (std::strcmp(command, "A") == 0 ||
         std::strcmp(command, "concatenate") == 0)) {
        if (archive1 && archive2 && archive3) {
            ConcatenateArchives(archive1, archive2, archive3);
        }
    }

    return 0;
}