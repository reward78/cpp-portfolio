#include "argparser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace nargparse {

enum { kMaxSpecs = 128 };
enum { kMaxRepeated = 512 };
enum { kDefaultMaxStringLen = 128 };

enum ArgType { kInt = 1, kFloat = 2, kCstr = 3 };

struct RepeatedStorage {
  int count;
  int ints[kMaxRepeated];
  float floats[kMaxRepeated];
  char** cstrs;
};

struct FlagSpec {
  const char* short_name;
  const char* long_name;
  bool* target;
  bool default_value;
  const char* help;
};

struct ArgSpec {
  const char* name_for_get;
  const char* short_name;
  const char* long_name;
  int type;
  void* first_target;
  int nargs_mode;
  bool (*int_validator)(const int&);
  bool (*float_validator)(const float&);
  bool (*cstr_validator)(const char* const&);
  const char* error_msg;
  RepeatedStorage store;
};

struct HelpSpec {
  int enabled;
  const char* short_name;
  const char* long_name;
};

struct Impl {
  char program[kDefaultMaxStringLen];
  size_t max_str_len;
  FlagSpec flags[kMaxSpecs];
  int flags_count;
  ArgSpec args[kMaxSpecs];
  int args_count;
  HelpSpec help;
};

static int StrEq(const char* a, const char* b) {
  if (!a || !b) return 0;
  return strcmp(a, b) == 0;
}

static int StartsWith(const char* s, const char* pref) {
  if (!s || !pref) return 0;
  size_t ls = strlen(s);
  size_t lp = strlen(pref);
  if (lp > ls) return 0;
  return strncmp(s, pref, lp) == 0;
}

static int IsLongOpt(const char* tok) { return StartsWith(tok, "--"); }
static int IsShortOpt(const char* tok) {
  return (tok && tok[0] == '-' && tok[1] && tok[1] != '-');
}

static ArgSpec* FindArgByOpt(Impl* p, const char* tok, int* out_equal_pos) {
  if (!tok) return 0;
  int eqpos = -1;
  int i = 0;
  while (tok[i]) {
    if (tok[i] == '=') {
      eqpos = i;
      break;
    }
    ++i;
  }
  if (out_equal_pos) *out_equal_pos = eqpos;

  i = 0;
  while (i < p->args_count) {
    ArgSpec* a = &p->args[i];
    if (a->long_name && IsLongOpt(tok)) {
      if (eqpos >= 0) {
        if ((int)strlen(a->long_name) == eqpos &&
            strncmp(tok, a->long_name, (size_t)eqpos) == 0)
          return a;
      } else {
        if (StrEq(tok, a->long_name)) return a;
      }
    }
    if (a->short_name && IsShortOpt(tok) && eqpos < 0) {
      if (StrEq(tok, a->short_name)) return a;
    }
    ++i;
  }
  return 0;
}

static FlagSpec* FindFlagByOpt(Impl* p, const char* tok) {
  int i = 0;
  while (i < p->flags_count) {
    FlagSpec* f = &p->flags[i];
    if ((f->long_name && StrEq(tok, f->long_name)) ||
        (f->short_name && StrEq(tok, f->short_name)))
      return f;
    ++i;
  }
  return 0;
}

static ArgSpec* FindArgByName(Impl* p, const char* name) {
  int i = 0;
  while (i < p->args_count) {
    if (p->args[i].name_for_get && StrEq(p->args[i].name_for_get, name)) return &p->args[i];
    ++i;
  }
  return 0;
}

static int EnsureCstrStore(Impl* p, ArgSpec* a) {
  if (a->type != kCstr) return 1;
  if (a->store.cstrs) return 1;

  char** arr = (char**)malloc(sizeof(char*) * kMaxRepeated);
  if (!arr) return 0;

  int i = 0;
  while (i < kMaxRepeated) {
    arr[i] = (char*)malloc(p->max_str_len);
    if (!arr[i]) return 0;
    arr[i][0] = '\0';
    ++i;
  }
  a->store.cstrs = arr;
  return 1;
}

static int PushValue(Impl* p, ArgSpec* a, const char* token) {
  if (a->store.count >= kMaxRepeated) return 0;

  if (a->type == kInt) {
    char* endp = 0;
    long v = strtol(token, &endp, 10);
    if (!token[0] || (endp && *endp != '\0')) return 0;
    int iv = (int)v;
    if (a->int_validator && !a->int_validator(iv)) return 0;
    if (a->store.count == 0 && a->first_target) *((int*)a->first_target) = iv;
    a->store.ints[a->store.count] = iv;
    a->store.count += 1;
    return 1;
  }

  if (a->type == kFloat) {
    char* endp = 0;
    double dv = strtod(token, &endp);
    if (!token[0] || (endp && *endp != '\0')) return 0;
    float v = (float)dv;
    if (a->float_validator && !a->float_validator(v)) return 0;
    if (a->store.count == 0 && a->first_target) *((float*)a->first_target) = v;
    a->store.floats[a->store.count] = v;
    a->store.count += 1;
    return 1;
  }

  if (a->type == kCstr) {
    size_t len = strlen(token);
    if (len >= p->max_str_len) return 0;
    if (!EnsureCstrStore(p, a)) return 0;
    if (a->cstr_validator && !a->cstr_validator(token)) return 0;

    if (a->store.count == 0 && a->first_target) {
      char* dst = (char*)a->first_target;
      if (dst) {
        dst[0] = '\0';
        strncpy(dst, token, p->max_str_len - 1);
        dst[p->max_str_len - 1] = '\0';
      }
    }
    strncpy(a->store.cstrs[a->store.count], token, p->max_str_len - 1);
    a->store.cstrs[a->store.count][p->max_str_len - 1] = '\0';
    a->store.count += 1;
    return 1;
  }
  return 0;
}

static int SetEmptyIfOptionalString(Impl* p, ArgSpec* a) {
  if (a->type != kCstr) return 0;
  if (a->nargs_mode != kNargsOptional) return 0;
  if (!EnsureCstrStore(p, a)) return 0;

  if (a->store.count == 0 && a->first_target) {
    char* dst = (char*)a->first_target;
    if (dst) dst[0] = '\0';
  }
  a->store.cstrs[a->store.count][0] = '\0';
  a->store.count += 1;
  return 1;
}

ArgumentParser CreateParser(const char* program_name, size_t max_string_len) {
  ArgumentParser ap;
  ap.impl = 0;

  Impl* p = (Impl*)malloc(sizeof(Impl));
  if (!p) return ap;
  memset(p, 0, sizeof(Impl));

  if (program_name) {
    size_t name_len = strlen(program_name);
    if (name_len >= sizeof(p->program)) name_len = sizeof(p->program) - 1;
    memcpy(p->program, program_name, name_len);
    p->program[name_len] = '\0';
  } else {
    p->program[0] = '\0';
  }

  p->max_str_len = (max_string_len == 0) ? 1u : max_string_len;
  p->flags_count = 0;
  p->args_count = 0;
  p->help.enabled = 0;
  p->help.short_name = "-h";
  p->help.long_name = "--help";

  ap.impl = p;
  return ap;
}

ArgumentParser CreateParser(const char* program_name) {
  return CreateParser(program_name, kDefaultMaxStringLen);
}

void FreeParser(ArgumentParser& parser) {
  Impl* p = (Impl*)parser.impl;
  if (!p) return;

  int i = 0;
  while (i < p->args_count) {
    ArgSpec* a = &p->args[i];
    if (a->type == kCstr && a->store.cstrs) {
      int j = 0;
      while (j < kMaxRepeated) {
        if (a->store.cstrs[j]) free(a->store.cstrs[j]);
        ++j;
      }
      free(a->store.cstrs);
      a->store.cstrs = 0;
    }
    ++i;
  }
  free(p);
  parser.impl = 0;
}

static ArgSpec* AddArgSpec(Impl* p) {
  if (p->args_count >= kMaxSpecs) return 0;
  ArgSpec* a = &p->args[p->args_count];
  memset(a, 0, sizeof(ArgSpec));
  a->store.count = 0;
  a->store.cstrs = 0;
  p->args_count += 1;
  return a;
}

static FlagSpec* AddFlagSpec(Impl* p) {
  if (p->flags_count >= kMaxSpecs) return 0;
  FlagSpec* f = &p->flags[p->flags_count];
  memset(f, 0, sizeof(FlagSpec));
  p->flags_count += 1;
  return f;
}

bool AddFlag(ArgumentParser& parser,
             const char* short_name,
             const char* long_name,
             bool* target,
             const char* help,
             bool default_value) {
  Impl* p = (Impl*)parser.impl;
  if (!p || !target) return false;
  FlagSpec* f = AddFlagSpec(p);
  if (!f) return false;
  f->short_name = short_name;
  f->long_name = long_name;
  f->target = target;
  f->help = help;
  f->default_value = default_value;
  *(f->target) = default_value;
  return true;
}

bool AddArgument(ArgumentParser& parser, int* target, const char* name, Nargs nargs,
                 bool (*validator)(const int&), const char* error_msg) {
  Impl* p = (Impl*)parser.impl;
  if (!p || !target || !name) return false;
  ArgSpec* a = AddArgSpec(p);
  if (!a) return false;
  a->name_for_get = name;
  a->short_name = 0;
  a->long_name = 0;
  a->type = kInt;
  a->first_target = target;
  a->nargs_mode = (int)nargs;
  a->int_validator = validator;
  a->float_validator = 0;
  a->cstr_validator = 0;
  a->error_msg = error_msg;
  return true;
}

bool AddArgument(ArgumentParser& parser, float* target, const char* name, Nargs nargs,
                 bool (*validator)(const float&), const char* error_msg) {
  Impl* p = (Impl*)parser.impl;
  if (!p || !target || !name) return false;
  ArgSpec* a = AddArgSpec(p);
  if (!a) return false;
  a->name_for_get = name;
  a->short_name = 0;
  a->long_name = 0;
  a->type = kFloat;
  a->first_target = target;
  a->nargs_mode = (int)nargs;
  a->int_validator = 0;
  a->float_validator = validator;
  a->cstr_validator = 0;
  a->error_msg = error_msg;
  return true;
}

bool AddArgument(ArgumentParser& parser, void* target_addr, const char* name, Nargs nargs,
                 bool (*validator)(const char* const&), const char* error_msg) {
  Impl* p = (Impl*)parser.impl;
  if (!p || !target_addr || !name) return false;
  ArgSpec* a = AddArgSpec(p);
  if (!a) return false;
  a->name_for_get = name;
  a->short_name = 0;
  a->long_name = 0;
  a->type = kCstr;
  a->first_target = target_addr;  // адрес начала буфера
  a->nargs_mode = (int)nargs;
  a->int_validator = 0;
  a->float_validator = 0;
  a->cstr_validator = validator;
  a->error_msg = error_msg;
  return true;
}

bool AddArgument(ArgumentParser& parser, const char* short_name, const char* long_name,
                 int* target, const char* help, Nargs nargs,
                 bool (*validator)(const int&), const char* error_msg) {
  Impl* p = (Impl*)parser.impl;
  if (!p || !long_name || !target) return false;
  ArgSpec* a = AddArgSpec(p);
  if (!a) return false;
  a->name_for_get = help;
  a->short_name = short_name;
  a->long_name = long_name;
  a->type = kInt;
  a->first_target = target;
  a->nargs_mode = (int)nargs;
  a->int_validator = validator;
  a->float_validator = 0;
  a->cstr_validator = 0;
  a->error_msg = error_msg;
  return true;
}

bool AddArgument(ArgumentParser& parser, const char* short_name, const char* long_name,
                 float* target, const char* help, Nargs nargs,
                 bool (*validator)(const float&), const char* error_msg) {
  Impl* p = (Impl*)parser.impl;
  if (!p || !long_name || !target) return false;
  ArgSpec* a = AddArgSpec(p);
  if (!a) return false;
  a->name_for_get = help;
  a->short_name = short_name;
  a->long_name = long_name;
  a->type = kFloat;
  a->first_target = target;
  a->nargs_mode = (int)nargs;
  a->int_validator = 0;
  a->float_validator = validator;
  a->cstr_validator = 0;
  a->error_msg = error_msg;
  return true;
}

bool AddArgument(ArgumentParser& parser, const char* short_name, const char* long_name,
                 void* target_addr, const char* help, Nargs nargs,
                 bool (*validator)(const char* const&), const char* error_msg) {
  Impl* p = (Impl*)parser.impl;
  if (!p || !long_name || !target_addr) return false;
  ArgSpec* a = AddArgSpec(p);
  if (!a) return false;
  a->name_for_get = help;
  a->short_name = short_name;
  a->long_name = long_name;
  a->type = kCstr;
  a->first_target = target_addr;  // адрес начала буфера
  a->nargs_mode = (int)nargs;
  a->int_validator = 0;
  a->float_validator = 0;
  a->cstr_validator = validator;
  a->error_msg = error_msg;
  return true;
}

bool AddHelp(ArgumentParser& parser) {
  Impl* p = (Impl*)parser.impl;
  if (!p) return false;
  p->help.enabled = 1;
  return true;
}

static int IsAnyOption(Impl* p, const char* tok) {
  if (!tok) return 0;
  if (p->help.enabled && (StrEq(tok, p->help.short_name) || StrEq(tok, p->help.long_name)))
    return 1;
  if (FindFlagByOpt(p, tok)) return 1;

  int dummy = -1;
  if (FindArgByOpt(p, tok, &dummy)) return 1;

  if (IsLongOpt(tok)) {
    int eqpos = -1;
    int i = 0;
    while (tok[i]) {
      if (tok[i] == '=') {
        eqpos = i;
        break;
      }
      ++i;
    }
    if (eqpos > 0) {
      int j = 0;
      while (j < p->args_count) {
        ArgSpec* a = &p->args[j];
        if (a->long_name && (int)strlen(a->long_name) == eqpos &&
            strncmp(tok, a->long_name, (size_t)eqpos) == 0)
          return 1;
        ++j;
      }
    }
  }
  return 0;
}

bool Parse(ArgumentParser& parser, int argc, const char* const* argv) {
  Impl* p = (Impl*)parser.impl;
  if (!p) return false;

  int i = 0;
  while (i < p->flags_count) {
    if (p->flags[i].target) *(p->flags[i].target) = p->flags[i].default_value;
    ++i;
  }
  i = 0;
  while (i < p->args_count) {
    p->args[i].store.count = 0;
    ++i;
  }

  int pos_scan_index = 0;
  int argi = 1;

  while (argi < argc) {
    const char* tok = argv[argi];

    if (p->help.enabled && (StrEq(tok, p->help.short_name) || StrEq(tok, p->help.long_name))) {
      return true;
    }

    FlagSpec* f = FindFlagByOpt(p, tok);
    if (f) {
      *(f->target) = true;
      argi += 1;
      continue;
    }

    int eqpos = -1;
    ArgSpec* named = FindArgByOpt(p, tok, &eqpos);
    if (named) {
      if (eqpos >= 0) {
        const char* val = tok + eqpos + 1;
        if (val[0] == '\0') {
          if (!SetEmptyIfOptionalString(p, named)) return false;
        } else {
          if ((named->nargs_mode == kNargsOptional || named->nargs_mode == kNargsRequired) &&
              named->store.count > 0)
            return false;
          if (!PushValue(p, named, val)) return false;
        }
        argi += 1;
        continue;
      } else {
        const char* next = (argi + 1 < argc) ? argv[argi + 1] : 0;

        if (named->type == kCstr) {
          if (next) {
            if ((named->nargs_mode == kNargsOptional || named->nargs_mode == kNargsRequired) &&
                named->store.count > 0)
              return false;
            if (!PushValue(p, named, next)) return false;  // берём даже если next выглядит как опция
            argi += 2;
          } else {
            if (!SetEmptyIfOptionalString(p, named)) return false;
            argi += 1;
          }
        } else {
          if (!next || IsAnyOption(p, next)) return false;
          if ((named->nargs_mode == kNargsOptional || named->nargs_mode == kNargsRequired) &&
              named->store.count > 0)
            return false;
          if (!PushValue(p, named, next)) return false;
          argi += 2;
        }
        continue;
      }
    }

    ArgSpec* pos = 0;
    int scan = pos_scan_index;
    while (scan < p->args_count) {
      if (p->args[scan].short_name == 0 && p->args[scan].long_name == 0) {
        pos = &p->args[scan];
        break;
      }
      ++scan;
    }
    pos_scan_index = scan;

    if (!pos) return false;

    if (pos->nargs_mode == kNargsZeroOrMore || pos->nargs_mode == kNargsOneOrMore) {
      if (!PushValue(p, pos, tok)) return false;
    } else {
      if (pos->store.count > 0) {
        scan = pos_scan_index + 1;
        ArgSpec* nextpos = 0;
        while (scan < p->args_count) {
          if (p->args[scan].short_name == 0 && p->args[scan].long_name == 0) {
            nextpos = &p->args[scan];
            break;
          }
          ++scan;
        }
        pos_scan_index = scan;
        pos = nextpos;
        if (!pos) return false;
        if (!PushValue(p, pos, tok)) return false;
      } else {
        if (!PushValue(p, pos, tok)) return false;
      }
    }

    argi += 1;
  }

  i = 0;
  while (i < p->args_count) {
    ArgSpec* a = &p->args[i];
    if (a->nargs_mode == kNargsRequired) {
      if (a->store.count == 0) return false;
    }
    if (a->nargs_mode == kNargsOneOrMore) {
      if (a->store.count < 1) return false;
    }
    if (a->nargs_mode == kNargsOptional && a->store.count > 1 &&
        (a->short_name || a->long_name)) {
      return false;
    }
    ++i;
  }

  return true;
}

int GetRepeatedCount(ArgumentParser& parser, const char* name) {
  Impl* p = (Impl*)parser.impl;
  if (!p || !name) return 0;
  ArgSpec* a = FindArgByName(p, name);
  if (!a) return 0;
  return a->store.count;
}

bool GetRepeated(ArgumentParser& parser, const char* name, int index, int* out_value) {
  Impl* p = (Impl*)parser.impl;
  if (!p || !name || !out_value) return false;
  ArgSpec* a = FindArgByName(p, name);
  if (!a || a->type != kInt) return false;
  if (index < 0 || index >= a->store.count) return false;
  *out_value = a->store.ints[index];
  return true;
}

bool GetRepeated(ArgumentParser& parser, const char* name, int index, float* out_value) {
  Impl* p = (Impl*)parser.impl;
  if (!p || !name || !out_value) return false;
  ArgSpec* a = FindArgByName(p, name);
  if (!a || a->type != kFloat) return false;
  if (index < 0 || index >= a->store.count) return false;
  *out_value = a->store.floats[index];
  return true;
}

bool GetRepeated(ArgumentParser& parser, const char* name, int index, const char** out_value) {
  Impl* p = (Impl*)parser.impl;
  if (!p || !name || !out_value) return false;
  ArgSpec* a = FindArgByName(p, name);
  if (!a || a->type != kCstr) return false;
  if (index < 0 || index >= a->store.count) return false;
  *out_value = a->store.cstrs[index];
  return true;
}

bool PrintHelp(ArgumentParser& parser) {
  Impl* p = (Impl*)parser.impl;
  if (!p) return false;

  if (p->program[0]) printf("Usage: %s [options] [args]\n", p->program);
  else printf("Usage: program [options] [args]\n");

  if (p->flags_count > 0) {
    printf("\nFlags:\n");
    int i = 0;
    while (i < p->flags_count) {
      const char* s = p->flags[i].short_name ? p->flags[i].short_name : "";
      const char* l = p->flags[i].long_name ? p->flags[i].long_name : "";
      const char* h = p->flags[i].help ? p->flags[i].help : "";
      if (s[0] && l[0]) printf("  %s, %s\t%s\n", s, l, h);
      else if (s[0]) printf("  %s\t%s\n", s, h);
      else if (l[0]) printf("  %s\t%s\n", l, h);
      ++i;
    }
  }

  if (p->args_count > 0) {
    printf("\nOptions / positional args:\n");
    int i = 0;
    while (i < p->args_count) {
      ArgSpec* a = &p->args[i];
      const char* s = a->short_name ? a->short_name : "";
      const char* l = a->long_name ? a->long_name : "";
      const char* n = a->name_for_get ? a->name_for_get : "";
      if (s[0] || l[0]) {
        if (s[0] && l[0]) printf("  %s, %s\t%s\n", s, l, n);
        else if (s[0]) printf("  %s\t%s\n", s, n);
        else printf("  %s\t%s\n", l, n);
      } else {
        printf("  %s\n", n);
      }
      ++i;
    }
  }

  if (p->help.enabled) {
    printf("\nHelp:\n  %s, %s\tShow help\n", p->help.short_name, p->help.long_name);
  }
  return true;
}

}  // namespace nargparse