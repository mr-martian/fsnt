#include "to_lookup.h"

#include "strip.h"

#include <map>
#include <vector>
#include <set>
#include <stdexcept>

Transducer*
cleanTransducer(Transducer* t, size_t input_tape, size_t* stringTapes)
{
  auto alpha = t->getAlphabet();
  std::vector<std::set<string_ref>> sym_locs;
  std::vector<std::set<string_ref>> flag_locs;
  sym_locs.resize(t->getTapeCount());
  flag_locs.resize(t->getTapeCount());
  for(auto state : t->getTransitions()) {
    for(auto blob : state) {
      for(auto tr : blob.second) {
        for(size_t i = 0; i < tr.symbols.size(); i++) {
          string_ref sym = tr.symbols[i];
          if(sym == string_ref(0)) {
            continue;
          }
          if(alpha.isDefined(sym)) {
            if(alpha.lookup(sym).type != FlagSymbol) {
              throw std::runtime_error("Complex symbols are not allowed in optimized lookup transducers.");
            } else {
              flag_locs[i].insert(sym);
            }
          } else {
            sym_locs[i].insert(sym);
          }
        }
      }
    }
  }
  std::vector<size_t> symTapes = std::vector<size_t>(t->getTapeCount(), 0);
  std::vector<size_t> flagTapes = std::vector<size_t>(t->getTapeCount(), 0);
  if(sym_locs[input_tape].empty()) {
    throw std::runtime_error("Designated input tape contains no symbols.");
  }
  size_t n = 1;
  for(size_t i = 0; i < sym_locs.size(); i++) {
    if(i == input_tape) {
      continue;
    }
    if(!sym_locs[i].empty()) {
      symTapes[i] = n;
      n++;
    }
  }
  *stringTapes = n;
  for(size_t i = 0; i < flag_locs.size(); i++) {
    if(!flag_locs[i].empty()) {
      flagTapes[i] = n;
      n++;
    }
  }
  Transducer* ret = new Transducer(n);
  /*
    TODO:
    - reorder alphabet so that all input symbols are listed first
      - components of flags might be messy
        (although maybe temporarily referring to non-existent entries is ok?)
    - ensure that input tape is semi-deterministic
      - every input symbol should only lead to 1 state
      0 1 a b
      0 2 a c
      becomes
      0 1 a b
      0 2 0 0
      2 3 a c
      unless there's a better algorithm
      (this one makes multiple input epsilons tricky)
  */
  return ret;
}

void
toLookup(Transducer* t, FILE* out)
{
  Transducer* temp = strip(t);
  size_t stringTapes;
  Transducer* clean = cleanTransducer(temp, 0, &stringTapes);
  delete temp;
  // blah
  delete clean;
}
