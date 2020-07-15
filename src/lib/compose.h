#ifndef _LIB_COMPOSE_H_
#define _LIB_COMPOSE_H_

#include "transducer.h"
#include <vector>
#include <unicode/unistr.h>

Transducer* compose(Transducer* a, Transducer* b, std::vector<std::pair<UnicodeString, UnicodeString>> tapes, bool flagsAsEpsilon = true);

#endif
