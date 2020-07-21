#include "symbol_table.h"
#include "utils/compression.h"
#include "utils/icu-iter.h"
#include <iostream>
#include <unicode/ustream.h>

bool
operator==(const SymbolExpansion& a, const SymbolExpansion& b)
{
  return !(a != b);
}

bool
operator!=(const SymbolExpansion& a, const SymbolExpansion& b)
{
  return (a.type != b.type ||
          ((a.type == UnionSymbol || a.type == NegationSymbol) &&
           a.syms != b.syms) ||
          (a.type == IdentitySymbol && a.tape != b.tape) ||
          (a.type == CategorySymbol && a.cls != b.cls) ||
          (a.type == FlagSymbol &&
           (a.flag.type != b.flag.type ||
            a.flag.sym != b.flag.sym ||
            a.flag.val != b.flag.val)));
}

SymbolTable::SymbolTable()
{
  internName("");
}

SymbolTable::~SymbolTable()
{
}

const UnicodeString&
SymbolTable::name(string_ref r) const
{
  return id_to_name[(unsigned int)r];
}

string_ref
SymbolTable::internName(const UnicodeString& name)
{
  if(name_to_id.find(name) == name_to_id.end())
  {
    name_to_id[name] = string_ref(id_to_name.size());
    id_to_name.push_back(name);
  }
  return name_to_id[name];
}

const std::vector<UnicodeString>&
SymbolTable::getSymbols()
{
  return id_to_name;
}

void
SymbolTable::read(FILE* in)
{
  id_to_name.clear();
  name_to_id.clear();
  internName("");
  for(unsigned int i = 1, lim = Compression::multibyte_read(in); i < lim; i++) {
    id_to_name.push_back(Compression::string_read(in));
    name_to_id[id_to_name.back()] = string_ref(i);
  }
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
  Compression::multibyte_write(id_to_name.size(), out);
  for(unsigned int i = 1; i < id_to_name.size(); i++) {
    Compression::string_write(id_to_name[i], out);
  }
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
SymbolTable::write_symbol(UFILE* out, string_ref sym, bool escape)
{
  UnicodeString& s = id_to_name[(unsigned int)sym];
  if(escape) {
    if(s == " ") {
      u_fprintf(out, "@_SPACE_@");
      return;
    } else if(s == "\t") {
      u_fprintf(out, "@_TAB_@");
      return;
    } else if(s == "") {
      u_fprintf(out, "@0@");
      return;
    }
  }
  u_fprintf(out, "%S", s.getTerminatedBuffer());
}

void
SymbolTable::write_symbol(UnicodeString& s, string_ref sym, bool escape)
{
  UnicodeString& str = id_to_name[(unsigned int)sym];
  if(escape) {
    if(str == " ") {
      s += "@_SPACE_@";
      return;
    } else if(s == "\t") {
      s += "@_TAB_@";
      return;
    } else if(s == "") {
      s += "@0@";
      return;
    }
  }
  s += str;
}

std::map<string_ref, string_ref>
SymbolTable::merge(SymbolTable& other)
{
  std::map<string_ref, string_ref> ret;
  for(size_t i = 1; i < other.id_to_name.size(); i++) {
    ret[string_ref(i)] = internName(other.id_to_name[i]);
  }
  // TODO: merge symbols
  return ret;
}

void
SymbolTable::define(string_ref sym, SymbolExpansion exp, bool check)
{
  if(check && symbols.find(sym) != symbols.end() &&
     symbols[sym] != exp) {
    throw std::runtime_error("Multiple conflicting definitions for symbol.");
  }
  symbols[sym] = exp;
}

bool
SymbolTable::isDefined(string_ref sym)
{
  return symbols.find(sym) != symbols.end();
}

const SymbolExpansion&
SymbolTable::lookup(string_ref sym)
{
  return symbols[sym];
}

void
SymbolTable::insertUnion(string_ref sym, std::set<string_ref> ls, bool check)
{
  SymbolExpansion exp;
  exp.type = UnionSymbol;
  exp.syms = ls;
  define(sym, exp, check);
}

void
SymbolTable::insertNegation(string_ref sym, std::set<string_ref> ls, bool check)
{
  SymbolExpansion exp;
  exp.type = NegationSymbol;
  exp.syms = ls;
  define(sym, exp, check);
}

void
SymbolTable::insertIdentity(string_ref sym, size_t tape_idx, bool check)
{
  SymbolExpansion exp;
  exp.type = IdentitySymbol;
  exp.tape = tape_idx;
  define(sym, exp, check);
}

void
SymbolTable::insertCategory(string_ref sym, SymbolClass cls, bool check)
{
  SymbolExpansion exp;
  exp.type = CategorySymbol;
  exp.cls = cls;
  define(sym, exp, check);
}

void
SymbolTable::insertFlag(string_ref sym, FlagSymbolType type, string_ref flag, string_ref val, bool check)
{
  SymbolExpansion exp;
  exp.type = FlagSymbol;
  exp.flag.type = type;
  exp.flag.sym = flag;
  exp.flag.val = val;
  define(sym, exp, check);
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

std::set<string_ref>
split_comma(const UnicodeString& s, SymbolTable* table)
{
  std::set<string_ref> ret;
  int pos = 0;
  for(auto c = char_iter(s); c != c.end(); c++) {
    if(*c == ",") {
      ret.insert(table->internName(s.tempSubStringBetween(pos, c.span().first)));
      pos = c.span().second;
    }
  }
  ret.insert(table->internName(s.tempSubStringBetween(pos, s.length())));
  return ret;
}

string_ref
SymbolTable::parseSymbol(const UnicodeString& s)
{
  if(s == "@0@") {
    return string_ref(0);
  }
  string_ref ret = internName(s);
  if(s.length() > 4 && s[0] == '@' && s[s.length()-1] == '@') {
    SymbolExpansion exp;
    if(s[1] == '_' && s[s.length()-2] == '_') {
      if(s.startsWith("@_UNION_{") && s[s.length()-3] == '}') {
        std::set<string_ref> syms = split_comma(s.tempSubStringBetween(9, s.length()-3), this);
        exp.type = UnionSymbol;
        exp.syms = syms;
        define(ret, exp, true);
        return ret;
      }
    } else {
      exp.type = FlagSymbol;
      switch(s[1]) {
        case 'C':
          exp.flag.type = Clear; break;
        case 'P':
          exp.flag.type = Positive; break;
        case 'N':
          exp.flag.type = Negative; break;
        case 'R':
          exp.flag.type = Require; break;
        case 'D':
          exp.flag.type = Disallow; break;
        case 'U':
          exp.flag.type = Unification; break;
        default:
          return ret;
      }
      for(auto c = char_iter(s); c != c.end(); c++) {
        if(c.span().first > 3 && *c == ".") {
          exp.flag.sym = internName(s.tempSubStringBetween(3, c.span().first));
          exp.flag.val = internName(s.tempSubStringBetween(c.span().second, s.length() - 1));
          define(ret, exp, true);
          return ret;
        }
      }
      exp.flag.sym = internName(s.tempSubStringBetween(3, s.length() - 1));
      exp.flag.val = string_ref(0);
      define(ret, exp, true);
    }
  }
  return ret;
}

string_ref
SymbolTable::makeUnion(std::set<string_ref> ls)
{
  // TODO
  return string_ref(0);
}

string_ref
SymbolTable::makeNegation(std::set<string_ref> ls)
{
  // TODO
  return string_ref(0);
}

string_ref
SymbolTable::makeIdentity(size_t tape_idx)
{
  UnicodeString s;
  char16_t* buf = s.getBuffer(20);
  u_sprintf(buf, "@_ID_%d_@", tape_idx);
  s.releaseBuffer();
  string_ref ret = internName(s);
  insertIdentity(ret, tape_idx, true);
  return ret;
}

string_ref
SymbolTable::makeCategory(SymbolClass cls)
{
  // TODO
  return string_ref(0);
}

string_ref
SymbolTable::makeFlag(FlagSymbolType type, string_ref flag, string_ref val)
{
  UnicodeString s = "@";
  switch(type) {
    case Clear:
      s += 'C';
      break;
    case Positive:
      s += 'P';
      break;
    case Negative:
      s += 'N';
      break;
    case Require:
      s += 'R';
      break;
    case Disallow:
      s += 'D';
      break;
    case Unification:
      s += 'U';
      break;
  }
  s += '.';
  write_symbol(s, flag, true);
  if(val != string_ref(0)) {
    s += '.';
    write_symbol(s, val, true);
  }
  s += '@';
  string_ref ret = internName(s);
  insertFlag(ret, type, flag, val);
  return ret;
}

bool
SymbolTable::isInterned(const UnicodeString& s)
{
  return (name_to_id.find(s) != name_to_id.end());
}
