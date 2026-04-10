#include <algorithm>
#include <cstddef>
#include <cctype>
#include <iostream>
#include <string>
#include <utility>

#include <processing.h>

int main(int argc, char** argv) {
	if (argc != 2) {
		return 1;
	}

	const bool recursive = false;
	Dir(argv[1], recursive)
		| Filter([](const std::filesystem::path& path) { return path.extension() == ".txt"; })
		| OpenFiles()
		| Split("\n ,.;")
		| Transform([](std::string token) {
			std::transform(token.begin(), token.end(), token.begin(), [](unsigned char ch) {
				return static_cast<char>(std::tolower(ch));
			});
			return token;
		})
		| AggregateByKey(
			std::size_t{0},
			[](const std::string&, std::size_t& count) { ++count; },
			[](const std::string& token) { return token; })
		| Transform([](const std::pair<std::string, std::size_t>& stat) {
			return stat.first + " - " + std::to_string(stat.second);
		})
		| Out(std::cout);

	return 0;
}
