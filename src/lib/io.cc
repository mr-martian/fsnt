#include "io.h"
#include "utils/icu-iter.h"
#include "utils/compression.h"
#include <vector>
#include <unicode/unistr.h>
#include <unicode/uchar.h>
#include <cstring>

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
  size_t flagTapes = Compression::multibyte_read(in);

  Transducer* t = new Transducer(tapes, flagTapes);

  ////////// ALPHABET

  t->getAlphabet().read(in);
  t->getSymbols().read(in);

  ////////// FINALS

  for(unsigned int i = 0, lim = Compression::multibyte_read(in); i < lim; i++) {
    state_number_t state = Compression::multibyte_read(in);
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
        tr.flags.resize(flagTapes);
        for(unsigned int s = 0; s < tapes; s++) {
          tr.symbols[s] = string_ref(Compression::multibyte_read(in));
        }
        for(unsigned int f = 0; f < flagTapes; f++) {
          FlagSymbol& flag = tr.flags[f];
          flag.type = (FlagSymbolType)Compression::multibyte_read(in);
          flag.sym = string_ref(Compression::multibyte_read(in));
          flag.val = string_ref(Compression::multibyte_read(in));
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

  Compression::multibyte_write(t->getTapes(), out);
  Compression::multibyte_write(t->getFlagTapes(), out);

  ////////// ALPHABET

  t->getAlphabet().write(out);
  t->getSymbols().write(out);

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
        for(auto flag : tr.flags) {
          Compression::multibyte_write(flag.type, out);
          Compression::multibyte_write((unsigned int)flag.sym, out);
          Compression::multibyte_write((unsigned int)flag.val, out);
        }
        if(write_weights) {
          Compression::long_multibyte_write(tr.weight, out);
        }
      }
    }
  }
}

bool
isFlagSymbol(const UnicodeString& sym)
{
  return sym.length() >= 2 && sym[0] == '@' && sym[sym.length()-1] == '@' &&
          (sym == "@@" || // empty flag
           (sym.length() > 4 && sym[2] == '.' &&
             (sym[1] == 'C' || sym[1] == 'P' || sym[1] == 'N' ||
              sym[1] == 'R' || sym[1] == 'D' || sym[1] == 'U')));
}

FlagSymbol
parseFlag(const UnicodeString& sym, StringInterner& alphabet)
{
  FlagSymbol ret;
  if(sym == "@@") {
    ret.type = None;
    ret.sym = string_ref(0);
    ret.val = string_ref(0);
    return ret;
  }
  if(sym[1] == 'C') {
    ret.type = Clear;
  } else if(sym[1] == 'P') {
    ret.type = Positive;
  } else if(sym[1] == 'N') {
    ret.type = Negative;
  } else if(sym[1] == 'R') {
    ret.type = Require;
  } else if(sym[1] == 'D') {
    ret.type = Disallow;
  } else if(sym[1] == 'U') {
    ret.type = Unification;
  }
  for(auto c = char_iter(sym); c != c.end(); c++) {
    if(c.span().first > 3 && *c == ".") {
      ret.sym = alphabet.internName(sym.tempSubStringBetween(3, c.span().first));
      ret.val = alphabet.internName(sym.tempSubStringBetween(c.span().second, sym.length() - 1));
      return ret;
    }
  }
  ret.sym = alphabet.internName(sym.tempSubStringBetween(3, sym.length() - 1));
  ret.val = string_ref(0);
  return ret;
}

Transducer*
readATT(UFILE* in, bool flagsSeparate)
{
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
  size_t transition_len = lines[0].size();
  size_t final_len = (weighted ? 2 : 1);
  size_t flagTapes = (flagsSeparate ? 1 : 0);
  if(!flagsSeparate)
  {
    while(tapes > 0) {
      if(isFlagSymbol(lines[0][tapes+1])) {
        tapes--;
        flagTapes++;
      } else {
        break;
      }
    }
  }

  Transducer* t = new Transducer(tapes, flagTapes);
  StringInterner& alpha = t->getAlphabet();
  for(auto line : lines) {
    if(line.size() == transition_len) {
      Transition tr;
      tr.symbols.resize(tapes);
      tr.flags.resize(flagTapes);
      state_number_t src = stoul(line[0]);
      state_number_t trg = stoul(line[1]);
      while(t->size() <= src || t->size() <= trg) {
        t->addState();
      }
      tr.weight = (weighted ? stoul(line.back()) : 0.000);
      if(flagsSeparate && isFlagSymbol(line[2])) {
        bool is_flag = true;
        for(size_t i = 3; i < tapes + 2; i++) {
          is_flag = false;
          if(line[i] != line[2]) {
            break;
          }
        }
        if(is_flag) {
          tr.flags[0] = parseFlag(line[2], alpha);
          t->insertTransition(src, trg, tr);
          continue;
        }
      }
      for(size_t i = 0; i < tapes; i++) {
        tr.symbols[i] = alpha.internName(line[i+2]);
      }
      if(!flagsSeparate) {
        for(size_t i = 0; i < flagTapes; i++) {
          tr.flags[i] = parseFlag(line[2 + tapes + i], alpha);
        }
      }
      t->insertTransition(src, trg, tr);
    } else if(line.size() == final_len) {
      state_number_t state = stoul(line[0]);
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
  return t;
}

void
writeATT(Transducer* t, UFILE* out, bool writeWeights)
{
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
        for(auto& flag : tr.flags) {
          u_fprintf(out, "@");
          switch(flag.type) {
            case Clear:
              u_fprintf(out, "C"); break;
            case Positive:
              u_fprintf(out, "P"); break;
            case Negative:
              u_fprintf(out, "N"); break;
            case Require:
              u_fprintf(out, "R"); break;
            case Disallow:
              u_fprintf(out, "D"); break;
            case Unification:
              u_fprintf(out, "U"); break;
            default:
              ; // None
          }
          if(flag.type != None)
          {
            u_fprintf(out, ".");
            alphabet.write_symbol(out, flag.sym, false);
            if(flag.type == Positive || flag.type == Negative ||
               flag.type == Unification ||
               ((flag.type == Require || flag.type == Disallow) &&
                (unsigned int)flag.val != 0)) {
              u_fprintf(out, ".");
              alphabet.write_symbol(out, flag.val, false);
            }
          }
          u_fprintf(out, "@\t");
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
