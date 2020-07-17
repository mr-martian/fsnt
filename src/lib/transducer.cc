#include "transducer.h"
#include "utils/compression.h"
#include <stdexcept>

Transducer::Transducer(size_t tp) :
  tapeCount(tp)
{
  addState();
}

Transducer::~Transducer()
{
}

StringInterner&
Transducer::getAlphabet()
{
  return alphabet;
}

SymbolTable&
Transducer::getSymbols()
{
  return symbols;
}

std::vector<std::map<state_t, std::vector<Transition>>>&
Transducer::getTransitions()
{
  return transitions;
}

std::map<state_t, double>&
Transducer::getFinals()
{
  return finals;
}

std::map<UnicodeString, TapeInfo>&
Transducer::getTapeInfo()
{
  return tapeNames;
}

void
Transducer::setTapeInfo(std::map<UnicodeString, TapeInfo> names)
{
  // TODO: is it more useful to have this be for adding multiple names
  // or for completely replacing the set of names?
  for(auto it : names) {
    setTapeInfo(it.first, it.second);
  }
}

void
Transducer::setTapeInfo(UnicodeString name, TapeInfo info)
{
  if(info.index >= tapeCount) {
    throw std::invalid_argument("TapeInfo refers to non-existent index");
  }
  tapeNames[name] = info;
}

size_t
Transducer::getTapeCount()
{
  return tapeCount;
}

size_t
Transducer::size()
{
  return transitions.size();
}

state_t
Transducer::addState()
{
  state_t ret = transitions.size();
  transitions.resize(ret + 1);
  return ret;
}

void
Transducer::addStates(size_t n)
{
  transitions.resize(transitions.size() + n);
}

bool
Transducer::isFinal(state_t state)
{
  return finals.find(state) != finals.end();
}

void
Transducer::setFinal(state_t state, double weight)
{
  finals[state] = weight;
}

void
Transducer::setNotFinal(state_t state)
{
  finals.erase(state);
}

void
Transducer::insertTransition(state_t src, state_t trg, Transition trans)
{
  if(trans.symbols.size() != tapeCount) {
    throw std::invalid_argument("Transition has wrong dimensions");
  }
  transitions[src][trg].push_back(trans);
  // TODO: Don't duplicate? maybe it should be a set rather than a vector?
}

state_t
Transducer::insertTransition(state_t src, Transition trans, bool checkExists)
{
  if(checkExists) {
    for(auto it : transitions[src]) {
      for(auto tr : it.second) {
        if(tr == trans) {
          return it.first;
        }
      }
    }
  }
  state_t trg = addState();
  insertTransition(src, trg, trans);
  return trg;
}

state_t
Transducer::insertEpsilonTransition(state_t src, bool checkExists, double weight)
{
  Transition tr;
  tr.symbols = std::vector<string_ref>(tapeCount, string_ref(0));
  tr.weight = weight;
  return insertTransition(src, tr, checkExists);
}

void
Transducer::insertEpsilonTransition(state_t src, state_t trg, double weight)
{
  Transition tr;
  tr.symbols = std::vector<string_ref>(tapeCount, string_ref(0));
  tr.weight = weight;
  insertTransition(src, trg, tr);
}
