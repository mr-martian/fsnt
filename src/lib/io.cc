#include "io.h"
#include "utils/icu-iter.h"
#include "utils/compression.h"
#include <vector>
#include <unicode/unistr.h>
#include <unicode/uchar.h>
#include <cstring>

#include <iostream>

Transducer*
readBin(FILE* in)
{
  ////////// HEADER

  bool read_weights = false;

  char header[4]{};
  size_t bytes_read = fread(header, 1, 4, in);
  if (bytes_read == 4 && strncmp(header, HEADER_TRANSDUCER, 4) == 0) {
    auto features = read_le<uint64_t>(in);
    if (features >= TDF_UNKNOWN) {
      throw std::runtime_error("Transducer has features that are unknown to this version of fsnt - upgrade!");
    }
    read_weights = (features & TDF_WEIGHTS);
  } else {
    throw std::runtime_error("Missing transducer header");
  }

  size_t tapes = Compression::multibyte_read(in);

  Transducer* t = new Transducer(tapes);

  size_t tape_name_count = Compression::multibyte_read(in);
  for(size_t i = 0; i < tape_name_count; i++) {
    UnicodeString name = Compression::string_read(in);
    TapeInfo info;
    info.index = Compression::multibyte_read(in);
    info.flags = Compression::multibyte_read(in);
    t->setTapeInfo(name, info);
  }

  ////////// ALPHABET

  t->getAlphabet().read(in);

  ////////// FINALS

  for(unsigned int i = 0, lim = Compression::multibyte_read(in); i < lim; i++) {
    state_t state = Compression::multibyte_read(in);
    t->setFinal(state, (read_weights ? Compression::long_multibyte_read(in) : 0.000));
  }

  ////////// TRANSITIONS

  unsigned int state_count = Compression::multibyte_read(in);
  t->addStates(state_count - 1);

  for(unsigned int src = 0; src < state_count; src++) {
    for(unsigned int i = 0, lim = Compression::multibyte_read(in); i < lim; i++) {
      unsigned int dest = Compression::multibyte_read(in);
      unsigned int tr_count = Compression::multibyte_read(in);
      for(unsigned int ti = 0; ti < tr_count; ti++)
      {
        Transition tr;
        tr.symbols.resize(tapes);
        for(unsigned int s = 0; s < tapes; s++) {
          tr.symbols[s] = string_ref(Compression::multibyte_read(in));
        }
        tr.weight = (read_weights ? Compression::long_multibyte_read(in) : 0.000);
        t->insertTransition(src, dest, tr);
      }
    }
  }

  return t;
}

void
writeBin(Transducer* t, FILE *out)
{
  ////////// HEADER

  fwrite(HEADER_TRANSDUCER, 1, 4, out);

  bool write_weights = true; //weighted();

  uint64_t features = 0;
  if (write_weights) {
      features |= TDF_WEIGHTS;
  }
  write_le(out, features);

  Compression::multibyte_write(t->getTapeCount(), out);

  auto tapes = t->getTapeInfo();
  Compression::multibyte_write(tapes.size(), out);
  for(auto it : tapes) {
    Compression::string_write(it.first, out);
    Compression::multibyte_write(it.second.index, out);
    Compression::multibyte_write(it.second.flags, out);
  }

  ////////// ALPHABET

  t->getAlphabet().write(out);

  ////////// FINALS

  auto finals = t->getFinals();
  Compression::multibyte_write(finals.size(), out);

  for(auto& it : finals) {
    Compression::multibyte_write(it.first, out);
    if(write_weights) {
      Compression::long_multibyte_write(it.second, out);
    }
  }

  ////////// TRANSITIONS

  auto transitions = t->getTransitions();
  Compression::multibyte_write(transitions.size(), out);

  for(auto& it : transitions) {
    Compression::multibyte_write(it.size(), out);
    for(auto& it2 : it) {
      Compression::multibyte_write(it2.first, out);
      Compression::multibyte_write(it2.second.size(), out);
      for(auto& tr : it2.second) {
        for(auto sym : tr.symbols) {
          Compression::multibyte_write((unsigned int)sym, out);
        }
        if(write_weights) {
          Compression::long_multibyte_write(tr.weight, out);
        }
      }
    }
  }
}

Transducer*
readATT(UFILE* in)
{
  vector<vector<UnicodeString>> headers;
  vector<vector<UnicodeString>> lines;
  while(true) {
    if(u_feof(in)) {
      break;
    }
    vector<UnicodeString> pieces(1);
    bool lastLine = false;
    while(true) {
      UChar32 c = u_fgetcx(in);
      if(c == U_EOF) {
        lastLine = true;
        break;
      } else if(c == '\n') {
        break;
      } else if(c == '\t') {
        pieces.resize(pieces.size() + 1);
      } else {
        pieces.back() += c;
      }
    }
    if(pieces[0].startsWith("#")) {
      if(pieces.back().length() == 0) {
        pieces.pop_back();
      }
      headers.push_back(pieces);
      continue;
    }
    if(pieces.back().length() == 0) {
      pieces.pop_back();
    } else {
      lines.push_back(pieces);
    }
    if(pieces.size() == 1) {
      bool is_separator = true;
      for(auto it = char_iter(pieces[0]); it != it.end(); it++) {
        if(*it != "-") {
          is_separator = false;
          break;
        }
      }
      if(is_separator) {
        lastLine = true;
        lines.pop_back();
      }
    }
    if(lastLine) {
      break;
    }
  }
  if(lines.size() == 0) {
    return NULL;
  } else if(lines[0].size() < 3) {
    return NULL; // TODO: how do we want to deal with error handling?
  }
  bool weighted = false;
  size_t tapes = lines[0].size() - 2;
  vector<UnicodeString> tapeNames;
  for(auto line : headers) {
    if(line[0] == "# tapes:") {
      for(unsigned int i = 1; i < line.size(); i++) {
        tapeNames.push_back(line[i]);
      }
    }
  }
  if(tapeNames.size() == 0) {
    if(lines[0].size() >= 4) {
      weighted = true;
      for(auto c = char_iter(lines[0].back()); c != c.end(); c++) {
        if((*c).length() == 1 && (*c == "." || u_isdigit((*c)[0]))) {
          continue;
        } else {
          weighted = false;
          break;
        }
      }
      if(weighted) {
        tapes--;
      }
    }
    for(size_t i = 1; i <= tapes; i++) {
      UnicodeString name;
      char16_t* buf = name.getBuffer(10);
      u_sprintf(buf, "Tape_%u", i);
      name.releaseBuffer();
      tapeNames.push_back(name);
    }
  } else if(tapeNames.size() == tapes-1) {
    weighted = true;
    tapes--;
  } else if(tapeNames.size() != tapes) {
    return NULL;
  }

  size_t transition_len = lines[0].size();
  size_t final_len = (weighted ? 2 : 1);

  Transducer* t = new Transducer(tapes);
  SymbolTable& alpha = t->getAlphabet();
  for(auto line : lines) {
    if(line.size() == transition_len) {
      Transition tr;
      tr.symbols.resize(tapes);
      state_t src = stoul(line[0]);
      state_t trg = stoul(line[1]);
      while(t->size() <= src || t->size() <= trg) {
        t->addState();
      }
      tr.weight = (weighted ? stoul(line.back()) : 0.000);

      for(size_t i = 0; i < tapes; i++) {
        tr.symbols[i] = alpha.parseSymbol(line[i+2]);
      }
      t->insertTransition(src, trg, tr);
    } else if(line.size() == final_len) {
      state_t state = stoul(line[0]);
      double weight = (weighted ? stoul(line[1]) : 0.000);
      while(t->size() <= state) {
        t->addState();
      }
      t->setFinal(state, weight);
    } else {
      // TODO: @ERROR
      delete t;
      return NULL;
    }
  }
  for(size_t i = 0; i < tapes; i++) {
    TapeInfo info;
    info.index = i;
    info.flags = 0;
    t->setTapeInfo(tapeNames[i], info);
  }
  return t;
}

void
writeATT(Transducer* t, UFILE* out, bool writeWeights, bool writeHeaders)
{
  if(writeHeaders) {
    vector<UnicodeString> names = vector<UnicodeString>(t->getTapeCount());
    map<UnicodeString, vector<UnicodeString>> altNames;
    auto info = t->getTapeInfo();
    for(auto it : info) {
      size_t idx = it.second.index;
      if(names[idx].length() == 0) {
        names[idx] = it.first;
      } else {
        altNames[names[idx]].push_back(it.first);
      }
    }
    u_fprintf(out, "# tapes:");
    for(auto name : names) {
      u_fprintf(out, "\t%S", name.getTerminatedBuffer());
    }
    u_fprintf(out, "\n");
    for(auto it : altNames) {
      UnicodeString name = it.first; // not sure why this line is needed, but apparently it is
      const char16_t* buf = name.getTerminatedBuffer();
      for(auto it2 : it.second) {
        u_fprintf(out, "# alt:\t%S\t%S\n", buf, it2.getTerminatedBuffer());
      }
    }
  }
  auto transitions = t->getTransitions();
  auto alphabet = t->getAlphabet();
  for(unsigned int src = 0; src < transitions.size(); src++) {
    for(auto& it : transitions[src]) {
      unsigned int dest = it.first;
      for(auto& tr : it.second) {
        u_fprintf(out, "%d\t%d\t", src, dest);
        for(auto& sym : tr.symbols) {
          alphabet.write_symbol(out, sym, true);
          u_fprintf(out, "\t");
        }
        if(writeWeights) {
          u_fprintf(out, "%f", tr.weight);
        }
        u_fprintf(out, "\n");
      }
    }
  }
  for(auto& fin : t->getFinals()) {
    if(writeWeights) {
      u_fprintf(out, "%d\t%f\n", fin.first, fin.second);
    } else {
      u_fprintf(out, "%d\n", fin.first);
    }
  }
}
