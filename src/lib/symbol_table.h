#ifndef _LIB_SYMBOL_TABLE_H_
#define _LIB_SYMBOL_TABLE_H_

#include <map>
#include <set>
#include <vector>
#include <cstdio>
#include <unicode/ustdio.h>
#include <unicode/unistr.h>

// All enums in this file should have explicit values
// in order to ensure consistent serialization

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

enum SymbolType {
  UnionSymbol    = 0,
  NegationSymbol = 1,
  IdentitySymbol = 2,
  CategorySymbol = 3,
  FlagSymbol     = 4
};

enum SymbolClass {
  SymbolClass_Any   = 0,
  SymbolClass_Tag   = 1,
  SymbolClass_Char  = 2,
  SymbolClass_Upper = 3,
  SymbolClass_Lower = 4
};

// These values are declared explicitly to ensure consistent serialization
enum FlagSymbolType {
  None        = 0,
  Clear       = 1,
  Positive    = 2,
  Negative    = 3,
  Require     = 4,
  Disallow    = 5,
  Unification = 6
};

struct FlagSymbolStruct {
  FlagSymbolType type;
  string_ref sym;
  string_ref val;
};

struct SymbolExpansion {
  SymbolType type;
  std::set<string_ref> syms;  // UnionSymbol, NegationSymbol
  size_t tape;                // IdentitySymbol
  SymbolClass cls;            // CategorySymbol
  FlagSymbolStruct flag;      // FlagSymbol
};

bool operator==(const SymbolExpansion& a, const SymbolExpansion& b);
bool operator!=(const SymbolExpansion& a, const SymbolExpansion& b);

class SymbolTable {
private:
  std::map<UnicodeString, string_ref> name_to_id;
  std::vector<UnicodeString> id_to_name;
  std::map<string_ref, SymbolExpansion> symbols;
public:
  SymbolTable();
  ~SymbolTable();

  const UnicodeString& name(string_ref r) const;
  string_ref internName(const UnicodeString& name);

  void read(FILE* in);
  void write(FILE* out);
  void write_symbol(UFILE* out, string_ref sym, bool escape);
  void write_symbol(UnicodeString& s, string_ref sym, bool escape);
  std::map<string_ref, string_ref> merge(SymbolTable& other);

  void define(string_ref sym, SymbolExpansion exp, bool check = false);
  bool isDefined(string_ref sym);
  const SymbolExpansion& lookup(string_ref sym);

  void insertUnion(string_ref sym, std::set<string_ref> ls, bool check = false);
  void insertNegation(string_ref sym, std::set<string_ref> ls, bool check = false);
  void insertIdentity(string_ref sym, size_t tape_idx, bool check = false);
  void insertCategory(string_ref sym, SymbolClass cls, bool check = false);
  void insertFlag(string_ref sym, FlagSymbolType type, string_ref flag, string_ref val, bool check = false);

  string_ref parseSymbol(const UnicodeString& s);

  string_ref makeUnion(std::set<string_ref> ls);
  string_ref makeNegation(std::set<string_ref> ls);
  string_ref makeIdentity(size_t tape_idx);
  string_ref makeCategory(SymbolClass cls);
  string_ref makeFlag(FlagSymbolType type, string_ref flag, string_ref val);

  bool isEpsilon(string_ref sym, bool flagsAsEpsilon);
};

#endif
