#include "relabel.h"

Transducer*
relabel(Transducer* t, std::map<string_ref, string_ref>& update)
{
  Transducer* ret = t->emptyCopy();
  ret->addStates(t->size()-1);
  auto trans = t->getTransitions();
  for(size_t src = 0; src < trans.size(); src++) {
    for(auto it : trans[src]) {
      for(auto old_tr : it.second) {
        Transition new_tr;
        new_tr.symbols = old_tr.symbols;
        new_tr.weight = old_tr.weight;
        for(size_t i = 0; i < new_tr.symbols.size(); i++) {
          if(update.find(new_tr.symbols[i]) != update.end()) {
            new_tr.symbols[i] = update[old_tr.symbols[i]];
          }
        }
        ret->insertTransition(src, it.first, new_tr);
      }
    }
  }
  for(auto it : t->getFinals()) {
    ret->setFinal(it.first, it.second);
  }
  return ret;
}
