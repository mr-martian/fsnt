#include <array>
#include "string_interner.h"

// These values are declared explicitly to ensure consistent serialization
enum FlagSymbolType {
  None        = 0,
  Clear       = 1,
  Positive    = 2,
  Negative    = 3,
  Require     = 4,
  Disallow    = 5,
  Unification = 6
};

struct FlagSymbol {
  FlagSymbolType type;
  string_ref sym;
  string_ref val;
};

struct Transition {
  std::vector<string_ref> symbols;
  std::vector<FlagSymbol> flags;
  double weight;
};
