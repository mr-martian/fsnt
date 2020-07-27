#include "optimize_flags.h"

#include <deque>
#include <map>
#include <set>
#include <vector>

#include "utils/set_utils.h"
#include "relabel.h"
#include "strip.h"

#include <iostream>

bool
updateState(std::map<string_ref, std::set<int>>& state, std::map<string_ref, std::set<int>>& update)
{
  bool changed = false;
  for(auto it : update) {
    if(state.find(it.first) == state.end()) {
      changed = true;
    }
    for(auto val : it.second) {
      bool ch = state[it.first].insert(val).second;
      if(ch) {
        changed = true;
      }
    }
  }
  return changed;
}

Transducer*
optimizeFlags(Transducer* t)
{
  std::map<string_ref, FlagSymbolStruct> flags;
  std::set<string_ref> flagNames;
  std::map<string_ref, std::set<string_ref>> flagValues;
  for(auto it : t->getAlphabet().getDefined()) {
    if(it.second.type == FlagSymbol) {
      flags[it.first] = it.second.flag;
      flagNames.insert(it.second.flag.sym);
      if(it.second.flag.val != string_ref(0)) {
        flagValues[it.second.flag.sym].insert(it.second.flag.val);
      }
    }
  }
  std::vector<std::map<string_ref, std::set<int>>> states;
  states.resize(t->size());
  std::set<state_t> reached;
  std::map<state_t, std::map<state_t, bool>> connected;
  std::map<string_ref, std::set<string_ref>> required;
  std::deque<state_t> todo;
  todo.push_back(0);
  for(auto name : flagNames) {
    states[0][name].insert(0);
  }
  while(todo.size() > 0) {
    state_t cur = todo.front();
    todo.pop_front();
    reached.insert(cur);
    for(auto it : t->getTransitions()[cur]) {
      state_t next = it.first;
      bool changed = false;
      connected[cur][next] = false;
      for(auto tr : it.second) {
        std::map<string_ref, std::set<int>> cur_state = states[cur];
        bool reachable = true;
        std::set<string_ref> seen_here;
        for(auto sym : tr.symbols) {
          if(seen_here.find(sym) != seen_here.end()) {
            continue;
          } else {
            seen_here.insert(sym);
          }
          if(flags.find(sym) != flags.end()) {
            string_ref name = flags[sym].sym;
            int val = (int)flags[sym].val.i;
            switch(flags[sym].type) {
              case Clear:
                cur_state[name].clear();
                cur_state[name].insert(0);
                break;
              case Positive:
                cur_state[name].clear();
                cur_state[name].insert(val);
                break;
              case Negative:
                cur_state[name].clear();
                cur_state[name].insert(-val);
                break;
              case Require:
                required[name].insert(flags[sym].val);
                if(val == 0) {
                  cur_state[name].erase(0);
                  if(cur_state[name].empty()) {
                    reachable = false;
                  }
                } else if(cur_state[name].find(val) != cur_state[name].end()) {
                  cur_state[name].clear();
                  cur_state[name].insert(val);
                } else {
                  reachable = false;
                }
                break;
              case Disallow:
                required[name].insert(flags[sym].val);
                if(val == 0) {
                  if(cur_state[name].find(0) == cur_state[name].end()) {
                    reachable = false;
                  }
                  cur_state[name].clear();
                  cur_state[name].insert(0);
                } else {
                  cur_state[name].erase(val);
                }
                break;
              case Unification:
                if(!cur_state[name].empty() &&
                   cur_state[name].find(0) == cur_state[name].end() &&
                   cur_state[name].find(val) == cur_state[name].end()) {
                  reachable = false;
                }
                if(cur_state[name].size() > 1 ||
                   (cur_state[name].size() == 1 && *cur_state[name].begin() != 0)) {
                  required[name].insert(flags[sym].val);
                }
                cur_state[name].clear();
                cur_state[name].insert(val);
                break;
            }
          }
        }
        if(reachable) {
          connected[cur][next] = true;
          if(updateState(states[next], cur_state)) {
            changed = true;
          }
        }
      }
      if(changed || reached.find(next) == reached.end()) {
        todo.push_back(next);
      }
    }
  }
  std::map<string_ref, string_ref> update;
  SymbolTable& alpha = t->getAlphabet();
  for(auto flag : flagNames) {
    std::set<string_ref> needed;
    std::set<string_ref> unused;
    bool drop = (required.find(flag) == required.end() ||
                 required[flag].find(string_ref(0)) == required[flag].end());
    for(auto val : flagValues[flag]) {
      if(required[flag].find(val) == required[flag].end()) {
        unused.insert(val);
      } else {
        needed.insert(val);
      }
    }
    if(needed.size() == 1 && *needed.begin() != string_ref(0) && unused.empty()) {
      unused.swap(needed);
    }
    if(drop) {
      for(auto val : unused) {
        update[alpha.makeFlag(Clear,       flag, val)] = string_ref(0);
        update[alpha.makeFlag(Positive,    flag, val)] = string_ref(0);
        update[alpha.makeFlag(Negative,    flag, val)] = string_ref(0);
        update[alpha.makeFlag(Require,     flag, val)] = string_ref(0);
        update[alpha.makeFlag(Disallow,    flag, val)] = string_ref(0);
        update[alpha.makeFlag(Unification, flag, val)] = string_ref(0);
      }
      unused.clear();
    }
    if(needed.empty()) {
      continue;
    }
    std::map<string_ref, std::set<string_ref>> mergeable;
    std::set<string_ref> all = unionset(needed, unused);
    for(auto val : all) {
      mergeable[val] = all;
      mergeable[val].erase(val);
    }
    for(auto it : states) {
      for(auto v1 : it[flag]) {
        if(v1 <= 0 || all.find(string_ref((unsigned int) v1)) == needed.end()) {
          continue;
        }
        for(auto v2 : it[flag]) {
          if(v2 <= 0 || v2 == v1 || all.find(string_ref((unsigned int) v2)) == needed.end()) {
            continue;
          }
          mergeable[string_ref((unsigned int) v1)].erase(string_ref((unsigned int) v2));
        }
      }
    }
    while(!mergeable.empty()) {
      string_ref key = (*mergeable.begin()).first;
      if(mergeable[key].empty()) {
        mergeable.erase(key);
        continue;
      }
      string_ref m = *mergeable[key].begin();
      mergeable[key] = intersectset(mergeable[key], mergeable[m]);
      mergeable[key].erase(key);
      mergeable[key].erase(m);
      mergeable.erase(m);
      for(auto it : mergeable) {
        if(it.second.find(m) != it.second.end()) {
          it.second.erase(m);
          it.second.insert(key);
        }
      }
      update[alpha.makeFlag(Clear,       flag, m)] = alpha.makeFlag(Clear,       flag, key);
      update[alpha.makeFlag(Positive,    flag, m)] = alpha.makeFlag(Positive,    flag, key);
      update[alpha.makeFlag(Negative,    flag, m)] = alpha.makeFlag(Negative,    flag, key);
      update[alpha.makeFlag(Require,     flag, m)] = alpha.makeFlag(Require,     flag, key);
      update[alpha.makeFlag(Disallow,    flag, m)] = alpha.makeFlag(Disallow,    flag, key);
      update[alpha.makeFlag(Unification, flag, m)] = alpha.makeFlag(Unification, flag, key);
    }
  }
  Transducer* rel = relabel(t, update);
  for(auto it : connected) {
    for(auto it2 : it.second) {
      if(!it2.second) {
        rel->eraseTransitions(it.first, it2.first);
      }
    }
  }
  Transducer* s = strip(rel);
  delete rel;
  return s;
}
