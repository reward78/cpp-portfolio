#pragma once

#include <stddef.h>

namespace nargparse {

enum Nargs {
  kNargsOptional = 0,
  kNargsRequired = 1,
  kNargsZeroOrMore = 2,
  kNargsOneOrMore = 3
};

struct ArgumentParser {
  void* impl;
};

/* construction / destruction */
ArgumentParser CreateParser(const char* program_name, size_t max_string_len);
ArgumentParser CreateParser(const char* program_name);  // перегрузка
void FreeParser(ArgumentParser& parser);

/* flags */
bool AddFlag(ArgumentParser& parser,
             const char* short_name,
             const char* long_name,
             bool* target,
             const char* help,
             bool default_value = false);

/* positional arguments (int) */
bool AddArgument(ArgumentParser& parser,
                 int* target_first_value,
                 const char* name,
                 Nargs nargs = kNargsOptional,
                 bool (*validator)(const int&) = 0,
                 const char* error_msg = 0);

/* positional arguments (float) */
bool AddArgument(ArgumentParser& parser,
                 float* target_first_value,
                 const char* name,
                 Nargs nargs = kNargsOptional,
                 bool (*validator)(const float&) = 0,
                 const char* error_msg = 0);

/* positional arguments (C-string; передаётся адрес массива: &buf) */
bool AddArgument(ArgumentParser& parser,
                 void* target_first_cstr_addr,
                 const char* name,
                 Nargs nargs = kNargsOptional,
                 bool (*validator)(const char* const&) = 0,
                 const char* error_msg = 0);

/* named arguments (int) */
bool AddArgument(ArgumentParser& parser,
                 const char* short_name,
                 const char* long_name,
                 int* target_first_value,
                 const char* help,
                 Nargs nargs = kNargsOptional,
                 bool (*validator)(const int&) = 0,
                 const char* error_msg = 0);

/* named arguments (float) */
bool AddArgument(ArgumentParser& parser,
                 const char* short_name,
                 const char* long_name,
                 float* target_first_value,
                 const char* help,
                 Nargs nargs = kNargsOptional,
                 bool (*validator)(const float&) = 0,
                 const char* error_msg = 0);

/* named arguments (C-string; передаётся адрес массива: &buf) */
bool AddArgument(ArgumentParser& parser,
                 const char* short_name,
                 const char* long_name,
                 void* target_first_cstr_addr,
                 const char* help,
                 Nargs nargs = kNargsOptional,
                 bool (*validator)(const char* const&) = 0,
                 const char* error_msg = 0);

/* help */
bool AddHelp(ArgumentParser& parser);
bool PrintHelp(ArgumentParser& parser);

/* parsing */
bool Parse(ArgumentParser& parser, int argc, const char* const* argv);

/* repeated accessors */
int GetRepeatedCount(ArgumentParser& parser, const char* name);
bool GetRepeated(ArgumentParser& parser, const char* name, int index, int* out_value);
bool GetRepeated(ArgumentParser& parser, const char* name, int index, float* out_value);
bool GetRepeated(ArgumentParser& parser, const char* name, int index, const char** out_value);

}  // namespace nargparse