#include "transition.h"

bool
operator==(const Transition& a, const Transition& b)
{
  if(a.symbols.size() != b.symbols.size() || a.weight != b.weight) {
    return false;
  }
  for(size_t i = 0; i < a.symbols.size(); i++) {
    if(a.symbols[i] != b.symbols[i]) {
      return false;
    }
  }
  return true;
}
