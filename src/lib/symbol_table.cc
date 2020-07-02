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
    }
  }
}
