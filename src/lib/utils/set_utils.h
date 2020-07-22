/*
 * This file is based on
 * https://github.com/apertium/lexd/blob/master/src/lexdcompiler.h
 */

#ifndef _LIB_SET_UTILS_H_
#define _LIB_SET_UTILS_H_

#include <set>

template<typename T>
bool subset(const std::set<T> &xs, const std::set<T> &ys)
{
  if(xs.size() > ys.size())
    return false;
  for(auto x: xs)
    if(ys.find(x) == ys.end())
      return false;
  return true;
}

template<typename T>
bool subset_strict(const std::set<T> &xs, const std::set<T> &ys)
{
  if(xs.size() >= ys.size())
    return false;
  return subset(xs, ys);
}

template<typename T>
std::set<T> unionset(const std::set<T> &xs, const std::set<T> &ys)
{
  std::set<T> u = xs;
  u.insert(ys.begin(), ys.end());
  return u;
}

template<typename T>
std::set<T> intersectset(const std::set<T> &xs, const std::set<T> &ys)
{
  std::set<T> i = xs;
  for(auto x: xs)
    if(ys.find(x) == ys.end())
      i.erase(x);
  return i;
}

template<typename T>
std::set<T> subtractset(const std::set<T> &xs, const std::set<T> &ys)
{
  std::set<T> i = xs;
  for(auto y: ys)
    i.erase(y);
  return i;
}

#endif
