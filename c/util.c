#include "util.h"

long
file_size(FILE *fin)
{
  long currpos, length, tmp;
  currpos = ftell(fin);
  
  if (currpos < 0) {
    fprintf(stderr, "Couldn't get current position of open file.");
    goto error;
  }
  tmp = fseek(fin, 0L, SEEK_END);
  if (tmp < 0) {
    goto error;
  }
  length = ftell(fin);
  currpos = fseek(fin, 0L, SEEK_SET);
  if (currpos < 0) {
    goto error;
  }
  return length;

 error:
  fprintf(stderr, "Couldn't get the length of file\n");
  return -1;
}
