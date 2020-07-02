#ifndef _LIB_TRANSDUCER_H_
#define _LIB_TRANSDUCER_H_

#include "transition.h"
#include "string_interner.h"
#include "symbol_table.h"

#include <unicode/unistr.h>
#include <unicode/ustdio.h>
#include <cstdio>
#include <map>
#include <vector>

typedef size_t state_number_t;

class Transducer {
private:
  size_t tapes;
  size_t flagTapes;
  StringInterner alphabet;
  SymbolTable symbols;
  std::vector<std::map<state_number_t, std::vector<Transition>>> transitions;
  std::map<state_number_t, double> finals;
public:
  Transducer(size_t tapes, size_t flagTapes);
  ~Transducer();

  StringInterner& getAlphabet();
  SymbolTable& getSymbols();
  std::vector<std::map<state_number_t, std::vector<Transition>>>& getTransitions();
  std::map<state_number_t, double>& getFinals();
  size_t getTapes();
  size_t getFlagTapes();
  size_t size();

  state_number_t addState();
  void addStates(size_t n);
  bool isFinal(state_number_t state);
  void setFinal(state_number_t state, double weight = 0.000);
  void setNotFinal(state_number_t state);

  // insert trans connecting src to trg
  void insertTransition(state_number_t src, state_number_t trg, Transition trans);
  // create a new state and connect it to src with trans
  // returns new state
  state_number_t insertTransition(state_number_t src, Transition trans);
  // TODO: shortcuts for adding common types of transitions
  // including checking if the transition already exists
};

#endif
