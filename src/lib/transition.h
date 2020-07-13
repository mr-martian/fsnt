#include <vector>
#include "string_interner.h"

struct Transition {
  std::vector<string_ref> symbols;
  double weight;
};
