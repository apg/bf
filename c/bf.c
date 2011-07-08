// A simple brainfuck interpreter.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

// how much loop nesting can we do?
#define LOOP_STACK_SIZE 32
// standard heap size is 30000 cells.
#define HEAP_SIZE 30000

struct BF {
  char *prog; 
  char *pc; 
  int prog_size; 
  char *heap; // cells are bytes
  int heap_size;
  char *dp;
  char **loopstack; // jump pointers for looping
  int loopstack_length;
};

int
load_program(struct BF *bf, char *filename)
{
  struct stat buf;
  int fd, left, rd;
  char *pr;

  fd = open(filename, O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "Couldn't open file: %s\n", filename);
    return -1;
  }

  if (!fstat(fd, &buf)) {
    bf->prog = malloc(1 + (sizeof(*bf->prog) * buf.st_size));
    bf->prog_size = buf.st_size;
    left = buf.st_size;
    if (!bf->prog) {
      return -1;
    }
  }
  else {
    fprintf(stderr, "Error stating file\n");
    return -1;
  }

  // read the program.
  pr = bf->prog;
  do {
    rd = read(fd, pr, left);
    if (rd > 0) {
      left -= rd;
      pr += rd;
    }
    else if (rd == 0) {
      left = 0;
    }
    else {
      fprintf(stderr, "Error reading file\n");
      return -1;
    }
  }
  while (left > 0);
  bf->prog[buf.st_size] = 0;
  return 0;
}


struct BF *
init(long hsize, char *filename)
{
  int i;
  struct BF *bf = malloc(sizeof(*bf));
  if (!bf) {
    goto error;
  }
  else {
    // allocate heap
    bf->heap = malloc(sizeof(*bf->heap) * hsize);
    if (!bf->heap) {
      goto errorheap;
    }
    else {
      bf->heap_size = hsize;
      memset(bf->heap, 0, hsize);
      // allocate nested loop stack
      bf->loopstack = malloc(sizeof(*bf->loopstack) * LOOP_STACK_SIZE);
      if (!bf->loopstack) {
        goto errorloop;
      }
      bf->loopstack_length = 0;
    }
  }

  if (load_program(bf, filename) < 0) {
    goto errorcode;
  }
  else {
    bf->pc = bf->prog;
    bf->dp = bf->heap;
    goto success;
  }

 errorcode:
  free(bf->loopstack);
 errorloop:
  free(bf->heap);
 errorheap:
  free(bf);
  bf = 0;
 error:
  fprintf(stderr, "Couldn't allocate enough memory\n");
 success:
  return bf;
}


int
exec(struct BF *bf)
{
  int opens = 0;
  char tmpchr;
  char *tpc;
  char *pe = bf->pc + bf->prog_size;
  int *t;

  while (bf->pc < pe) {
    switch (*bf->pc) {
    case '>':
      bf->dp++;
      break;
    case'<':
      bf->dp--;
      break;
    case '+':
      *bf->dp += 1;
      break;
    case '-':
      *bf->dp -= 1;
      break;
    case '.':
      putchar(*bf->dp);
      break;
    case ',':
      tmpchr = (char)getchar();
      switch (tmpchr) {
      case -1: 
        tmpchr = 0; // normalize EOF to 0
        break;
      case 13:
        tmpchr = 10; // normalize \r to \n
        break;
      }
      *bf->dp = tmpchr;
      break;
    case '[':
      if (bf->loopstack_length < LOOP_STACK_SIZE) {
        if (*bf->dp == 0) {
          // find the jump to place (after the loop)
          tpc = bf->pc + 1;
          while (*tpc != '\0') {
            if (*tpc == ']') {
              if (opens > 0) {
                opens--;
              }
              else {
                opens = 0;
                bf->pc = tpc;
                break;
              }
            }
            else if (*tpc == '[') {
              opens++;
            }
            ++tpc;
          }

          if (*tpc == '\0') {
            fprintf(stderr, "EOF reached while searching for a loop point. bombing out\n");
            exit(1);
          }
        }
        else {
          // push the loop onto the stack so we can get back to it.
          bf->loopstack[bf->loopstack_length++] = bf->pc;
        }
      }
      else {
        fprintf(stderr, "too many nested loops!\n");
        exit(1);
      }
      break;
    case ']':
      if (bf->loopstack_length > 0) {
        if (*bf->dp != 0) {
          bf->pc = bf->loopstack[--bf->loopstack_length];
          continue; // skip the increment
        }
      }
      else {
        fprintf(stderr, "No loop top to return to! %d loops pushed\n", bf->loopstack_length);
      }
      break;
    }

    ++bf->pc;
  }

  return 0;
}

int
main(int argc, char **argv)
{
  struct BF *bf;
  if (argc < 2) {
    fprintf(stderr, "fooey: give me a bf file\n");
    exit(1);
  }

  bf = init(HEAP_SIZE, argv[1]);
  return exec(bf);
}
