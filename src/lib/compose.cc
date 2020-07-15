#include "compose.h"
#include <stdexcept>
#include <deque>
#include <map>

#include <iostream>

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
  bool clearingBacklog;
  std::vector<std::deque<string_ref>> left_backlog;
  std::vector<std::deque<string_ref>> right_backlog;
  //std::vector<BacklogDependency> deps; // this might not be the right data structure
};

std::ostream&
operator<<(std::ostream& s, const ComposedState& next)
{
  s << "(" << next.left_state << ", " << next.right_state << ") -> " << next.out_state;
  s << "{" << next.clearingBacklog << "}\n";
  s << "left_backlog:\n";
  for(auto log : next.left_backlog) {
    s << "[";
    for(auto it : log) {
      s << it.i << ", ";
    }
    s << "]\n";
  }
  s << "right_backlog:\n";
  for(auto log : next.right_backlog) {
    s << "[";
    for(auto it : log) {
      s << it.i << ", ";
    }
    s << "]\n";
  }
  return s;
}

bool
operator==(const std::deque<string_ref>& a, const std::deque<string_ref>& b)
{
  if(a.size() != b.size()) {
    return false;
  }
  for(size_t i = 0; i < a.size(); i++) {
    if(a[i] != b[i]) {
      return false;
    }
  }
  return true;
}

bool
operator==(const std::vector<std::deque<string_ref>>& a, const std::vector<std::deque<string_ref>>& b)
{
  if(a.size() != b.size()) {
    return false;
  }
  for(size_t i = 0; i < a.size(); i++) {
    if(!(a[i] == b[i])) {
      return false;
    }
  }
  return true;
}

bool
backlogEmpty(const std::vector<std::deque<string_ref>>& log)
{
  for(auto it : log) {
    if(!it.empty()) {
      return false;
    }
  }
  return true;
}

bool
backlogEmpty(const ComposedState& s)
{
  return backlogEmpty(s.left_backlog) && backlogEmpty(s.right_backlog);
}

string_ref
stepBacklog(std::deque<string_ref>& backlog, std::map<string_ref, string_ref>& update, string_ref sym)
{
  if(sym != string_ref(0)) {
    string_ref up = (update.empty() ? sym : update[sym]);
    backlog.push_back(up);
  }
  string_ref ret = string_ref(0);
  if(!backlog.empty()) {
    ret = backlog.front();
    backlog.pop_front();
  }
  return ret;
}

bool
composeTransition(Transition& a, Transition& b, ComposedState* state, Transition* out, SymbolTable& table, std::map<string_ref, string_ref>& left_update, std::map<string_ref, string_ref>& right_update, std::vector<size_t> placement, bool flagsAsEpsilon)
{
  out->weight = a.weight + b.weight;
  for(size_t i = 0; i < a.symbols.size(); i++) {
    out->symbols[i] = stepBacklog(state->left_backlog[i], left_update, a.symbols[i]);
  }
  for(size_t i = 0; i < b.symbols.size(); i++) {
    string_ref rsym = stepBacklog(state->right_backlog[i], right_update, b.symbols[i]);
    size_t loc = placement[i];
    if(loc >= a.symbols.size()) {  // not composing
      out->symbols[loc] = rsym;
      continue;
    } else if(rsym == out->symbols[loc]) {  // simple equality
      continue;
    } else if(table.isEpsilon(out->symbols[loc], flagsAsEpsilon)) {  // left epsilon
      state->right_backlog[i].push_front(rsym);
    } else if(table.isEpsilon(rsym, flagsAsEpsilon)) {  // right epsilon
      state->left_backlog[loc].push_front(out->symbols[loc]);
      out->symbols[loc] = rsym; // rsym might be a flag, so keep it
    } else {  // failed to match
      return false;
    }
  }
  return true;
}

Transducer*
compose(Transducer* a, Transducer* b, std::vector<std::pair<UnicodeString, UnicodeString>> tapes, bool flagsAsEpsilon)
{
  if(tapes.size() > a->getTapeCount() || tapes.size() > b->getTapeCount()) {
    throw std::runtime_error("Transducer has fewer tapes than compose instructions.");
  }
  size_t tapeCount = a->getTapeCount() + b->getTapeCount() - tapes.size();

  std::vector<size_t> placement = std::vector<size_t>(b->getTapeCount(), 0);
  auto left_tapes = a->getTapeInfo();
  auto right_tapes = b->getTapeInfo();
  std::map<UnicodeString, TapeInfo> mergedTapeInfo = left_tapes;
  for(auto names : tapes) {
    if(left_tapes.find(names.first) == left_tapes.end() ||
       right_tapes.find(names.second) == right_tapes.end()) {
      throw std::runtime_error("Attempt to compose along non-existent tapes");
    }
    size_t ltape = left_tapes[names.first].index;
    size_t rtape = right_tapes[names.second].index;
    placement[rtape] = ltape;
    if(mergedTapeInfo.find(names.second) == mergedTapeInfo.end()) {
      TapeInfo info;
      info.index = ltape;
      info.flags = left_tapes[names.first].flags | right_tapes[names.second].flags;
      mergedTapeInfo[names.second] = info;
      // TODO: the flags are probably going to end up wrong in several places
    }
  }
  size_t n = a->getTapeCount();
  for(size_t i = 0; i < placement.size(); i++) {
    if(placement[i] == 0) {
      placement[i] = n;
      n++;
    }
  }
  for(auto it : right_tapes) {
    if(mergedTapeInfo.find(it.first) == mergedTapeInfo.end()) {
      TapeInfo info;
      info.index = placement[it.second.index];
      info.flags = it.second.flags;
      mergedTapeInfo[it.first] = info;
    }
  }

  Transducer* t = new Transducer(tapeCount);
  t->setTapeInfo(mergedTapeInfo);
  std::map<string_ref, string_ref> left_update = t->getAlphabet().merge(a->getAlphabet());
  std::map<string_ref, string_ref> right_update = t->getAlphabet().merge(b->getAlphabet());

  // TODO: merge symbol tables

  std::deque<ComposedState> todo_list;
  std::map<state_t, std::map<state_t, std::vector<ComposedState>>> done_list;

  ComposedState init;
  init.left_state = 0;
  init.right_state = 0;
  init.out_state = 0;
  init.clearingBacklog = false;
  init.left_backlog.resize(a->getTapeCount());
  init.right_backlog.resize(b->getTapeCount());
  todo_list.push_back(init);
  done_list[0][0].push_back(init);

  auto left_transitions = a->getTransitions();
  auto right_transitions = b->getTransitions();

  while(todo_list.size() > 0) {
    ComposedState cur = todo_list.front();
    todo_list.pop_front();
    if(a->isFinal(cur.left_state) && b->isFinal(cur.right_state)) {
      if(backlogEmpty(cur)) {
        t->setFinal(cur.out_state);
        continue;
      } else if(!cur.clearingBacklog) {
        ComposedState end = cur;
        end.clearingBacklog = true;
        todo_list.push_back(end);
      }
    }
    std::map<state_t, std::vector<Transition>> lblob = left_transitions[cur.left_state];
    if(lblob.empty() && cur.clearingBacklog) {
      lblob[0].resize(1);
    }
    for(auto lvect : lblob) {
      state_t lstate = lvect.first;
      for(auto ltrans : lvect.second) {
        std::map<state_t, std::vector<Transition>> rblob = right_transitions[cur.right_state];
        if(rblob.empty() && cur.clearingBacklog) {
          rblob[0].resize(1);
        }
        for(auto rvect : rblob) {
          state_t rstate = rvect.first;
          for(auto rtrans : rvect.second) {
            ComposedState next = cur;
            if(!cur.clearingBacklog) {
              next.left_state = lstate;
              next.right_state = rstate;
            }
            Transition tr;
            tr.symbols.resize(tapeCount);
            Transition left, right;
            if(!cur.clearingBacklog) {
              left = ltrans;
              right = rtrans;
            } else if(backlogEmpty(cur.right_backlog)) {
              left.symbols.resize(a->getTapeCount());
              right = rtrans;
            } else if(backlogEmpty(cur.left_backlog)) {
              left = ltrans;
              right.symbols.resize(b->getTapeCount());
            } else {
              left = ltrans;
              right = rtrans;
            }
            if(composeTransition(left, right, &next, &tr, t->getSymbols(),
                                 left_update, right_update, placement, flagsAsEpsilon)) {
              if(done_list.find(lstate) != done_list.end() &&
                 done_list[lstate].find(rstate) != done_list[lstate].end())
              {
                bool found = false;
                for(auto it : done_list[lstate][rstate]) {
                  if(it.clearingBacklog == next.clearingBacklog &&
                     it.left_backlog == next.left_backlog &&
                     it.right_backlog == next.right_backlog)
                  {
                    t->insertTransition(cur.out_state, it.out_state, tr);
                    found = true;
                    break;
                  }
                }
                if(found) {
                  continue;
                }
              }
              next.out_state = t->insertTransition(cur.out_state, tr);
              done_list[lstate][rstate].push_back(next);
              todo_list.push_back(next);
            }
            if(cur.clearingBacklog && backlogEmpty(cur.left_backlog)) {
              break;
            }
          }
        }
        if(cur.clearingBacklog && backlogEmpty(cur.right_backlog)) {
          break;
        }
      }
    }
  }
  return t;
}
