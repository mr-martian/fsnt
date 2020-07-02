/*
 * This file is based on
 * https://github.com/apertium/lexd/blob/master/src/lexdcompiler.h
 */
#include <set>

template<typename T>
bool subset(const set<T> &xs, const set<T> &ys)
{
  if(xs.size() > ys.size())
    return false;
  for(auto x: xs)
    if(ys.find(x) == ys.end())
      return false;
  return true;
}

template<typename T>
bool subset_strict(const set<T> &xs, const set<T> &ys)
{
  if(xs.size() >= ys.size())
    return false;
  return subset(xs, ys);
}

template<typename T>
set<T> unionset(const set<T> &xs, const set<T> &ys)
{
  set<T> u = xs;
  u.insert(ys.begin(), ys.end());
  return u;
}

template<typename T>
set<T> intersectset(const set<T> &xs, const set<T> &ys)
{
  set<T> i = xs;
  for(auto x: xs)
    if(ys.find(x) == ys.end())
      i.erase(x);
  return i;
}

template<typename T>
set<T> subtractset(const set<T> &xs, const set<T> &ys)
{
  set<T> i = xs;
  for(auto y: ys)
    i.erase(y);
  return i;
}
