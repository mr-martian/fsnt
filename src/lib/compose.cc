#include "compose.h"
#include <stdexcept>
#include <deque>
#include <map>

#include <iostream>
#include <unicode/ustream.h>

std::ostream&
operator<<(std::ostream& s, const ComposedState& next)
{
  s << "(" << next.left_state << ", " << next.right_state << ") -> " << next.out_state;// << std::endl;
  s << " left_backlog: ";
  for(auto log : next.left_backlog) {
    s << "[";
    for(auto it : log) {
      s << it.i << ", ";
    }
    s << "] ";
  }
  s << "right_backlog: ";
  for(auto log : next.right_backlog) {
    s << "[";
    for(auto it : log) {
      s << it.i << ", ";
    }
    s << "] ";
  }
  return s;
}

std::ostream&
operator<<(std::ostream& s, const Transition& tr)
{
  s << "[";
  for(auto sym : tr.symbols) { s << "\t" << sym.i; }
  s << "\t]";
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
Composer::composeTransition(Transition& l, Transition& r, ComposedState* state, Transition* out)
{
  SymbolTable& table = t->getAlphabet();
  out->weight = l.weight + r.weight;
  for(size_t i = 0; i < l.symbols.size(); i++) {
    out->symbols[i] = stepBacklog(state->left_backlog[i], left_update, l.symbols[i]);
  }
  for(size_t i = 0; i < r.symbols.size(); i++) {
    string_ref rsym = stepBacklog(state->right_backlog[i], right_update, r.symbols[i]);
    size_t loc = placement[i];
    if(loc >= l.symbols.size()) {  // not composing
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
      if(table.isDefined(rsym)) {
        auto exp = table.lookup(rsym);
        if(exp.type == UnionSymbol &&
           exp.syms.find(out->symbols[loc]) != exp.syms.end()) {
          continue;
        }
      }
      return false;
    }
  }
  return true;
}

Composer::Composer(Transducer* a_, Transducer* b_, std::vector<std::pair<UnicodeString, UnicodeString>> tapes, bool flagsAsEpsilon_)
{
  a = a_;
  b = b_;
  flagsAsEpsilon = flagsAsEpsilon_;

  if(tapes.size() > a->getTapeCount() || tapes.size() > b->getTapeCount()) {
    throw std::runtime_error("Transducer has fewer tapes than compose instructions.");
  }
  tapeCount = a->getTapeCount() + b->getTapeCount() - tapes.size();

  std::vector<int> placement_temp = std::vector<int>(b->getTapeCount(), -1);
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
    placement_temp[rtape] = ltape;
    if(mergedTapeInfo.find(names.second) == mergedTapeInfo.end()) {
      TapeInfo info;
      info.index = ltape;
      info.flags = left_tapes[names.first].flags | right_tapes[names.second].flags;
      mergedTapeInfo[names.second] = info;
      // TODO: the flags are probably going to end up wrong in several places
    }
  }
  size_t n = a->getTapeCount();
  for(size_t i = 0; i < placement_temp.size(); i++) {
    if(placement_temp[i] == -1) {
      placement_temp[i] = n;
      n++;
    }
    placement.push_back((size_t)placement_temp[i]);
  }
  for(auto it : right_tapes) {
    if(mergedTapeInfo.find(it.first) == mergedTapeInfo.end()) {
      TapeInfo info;
      info.index = (size_t)placement[it.second.index];
      info.flags = it.second.flags;
      mergedTapeInfo[it.first] = info;
    }
  }

  t = new Transducer(tapeCount);
  t->setTapeInfo(mergedTapeInfo);
  left_update = t->getAlphabet().merge(a->getAlphabet());
  right_update = t->getAlphabet().merge(b->getAlphabet());

  left_epsilon.symbols = std::vector<string_ref>(a->getTapeCount(), string_ref(0));
  right_epsilon.symbols = std::vector<string_ref>(b->getTapeCount(), string_ref(0));
}

Composer::~Composer()
{
}

bool
Composer::processTransitionPair(ComposedState& state, Transition& left, Transition& right, state_t lstate, state_t rstate)
{
  ComposedState next = state;
  next.left_state = lstate;
  next.right_state = rstate;
  Transition tr;
  tr.symbols.resize(tapeCount);

  //std::cerr << "processTransitionPair\n\tnext = " << next << "\n\tleft = " << left << "\n\tright = " << right << std::endl;

  if(composeTransition(left, right, &next, &tr)) {
    //std::cerr << "\tmatched" << std::endl;
    //std::cerr << "\t-> " << tr << std::endl;
    if(done_list.find(lstate) != done_list.end() &&
       done_list[lstate].find(rstate) != done_list[lstate].end())
    {
      for(auto it : done_list[lstate][rstate]) {
        if(it.left_backlog == next.left_backlog &&
           it.right_backlog == next.right_backlog)
        {
          t->insertTransition(state.out_state, it.out_state, tr);
          return true;
        }
      }
    }
    next.out_state = t->insertTransition(state.out_state, tr);
    done_list[lstate][rstate].push_back(next);
    todo_list.push_back(next);
    return true;
  }
  return false;
}

bool
Composer::isLeftEpsilon(Transition& tr)
{
  for(auto loc : placement) {
    if(loc < tr.symbols.size() && tr.symbols[loc] != string_ref(0)) {
      return false;
    }
  }
  return true;
}

bool
Composer::isRightEpsilon(Transition& tr)
{
  for(size_t i = 0; i < placement.size(); i++) {
    if(placement[i] < a->getTapeCount() && tr.symbols[i] != string_ref(0)) {
      return false;
    }
  }
  return true;
}

bool
Composer::backlogsOverlap(const ComposedState& s)
{
  for(size_t i = 0; i < placement.size(); i++) {
    if(placement[i] < s.left_backlog.size()) {
      if(s.left_backlog[placement[i]].size() > 0 &&
         s.right_backlog[i].size() > 0) {
        return true;
      }
    }
  }
  return false;
}

Transducer*
Composer::compose()
{
  ComposedState init;
  init.left_state = 0;
  init.right_state = 0;
  init.out_state = 0;
  init.left_backlog.resize(a->getTapeCount());
  init.right_backlog.resize(b->getTapeCount());
  todo_list.push_back(init);
  done_list[0][0].push_back(init);

  auto left_transitions = a->getTransitions();
  auto right_transitions = b->getTransitions();

  while(todo_list.size() > 0) {
    ComposedState cur = todo_list.front();
    todo_list.pop_front();
    state_t lstate = cur.left_state;
    state_t rstate = cur.right_state;
    bool lempty = backlogEmpty(cur.left_backlog);
    bool rempty = backlogEmpty(cur.right_backlog);

    //std::cerr << std::endl << "cur = " << cur << std::endl;

    if(backlogsOverlap(cur)) {
      processTransitionPair(cur, left_epsilon, right_epsilon, lstate, rstate);
      continue;
      // if we try to step by non-epsilon transitions when we have overlapping
      // backogs, we just end up duplicating the effort
    } else if(lempty && rempty &&
              a->isFinal(cur.left_state) && b->isFinal(cur.right_state)) {
      t->setFinal(cur.out_state);
    }
    std::vector<std::pair<state_t, Transition>> right_trans;
    for(auto rvect : right_transitions[cur.right_state]) {
      rstate = rvect.first;
      for(auto rtrans : rvect.second) {
        if((!lempty || isRightEpsilon(rtrans)) &&
           processTransitionPair(cur, left_epsilon, rtrans, lstate, rstate)) {
          continue;
          // see below for explanation of stepping by epsilon on the right
          // this is just the mirror of that
        }
        right_trans.push_back(std::make_pair(rstate, rtrans));
      }
    }
    for(auto lvect : left_transitions[cur.left_state]) {
      lstate = lvect.first;
      for(auto ltrans : lvect.second) {
        rstate = cur.right_state;
        if(!rempty || isLeftEpsilon(ltrans)) {
          if(processTransitionPair(cur, ltrans, right_epsilon, lstate, rstate)) {
            continue;
            // if stepping by epsilon on the right gets us somewhere
            // then stepping by the next transition will just add to the
            // backlog
            // if the left has multiple epsilons in a row on the composing
            // tape, then this results in symbols on non-composing tapes on
            // the right being output when the corresponding composing symbol
            // is output rather than creating duplicate paths for every
            // possible position
          }
        }
        for(auto it : right_trans) {
          processTransitionPair(cur, ltrans, it.second, lstate, it.first);
        }
      }
    }
  }
  return t;
}

Transducer*
compose(Transducer* a, Transducer* b, std::vector<std::pair<UnicodeString, UnicodeString>> tapes, bool flagsAsEpsilon)
{
  Composer comp(a, b, tapes, flagsAsEpsilon);
  return comp.compose();
}
