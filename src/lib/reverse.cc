#include "reverse.h"
#include <map>

Transducer*
reverse(Transducer* t)
{
  Transducer* ret = new Transducer(t->getTapeCount());
  ret->getAlphabet().merge(t->getAlphabet());
  ret->setTapeInfo(t->getTapeInfo());
  // TODO: symbol table

  ret->addStates(t->size()+1);
  state_t end = t->size();
  ret->setFinal(end);
  for(auto fin : t->getFinals()) {
    ret->insertEpsilonTransition(0, fin.first, fin.second);
  }
  auto trans = t->getTransitions();
  for(size_t idx = 0; idx < trans.size(); idx++) {
    state_t trg = (idx ? idx : end);
    for(auto it : trans[idx]) {
      state_t src = (it.first ? it.first : end);
      for(auto tr : it.second) {
        ret->insertTransition(src, trg, tr);
      }
    }
  }
  return ret;
}
