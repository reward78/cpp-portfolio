#include "cli.h"

#include <cstring>

void Parse(int argc,
           char* argv[],
           const char*& command,
           const char*& archive1,
           const char*& archive2,
           const char*& archive3,
           int& first_extra) {
    int i = 1;
    while (i < argc) {
        char* arg = argv[i];

        if (std::strcmp(arg, "-c") == 0 || std::strcmp(arg, "--create") == 0 ||
            std::strcmp(arg, "-l") == 0 || std::strcmp(arg, "--list") == 0 ||
            std::strcmp(arg, "-x") == 0 || std::strcmp(arg, "--extract") == 0 ||
            std::strcmp(arg, "-a") == 0 || std::strcmp(arg, "--append") == 0 ||
            std::strcmp(arg, "-d") == 0 || std::strcmp(arg, "--delete") == 0) {
            if (arg[1] == '-') {
                command = arg + 2;
            } else {
                command = arg + 1;
            }
            ++i;
            continue;
        }

        if (std::strcmp(arg, "-A") == 0 ||
            std::strcmp(arg, "--concatenate") == 0) {
            if (arg[1] == '-') {
                command = arg + 2;
            } else {
                command = arg + 1;
            }
            ++i;
            continue;
        }

        if (std::strcmp(arg, "-f") == 0) {
            ++i;
            if (i < argc) {
                archive3 = argv[i];
                ++i;
            }
            continue;
        }

        if (std::strncmp(arg, "--file=", 7) == 0) {
            archive3 = arg + 7;
            ++i;
            continue;
        }

        if (command &&
            (std::strcmp(command, "A") == 0 ||
             std::strcmp(command, "concatenate") == 0)) {
            if (!archive1) {
                archive1 = arg;
                ++i;
                continue;
            }
            if (!archive2) {
                archive2 = arg;
                ++i;
                continue;
            }
            first_extra = i;
            return;
        }

        if (!archive3 && command &&
            (std::strcmp(command, "c") == 0 ||
             std::strcmp(command, "create") == 0 ||
             std::strcmp(command, "l") == 0 ||
             std::strcmp(command, "list") == 0 ||
             std::strcmp(command, "x") == 0 ||
             std::strcmp(command, "extract") == 0 ||
             std::strcmp(command, "a") == 0 ||
             std::strcmp(command, "append") == 0 ||
             std::strcmp(command, "d") == 0 ||
             std::strcmp(command, "delete") == 0)) {
            archive3 = arg;
            ++i;
            first_extra = i;
            return;
        }

        first_extra = i;
        return;
    }

    first_extra = i;
}