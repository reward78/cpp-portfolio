#include <iostream>
#include "argparser.h"

int main(int argc, const char* const* argv) {
    using namespace nargparse;

    ArgumentParser parser = CreateParser("myprog");

    bool verbose = false;
    int count = 0;
    float ratio = 0.0f;
    char output[128];
    char dummy[1]; // для позиционных строк

    AddFlag(parser, "-v", "--verbose", &verbose, "enable verbose output");

    AddArgument(parser, "-c", "--count", &count, "items count",
                kNargsOptional, nullptr, nullptr);

    AddArgument(parser, nullptr, "--ratio", &ratio, "ratio", 
                kNargsOptional, nullptr, nullptr);

    AddArgument(parser, nullptr, "--output", (void*)output, "output file",
                kNargsOptional, nullptr, nullptr);

    // ПРАВИЛЬНАЯ строковая регистрация позиционных аргументов
    AddArgument(parser, (void*)dummy, "files", kNargsZeroOrMore,
                nullptr, nullptr);

    AddHelp(parser);

    if (!Parse(parser, argc, argv)) {
        std::cout << "Parse error\n";
        PrintHelp(parser);
        FreeParser(parser);
        return 1;
    }

    std::cout << "verbose=" << verbose << "\n";
    std::cout << "count=" << count << "\n";
    std::cout << "ratio=" << ratio << "\n";
    std::cout << "output=" << output << "\n";

    int files_count = GetRepeatedCount(parser, "files");
    std::cout << "files_count=" << files_count << "\n";

    for (int i = 0; i < files_count; ++i) {
        const char* f;
        GetRepeated(parser, "files", i, &f);
        std::cout << " file[" << i << "] = " << f << "\n";
    }

    FreeParser(parser);
    return 0;
}