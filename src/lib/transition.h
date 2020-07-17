#ifndef _LIB_TRANSITION_H_
#define _LIB_TRANSITION_H_

#include <vector>
#include "string_interner.h"

struct Transition {
  std::vector<string_ref> symbols;
  double weight;
};

bool operator==(const Transition& a, const Transition& b);

#endif
