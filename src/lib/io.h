#ifndef _LIB_IO_H_
#define _LIB_IO_H_

#include <cstdio>
#include <unicode/ustdio.h>
#include "transducer.h"

Transducer* readBin(FILE* in);
void writeBin(Transducer* t, FILE* out);

Transducer* readATT(UFILE* in, bool flagsSeparate = false);
void writeATT(Transducer* t, UFILE* out, bool writeWeights = true);

#endif
