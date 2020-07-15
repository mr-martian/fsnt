#include "symbol_table.h"
#include "utils/compression.h"

SymbolTable::SymbolTable()
{
}

SymbolTable::~SymbolTable()
{
}

void
SymbolTable::read(FILE* in)
{
  symbols.clear();
  for(unsigned int i = 0, lim = Compression::multibyte_read(in); i < lim; i++) {
    unsigned int sym = Compression::multibyte_read(in);
    SymbolExpansion exp;
    exp.type = (SymbolType)Compression::multibyte_read(in);
    switch(exp.type) {
      case UnionSymbol:
      case NegationSymbol:
      {
        unsigned int count = Compression::multibyte_read(in);
        for(unsigned int s = 0; s < count; s++) {
          exp.syms.insert(string_ref(Compression::multibyte_read(in)));
        }
        break;
      }
      case IdentitySymbol:
        exp.tape = Compression::multibyte_read(in);
        break;
      case CategorySymbol:
        exp.cls = (SymbolClass)Compression::multibyte_read(in);
        break;
      case FlagSymbol:
        exp.flag.type = (FlagSymbolType)Compression::multibyte_read(in);
        exp.flag.sym = string_ref(Compression::multibyte_read(in));
        exp.flag.val = string_ref(Compression::multibyte_read(in));
        break;
    }
    symbols[string_ref(sym)] = exp;
  }
}

void
SymbolTable::write(FILE* out)
{
  Compression::multibyte_write(symbols.size(), out);
  for(auto& it : symbols) {
    Compression::multibyte_write((unsigned int)it.first, out);
    Compression::multibyte_write(it.second.type, out);
    switch(it.second.type) {
      case UnionSymbol:
      case NegationSymbol:
        Compression::multibyte_write(it.second.syms.size(), out);
        for(auto op : it.second.syms) {
          Compression::multibyte_write((unsigned int)op, out);
        }
        break;
      case IdentitySymbol:
        Compression::multibyte_write(it.second.tape, out);
        break;
      case CategorySymbol:
        Compression::multibyte_write(it.second.cls, out);
        break;
      case FlagSymbol:
        Compression::multibyte_write(it.second.flag.type, out);
        Compression::multibyte_write((unsigned int)it.second.flag.sym, out);
        Compression::multibyte_write((unsigned int)it.second.flag.val, out);
        break;
    }
  }
}

void
SymbolTable::insertUnion(string_ref sym, std::set<string_ref> ls)
{
  symbols[sym].type = UnionSymbol;
  symbols[sym].syms = ls;
}

void
SymbolTable::insertNegation(string_ref sym, std::set<string_ref> ls)
{
  symbols[sym].type = NegationSymbol;
  symbols[sym].syms = ls;
}

void
SymbolTable::insertIdentity(string_ref sym, size_t tape_idx)
{
  symbols[sym].type = IdentitySymbol;
  symbols[sym].tape = tape_idx;
}

void
SymbolTable::insertCategory(string_ref sym, SymbolClass cls)
{
  symbols[sym].type = CategorySymbol;
  symbols[sym].cls = cls;
}

void
SymbolTable::insertFlag(string_ref sym, FlagSymbolType type, string_ref flag, string_ref val)
{
  symbols[sym].type = FlagSymbol;
  symbols[sym].flag.type = type;
  symbols[sym].flag.sym = flag;
  symbols[sym].flag.val = val;
}

bool
SymbolTable::defined(string_ref sym)
{
  return symbols.find(sym) != symbols.end();
}

const SymbolExpansion&
SymbolTable::lookup(string_ref sym)
{
  return symbols[sym];
}

bool
SymbolTable::isEpsilon(string_ref sym, bool flagsAsEpsilon)
{
  if(sym == string_ref(0)) {
    return true;
  } else if(flagsAsEpsilon && symbols.find(sym) != symbols.end() &&
            symbols[sym].type == FlagSymbol) {
    return true;
  } else {
    return false;
  }
}
