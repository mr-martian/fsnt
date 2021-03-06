#include "lib/transducer.h"
#include "lib/io.h"
#include "lib/reverse.h"
#include <unicode/ustdio.h>
#include <libgen.h>
#include <getopt.h>
#include <iostream>

using namespace std;

void endProgram(char *name)
{
  if(name != NULL)
  {
    cout << basename(name) << ": reverse a transducer" << endl;
    cout << "USAGE: " << basename(name) << " [transducer [output_file]]" << endl;
  }
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
  #if HAVE_GETOPT_LONG
  int option_index=0;
#endif

  while (true) {
#if HAVE_GETOPT_LONG
    static struct option long_options[] =
    {
      {"help",      no_argument, 0, 'h'},
      {0, 0, 0, 0}
    };

    int cnt=getopt_long(argc, argv, "h", long_options, &option_index);
#else
    int cnt=getopt(argc, argv, "h");
#endif
    if (cnt==-1)
      break;

    switch (cnt)
    {
      case 'h': // fallthrough
      default:
        endProgram(argv[0]);
        break;
    }
  }

  #include "tools/cli/get_io_fst2fst.cc"

  Transducer* t = readBin(input);

  Transducer* rev = reverse(t);

  writeBin(rev, output);

  if(input != stdin) {
    fclose(input);
  }
  if(output != stdout) {
    fclose(output);
  }
  delete t;
  delete rev;
  return 0;
}
