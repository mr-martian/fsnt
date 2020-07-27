#include "transition_iter.h"

TransitionIterator::TransitionIterator(const std::map<state_t, std::vector<Transition>>& tr)
{
  transitions = &tr;
  mit = (*transitions).begin();
  vit = (*mit).second.begin();
  cur = std::make_pair((*mit).first, *vit);
}

TransitionIterator::TransitionIterator(const TransitionIterator& other)
{
  transitions = other.transitions;
  mit = other.mit;
  vit = other.vit;
  cur = other.cur;
}

TransitionIterator::~TransitionIterator()
{
}

const std::pair<state_t, Transition>&
TransitionIterator::operator*() const
{
  return cur;
}

TransitionIterator
TransitionIterator::operator++(int)
{
  TransitionIterator other = TransitionIterator(*this);
  other++;
  return other;
}

TransitionIterator&
TransitionIterator::operator++()
{
  vit++;
  if(vit == (*mit).second.end()) {
    mit++;
    vit = (*mit).second.begin();
  }
  cur = std::make_pair((*mit).first, *vit);
  return *this;
}

TransitionIterator
TransitionIterator::operator--(int)
{
  TransitionIterator other = TransitionIterator(*this);
  other--;
  return other;
}

TransitionIterator&
TransitionIterator::operator--()
{
  if(vit == (*mit).second.begin()) {
    mit--;
    vit = (*mit).second.end();
  }
  vit--;
  std::make_pair((*mit).first, *vit);
  return *this;
}

bool
TransitionIterator::operator==(TransitionIterator& other) const
{
  return (transitions == other.transitions &&
          mit == other.mit && vit == other.vit);
}

bool
TransitionIterator::operator!=(TransitionIterator& other) const
{
  return (transitions != other.transitions ||
          mit != other.mit || vit != other.vit);
}

TransitionIterator
TransitionIterator::begin()
{
  return TransitionIterator(*transitions);
}

TransitionIterator
TransitionIterator::end()
{
  TransitionIterator ret = TransitionIterator(*transitions);
  ret.mit = (*transitions).end();
  ret.vit = (*mit).second.end();
  ret.cur = std::make_pair((*mit).first, *vit);
  return ret;
}

TransitionIterator
iter_state(Transducer* t, state_t s)
{
  return TransitionIterator(t->getTransitions()[s]);
}
