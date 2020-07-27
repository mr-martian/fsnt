#ifndef _LIB_RELABEL_H_
#define _LIB_RELABEL_H_

#include "transducer.h"
#include <map>

Transducer* relabel(Transducer* t, std::map<string_ref, string_ref>& update);

#endif
