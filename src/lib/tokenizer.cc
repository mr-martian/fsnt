#include "tokenizer.h"
#include "utils/icu-iter.h"

Tokenizer::Tokenizer(SymbolTable* alpha, UFILE* in)
{
  alphabet = alpha;
  input = in;
  auto syms = alpha->getSymbols();
  symbols = new TrieNode();
  symbols->is_sym = false;
  buffer_size = 0;
  for(auto s : syms) {
    addToTrie(s);
    if(s.length() > buffer_size) {
      buffer_size = s.length();
    }
  }
  token_buffer_index = 0;
  have_eof = false;
}

Tokenizer::~Tokenizer()
{
  deleteTrieNode(symbols);
}

void
Tokenizer::deleteTrieNode(TrieNode* node)
{
  for(auto it : node->cont) {
    deleteTrieNode(it.second);
  }
  delete node;
}

void
Tokenizer::addToTrie(const UnicodeString& s)
{
  TrieNode* node = symbols;
  for(auto c = char_iter(s); c != c.end(); c++) {
    if(node->cont.find(*c) == node->cont.end()) {
      node->cont[*c] = new TrieNode;
      node->cont[*c]->is_sym = false;
    }
    node = node->cont[*c];
  }
  node->is_sym = true;
}

bool
Tokenizer::done()
{
  return (have_eof && input_buffer.length() == 0);
}

void
Tokenizer::fillBuffer()
{
  while(input_buffer.length() < buffer_size && !u_feof(input)) {
    UChar c = u_fgetc(input);
    if(c == U_EOF) {
      have_eof = true;
      break;
    }
    input_buffer += c;
  }
}

const UnicodeString&
Tokenizer::nextToken()
{
  if(token_buffer_index < token_buffer.size()) {
    token_buffer_index++;
    return token_buffer[token_buffer_index-1];
  }
  UnicodeString non_word;
  UnicodeString word;
  while(true) {
    size_t last_idx = 0;
    TrieNode* node = symbols;
    fillBuffer();
    if(done()) {
      break;
    }
    for(auto c = char_iter(input_buffer); c != c.end(); c++) {
      if(node->cont.find(*c) != node->cont.end()) {
        node = node->cont[*c];
      } else {
        break;
      }
      if(node->is_sym) {
        last_idx = c.span().second;
      }
      if(node->cont.size() == 0) {
        break;
      }
    }
    if(last_idx == 0) {
      auto c = char_iter(input_buffer);
      non_word += *c;
      input_buffer.setTo(input_buffer, c.span().second);
    } else {
      word = input_buffer.tempSubString(0, last_idx);
      break;
    }
  }
  if(non_word.length() > 0) {
    token_buffer.push_back(non_word);
  }
  if(word.length() > 0) {
    token_buffer.push_back(word);
  }
  token_buffer_index++;
  return token_buffer[token_buffer_index-1];
}

void
Tokenizer::writeTokenBuffer(UnicodeString& s, size_t n)
{
  size_t lim = n;
  if(token_buffer.size() < lim) {
    lim = token_buffer.size();
  }
  for(size_t i = 0; i < lim; i++) {
    s += token_buffer[i];
  }
  token_buffer.erase(token_buffer.begin(), token_buffer.begin()+lim);
}
