#ifndef _LIB_STRING_INTERNER_H_
#define _LIB_STRING_INTERNER_H_

#include <map>
#include <vector>
#include <cstdio>
#include <unicode/ustdio.h>
#include <unicode/unistr.h>

struct string_ref {
  unsigned int i;
  string_ref() : i(0) {}
  explicit string_ref(unsigned int _i) : i(_i) {}
  explicit operator unsigned int() const { return i; }
  bool operator == (string_ref other) const { return i == other.i; }
  bool operator != (string_ref other) const { return !(*this == other); }
  bool operator < (string_ref other) const { return i < other.i; }
  bool operator !() const { return empty(); }
  string_ref operator || (string_ref other) const {
    return i ? *this : other;
  }
  bool empty() const { return i == 0; }
  bool valid() const { return i != 0; }
};

template<>
struct std::hash<string_ref> {
  size_t operator()(const string_ref &t) const
  {
    return std::hash<unsigned int>()(t.i);
  }
};

class StringInterner {
private:
  std::map<UnicodeString, string_ref> name_to_id;
  std::vector<UnicodeString> id_to_name;
public:
  StringInterner();
  ~StringInterner();
  const UnicodeString& name(string_ref r) const;
  string_ref internName(const UnicodeString& name);
  void read(FILE* in);
  void write(FILE* out);
  void write_symbol(UFILE* out, string_ref sym, bool escape);
};

#endif
