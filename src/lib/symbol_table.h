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
  CategorySymbol = 3
};

enum SymbolClass {
  SymbolClass_Any   = 0,
  SymbolClass_Tag   = 1,
  SymbolClass_Char  = 2,
  SymbolClass_Upper = 3,
  SymbolClass_Lower = 4
};

struct SymbolExpansion {
  SymbolType type;
  std::set<string_ref> syms;  // UnionSymbol, NegationSymbol
  size_t tape;                // IdentitySymbol
  SymbolClass cls;            // CategorySymbol
};

class SymbolTable {
private:
  std::map<string_ref, SymbolExpansion> symbols;
public:
  SymbolTable();
  ~SymbolTable();
  void read(FILE* in);
  void write(FILE* out);
};

#endif
