#include "strip.h"

#include "utils/set_utils.h"

#include <set>
#include <map>
#include <vector>
#include <deque>

void
followLinks(std::map<state_t, std::vector<state_t>>& con,
            std::set<state_t>& result, std::deque<state_t>& todo)
{
  while(todo.size() > 0) {
    state_t cur = todo.front();
    todo.pop_front();
    if(result.find(cur) == result.end()) {
      result.insert(cur);
      for(auto next : con[cur]) {
        todo.push_back(next);
      }
    }
  }
}

Transducer*
strip(Transducer* t)
{
  std::map<state_t, std::vector<state_t>> forward, backward;
  auto trans = t->getTransitions();
  for(size_t state = 0; state < trans.size(); state++) {
    for(auto it : trans[state]) {
      forward[state].push_back(it.first);
      backward[it.first].push_back(state);
    }
  }
  std::set<state_t> accessible, coaccessible;
  std::deque<state_t> todo;
  todo.push_back(0);
  followLinks(forward, accessible, todo);
  for(auto it : t->getFinals()) {
    todo.push_back(it.first);
  }
  followLinks(backward, coaccessible, todo);
  std::set<state_t> keep = intersectset(accessible, coaccessible);
  std::map<state_t, state_t> new_states;
  new_states[0] = 0;
  Transducer* ret = t->emptyCopy();
  for(auto state : keep) {
    if(new_states.find(state) == new_states.end()) {
      new_states[state] = ret->addState();
    }
    for(auto it : t->getTransitions()[state]) {
      if(keep.find(it.first) == keep.end()) {
        continue;
      }
      if(new_states.find(it.first) == new_states.end()) {
        new_states[it.first] = ret->addState();
      }
      for(auto tr : it.second) {
        ret->insertTransition(new_states[state], new_states[it.first], tr);
      }
    }
  }
  for(auto fin : t->getFinals()) {
    if(new_states.find(fin.first) != new_states.end()) {
      ret->setFinal(new_states[fin.first], fin.second);
    }
  }
  return ret;
}
