#include "lib/transducer.h"
#include "lib/io.h"
#include "lib/compose.h"
#include <unicode/ustdio.h>
#include <libgen.h>
#include <getopt.h>
#include <iostream>

using namespace std;

void endProgram(char *name)
{
  if(name != NULL)
  {
    cout << basename(name) << ": compose 2 transducers" << endl;
    cout << "USAGE: " << basename(name) << " transducer transducer (-g tape tape)* [output_file]" << endl;
  }
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
  vector<pair<UnicodeString, UnicodeString>> glue;

  #if HAVE_GETOPT_LONG
  int option_index=0;
#endif

  while (true) {
#if HAVE_GETOPT_LONG
    static struct option long_options[] =
    {
      {"glue",      required_argument, 0, 'g'},
      {"help",      no_argument,       0, 'h'},
      {0, 0, 0, 0}
    };

    int cnt=getopt_long(argc, argv, "g:h", long_options, &option_index);
#else
    int cnt=getopt(argc, argv, "g:h");
#endif
    if (cnt==-1)
      break;

    switch (cnt)
    {
      case 'g':
      {
        UnicodeString l = argv[optind-1];
        optind++;
        if(optind > argc) {
          cout << "Missing second tape name" << endl;
          exit(EXIT_FAILURE);
        }
        UnicodeString r = argv[optind-1];
        glue.push_back(make_pair(l, r));
      }
        break;

      case 'h': // fallthrough
      default:
        endProgram(argv[0]);
        break;
    }
  }

  #include "tools/cli/get_io_2fsts.cc"

  Transducer* t1 = readBin(input1);
  Transducer* t2 = readBin(input2);

  Transducer* t = compose(t1, t2, glue, false);

  writeBin(t, output);

  if(input1 != stdin) {
    fclose(input1);
  }
  if(input2 != stdin) {
    fclose(input2);
  }
  if(output != stdout) {
    fclose(output);
  }
  delete t1;
  delete t2;
  delete t;
  return 0;
}
