#ifndef _LIB_SYMBOL_TABLE_H_
#define _LIB_SYMBOL_TABLE_H_

#include <map>
#include <set>
#include <cstdio>
#include "string_interner.h"

// All enums in this file should have explicit values
// in order to ensure consistent serialization

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

class SymbolTable {
private:
  std::map<string_ref, SymbolExpansion> symbols;
public:
  SymbolTable();
  ~SymbolTable();
  void read(FILE* in);
  void write(FILE* out);
  void insertUnion(string_ref sym, std::set<string_ref> ls);
  void insertNegation(string_ref sym, std::set<string_ref> ls);
  void insertIdentity(string_ref sym, size_t tape_idx);
  void insertCategory(string_ref sym, SymbolClass cls);
  void insertFlag(string_ref sym, FlagSymbolType type, string_ref flag, string_ref val);
  bool defined(string_ref sym);
  const SymbolExpansion& lookup(string_ref sym);
  bool isEpsilon(string_ref sym, bool flagsAsEpsilon);
};

#endif
