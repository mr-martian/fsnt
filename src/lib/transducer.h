#ifndef _LIB_TRANSDUCER_H_
#define _LIB_TRANSDUCER_H_

#include "transition.h"
#include "symbol_table.h"

#include <unicode/unistr.h>
#include <unicode/ustdio.h>
#include <cstdio>
#include <map>
#include <vector>

typedef size_t state_t;

enum TapeInfoFlags {
  SymbolTape = 1,
  FlagTape   = 2
};

struct TapeInfo {
  size_t index;
  unsigned int flags;
};

class Transducer {
private:
  size_t tapeCount;
  SymbolTable alphabet;
  std::vector<std::map<state_t, std::vector<Transition>>> transitions;
  std::map<state_t, double> finals;
  std::map<UnicodeString, TapeInfo> tapeNames;
public:
  Transducer(size_t tapes);
  ~Transducer();

  Transducer* emptyCopy();

  SymbolTable& getAlphabet();
  std::vector<std::map<state_t, std::vector<Transition>>>& getTransitions();
  std::map<state_t, double>& getFinals();
  std::map<UnicodeString, TapeInfo>& getTapeInfo();
  void setTapeInfo(std::map<UnicodeString, TapeInfo> names);
  void setTapeInfo(UnicodeString name, TapeInfo info);
  size_t getTapeCount();
  size_t size();

  state_t addState();
  void addStates(size_t n);
  bool isFinal(state_t state);
  void setFinal(state_t state, double weight = 0.000);
  void setNotFinal(state_t state);

  // insert trans connecting src to trg
  void insertTransition(state_t src, state_t trg, Transition trans);
  // create a new state and connect it to src with trans
  // if checkExists = true and there is already an equivalent transition
  // from src, return that transition's destination
  // returns new state
  state_t insertTransition(state_t src, Transition trans, bool checkExists = false);
  // TODO: shortcuts for adding common types of transitions
  // including checking if the transition already exists
  state_t insertEpsilonTransition(state_t src, bool checkExists = false, double weight = 0.000);
  void insertEpsilonTransition(state_t src, state_t trg, double weight = 0.000);

  void eraseTransitions(state_t src, state_t trg);
};

#endif
