#include "lib/transducer.h"
#include "lib/io.h"
#include <unicode/ustdio.h>
#include <libgen.h>
#include <getopt.h>
#include <iostream>
#include <stack>
#include <map>
#include <vector>

using namespace std;

void endProgram(char *name)
{
  if(name != NULL)
  {
    cout << basename(name) << ": print all paths in a transducer" << endl;
    cout << "USAGE: " << basename(name) << " [transducer [output_file]]" << endl;
  }
  exit(EXIT_FAILURE);
}

struct WalkerState {
  state_t state;
  vector<vector<string_ref>> paths;
};

void print(WalkerState& w, Transducer* t, UFILE* out)
{
  for(size_t i = 0; i < w.paths.size(); i++) {
    if(i != 0) {
      u_fprintf(out, ":");
    }
    for(auto sym : w.paths[i]) {
      t->getAlphabet().write_symbol(out, sym, false);
    }
  }
  u_fprintf(out, "\n");
}

void expand(Transducer* t, UFILE* out)
{
  stack<WalkerState> todo;
  WalkerState first;
  first.state = 0;
  first.paths.resize(t->getTapeCount());
  todo.push(first);
  auto trans = t->getTransitions();
  while(todo.size() > 0) {
    WalkerState cur = todo.top();
    todo.pop();
    if(t->isFinal(cur.state)) {
      print(cur, t, out);
    }
    for(auto it : trans[cur.state]) {
      for(auto tr : it.second) {
        WalkerState next = cur;
        next.state = it.first;
        for(size_t i = 0; i < next.paths.size(); i++) {
          next.paths[i].push_back(tr.symbols[i]);
        }
        todo.push(next);
      }
    }
  }
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

  #include "tools/cli/get_io_fst2txt.cc"

  Transducer* t = readBin(input);

  expand(t, output);

  if(input != stdin) {
    fclose(input);
  }
  u_fclose(output);
  delete t;
  return 0;
}
