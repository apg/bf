#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include "bf.h"

void
usage(char *cmdname)
{
  fprintf(stderr, "usage: %s [-o] <filename>\n", cmdname);
}

int
main(int argc, char **argv) 
{
  int oflag, c, i;
  int heapsize;
  int loopstack;
  char *filename;
  
  bf_context_t *bf;

  while ((c = getopt(argc, argv, "oH:l:")) != -1) {
    switch (c) {
    case 'o':
      oflag = 1;
      break;
    case '?':
      if (isprint (optopt)) {
        fprintf (stderr, "Unknown option `-%c'.\n", optopt);
      }
      else {
        fprintf (stderr,
                 "Unknown option character `\\x%x'.\n",
                 optopt);
      }
      usage(argv[0]);
      return 1;
    default:
      return 1;
    }
  }

  if (optind < argc) {
    filename = argv[optind];
  }
  else {
    fprintf(stderr, "Required filename not found.\n");
    usage(argv[0]);
    return 1;
  }

  bf = bf_make_context(BF_HEAP_SIZE, BF_LOOP_STACK_SIZE);
  if (!bf) {
    fprintf(stderr, "Failed to allocate enough memory to run.\n");
    return 1;
  }

  if (oflag) {
    if (bf_load_optimized(bf, filename) < 0) {
      return 1;
    }
  }
  else {
    if (bf_load(bf, filename) < 0) {
      return 1;
    }
  }
  i = bf_exec(bf);
  bf_context_destroy(bf);

  return i;
}
