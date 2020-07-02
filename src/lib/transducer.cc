#include "transducer.h"
#include "utils/compression.h"
#include <stdexcept>

Transducer::Transducer(size_t tp, size_t ftp) :
  tapes(tp), flagTapes(ftp)
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

std::vector<std::map<state_number_t, std::vector<Transition>>>&
Transducer::getTransitions()
{
  return transitions;
}

std::map<state_number_t, double>&
Transducer::getFinals()
{
  return finals;
}

size_t
Transducer::getTapes()
{
  return tapes;
}

size_t
Transducer::getFlagTapes()
{
  return flagTapes;
}

size_t
Transducer::size()
{
  return transitions.size();
}

state_number_t
Transducer::addState()
{
  state_number_t ret = transitions.size();
  transitions.resize(ret + 1);
  return ret;
}

void
Transducer::addStates(size_t n)
{
  transitions.resize(transitions.size() + n);
}

bool
Transducer::isFinal(state_number_t state)
{
  return finals.find(state) != finals.end();
}

void
Transducer::setFinal(state_number_t state, double weight)
{
  finals[state] = weight;
}

void
Transducer::setNotFinal(state_number_t state)
{
  finals.erase(state);
}

void
Transducer::insertTransition(state_number_t src, state_number_t trg, Transition trans)
{
  if(trans.symbols.size() != tapes || trans.flags.size() != flagTapes) {
    throw std::invalid_argument("Transitions has wrong dimensions");
  }
  transitions[src][trg].push_back(trans);
  // TODO: Don't duplicate? maybe it should be a set rather than a vector?
}

state_number_t
Transducer::insertTransition(state_number_t src, Transition trans)
{
  state_number_t trg = addState();
  insertTransition(src, trg, trans);
  return trg;
}
