#ifndef _LIB_COMPOSE_H_
#define _LIB_COMPOSE_H_

#include "transducer.h"
#include <vector>
#include <unicode/unistr.h>
#include <map>
#include <vector>
#include <deque>

struct BacklogDependency {
  size_t source_tape;
  size_t reference_tape;
  size_t source_index;
  size_t reference_index;
};

struct ComposedState {
  state_t left_state;
  state_t right_state;
  state_t out_state;
  std::vector<std::deque<string_ref>> left_backlog;
  std::vector<std::deque<string_ref>> right_backlog;
  //std::vector<BacklogDependency> deps; // this might not be the right data structure
};

class Composer {
private:
  Transducer* a;
  Transducer* b;
  Transducer* t;
  std::map<string_ref, string_ref> left_update;
  std::map<string_ref, string_ref> right_update;
  std::vector<size_t> placement;
  bool flagsAsEpsilon;
  size_t tapeCount;
  std::deque<ComposedState> todo_list;
  std::map<state_t, std::map<state_t, std::vector<ComposedState>>> done_list;
  Transition left_epsilon;
  Transition right_epsilon;

  bool isLeftEpsilon(Transition& tr);
  bool isRightEpsilon(Transition& tr);
  bool backlogsOverlap(const ComposedState& s);

  bool composeTransition(Transition& a, Transition& b, ComposedState* state, Transition* out);
  bool processTransitionPair(ComposedState& state, Transition& left, Transition& right, state_t lstate, state_t rstate);
public:
  Composer(Transducer* a, Transducer* b, std::vector<std::pair<UnicodeString, UnicodeString>> tapes, bool flagsAsEpsilon = true);
  ~Composer();
  Transducer* compose();
};

Transducer* compose(Transducer* a, Transducer* b, std::vector<std::pair<UnicodeString, UnicodeString>> tapes, bool flagsAsEpsilon = true);

#endif
