#pragma once

void Parse(int argc,
           char* argv[],
           const char*& command,
           const char*& archive1,
           const char*& archive2,
           const char*& archive3,
           int& first_extra);