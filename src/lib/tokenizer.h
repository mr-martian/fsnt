#ifndef _LIB_TOKENIZER_H_
#define _LIB_TOKENIZER_H_

#include "symbol_table.h"
#include <unicode/ustdio.h>
#include <map>

class Tokenizer {
private:
  struct TrieNode {
    std::map<UnicodeString, TrieNode*> cont;
    bool is_sym;
  };

  TrieNode* symbols;
  SymbolTable* alphabet;
  UFILE* input;
  int buffer_size;
  UnicodeString input_buffer;
  std::vector<UnicodeString> token_buffer;
  size_t token_buffer_index;
  bool have_eof;

  void deleteTrieNode(TrieNode* node);
  void addToTrie(const UnicodeString& s);
  void fillBuffer();
public:
  Tokenizer(SymbolTable* alpha, UFILE* in);
  ~Tokenizer();
  bool done();
  const UnicodeString& nextToken();
  void writeTokenBuffer(UnicodeString& s, size_t n);
};

#endif
