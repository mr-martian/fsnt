#ifndef _UTIL_TRANSITION_ITER_H_
#define _UTIL_TRANSITION_ITER_H_

#include "../transducer.h"

class TransitionIterator {
private:
  const std::map<state_t, std::vector<Transition>>* transitions;
  std::map<state_t, std::vector<Transition>>::const_iterator mit;
  std::vector<Transition>::const_iterator vit;
  std::pair<state_t, Transition> cur;
public:
  TransitionIterator(const std::map<state_t, std::vector<Transition>>& tr);
  TransitionIterator(const TransitionIterator& other);
  ~TransitionIterator();

  const std::pair<state_t, Transition>& operator*() const;
  TransitionIterator operator++(int);
  TransitionIterator& operator++();
  TransitionIterator operator--(int);
  TransitionIterator& operator--();
  bool operator==(TransitionIterator& other) const;
  bool operator!=(TransitionIterator& other) const;

  TransitionIterator begin();
  TransitionIterator end();
};

TransitionIterator iter_state(Transducer* t, state_t s);

#endif
