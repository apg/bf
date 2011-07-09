// A simple (optimizing) brainfuck interpreter.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bf.h"

#define CODE_BUFFER_SIZE 1024

static bf_inst_t * 
locate_loop_skip_position(bf_context_t *bf)
{
  bf_inst_t *tmppc = bf->pc;
  int loops = 0;
  while (tmppc < (bf->code + bf->code_size)) {
    if (tmppc->operator == BF_OP_LOOPEND) {
      if (loops > 0) {
        loops--;
        continue;
      }
      loops = 0;
      printf("returning loop\n");
      return tmppc;
    }
    else if (tmppc->operator == BF_OP_LOOP) {
      loops++;
    }
    tmppc++;
  }
  return NULL;
}

int
bf_getchar()
{
  int tmp, c;
  c = getchar();

  switch (c) {
  case 13: // '\r\n' gets normalized to just '\n'
    tmp = getchar();
    if (tmp == 10) {
      return tmp;
    }
    ungetc(tmp, stdin);
    return c;
  case EOF:
    return 0;
  default:
    return c;
  }
}


bf_op_t
bf_op_from_ascii(unsigned char c)
{
  switch (c) {
  case BF_ASCII_OP_LEFT:
    return BF_OP_LEFT;
  case BF_ASCII_OP_RIGHT:
    return BF_OP_RIGHT;
  case BF_ASCII_OP_INCR:
    return BF_OP_INCR;
  case BF_ASCII_OP_DECR:
    return BF_OP_DECR;
  case BF_ASCII_OP_LOOP:
    return BF_OP_LOOP;
  case BF_ASCII_OP_LOOPEND:
    return BF_OP_LOOPEND;
  case BF_ASCII_OP_GET:
    return BF_OP_GET;
  case BF_ASCII_OP_PUT:
    return BF_OP_PUT;
  }
  return BF_OP_NOP; // dunno what that is.
}


int
bf_load(bf_context_t *bf, char *filename)
{
  long fsize;
  FILE *fin;
  bf_inst_t *code;
  long cp = 0;
  bf_op_t curop;

  fin = fopen(filename, "rb");
  if (!fin) {
    goto error;
  }

  fsize = file_size(fin);
  if (fsize < 0) {
    goto error;
  }
  
  code = malloc(sizeof(*code) * fsize);
  if (code == NULL) {
    goto error;
  }
  while (!feof(fin)) {
    curop = bf_op_from_ascii((unsigned char) fgetc(fin));
    if (curop != BF_OP_NOP) {
      code[cp].operator = curop;
      code[cp].count = 1;
      cp++;
    }
  }
  bf->code = realloc(code, cp); // resize code block
  bf->code = code;
  bf->code_size = cp;
  bf->pc = code;
  return 0;
 error:
  fprintf(stderr, "Couldn't load file");
  return -1;
}

int
bf_load_optimized(bf_context_t *bf, char *filename)
{
  long fsize;
  FILE *fin;
  bf_inst_t *code;
  long cp = 0;
  bf_op_t curop, lastop, tmpop;
  int opcount;


  fin = fopen(filename, "rb");
  if (!fin) {
    goto error;
  }

  fsize = file_size(fin);
  if (fsize < 0) {
    goto error;
  }
  
  code = malloc(sizeof(*code) * fsize);
  if (code == NULL) {
    goto error;
  }
  curop = bf_op_from_ascii((unsigned char) fgetc(fin));
  while (!feof(fin)) {
    if (curop == BF_OP_LOOP ||
        curop == BF_OP_LOOPEND ||
        curop == BF_OP_GET ||
        curop == BF_OP_PUT ||
        curop == BF_OP_NOP) {
      if (curop != BF_OP_NOP) {
        code[cp].operator = curop;
        code[cp].count = 1;
        cp++;
      }
      curop = bf_op_from_ascii((unsigned char) fgetc(fin));
    }
    else {
      opcount = 0;
      lastop = curop;
      do {
        tmpop = bf_op_from_ascii((unsigned char) fgetc(fin));
        opcount++;
      }
      while ((tmpop == lastop) && !feof(fin));

      code[cp].operator = lastop;
      code[cp].count = opcount;
      cp++;
      // set curop to whatever broke the loop
      curop = tmpop;
    }
  }

  bf->code = realloc(code, cp); // resize code block
  bf->code = code;
  bf->code_size = cp;
  bf->pc = code;
  return 0;
 error:
  fprintf(stderr, "Couldn't load file");
  return -1;
}

bf_context_t *
bf_make_context(int hsize, int lssize)
{
  bf_context_t *bf = malloc(sizeof(*bf));
  if (!bf) {
    fprintf(stderr, "Couldn't allocate a BF context: out of memory\n");
    goto error;
  }
  else {
    bf->heap = malloc(sizeof(*bf->heap) * hsize);
    if (!bf->heap) {
      goto error_heap;
    }
    else {
      bf->heap_size = hsize;
      bf->dp = bf->heap;
      memset(bf->heap, 0, hsize);

      bf->loop_stack = malloc(sizeof(*bf->loop_stack) * lssize);
      if (!bf->loop_stack) {
        goto error_loop;
      }
      bf->loop_stack_length = 0;
      bf->loop_stack_size = lssize;
    }
  }
  return bf;

 error_loop:
  free(bf->heap);
 error_heap:
  free(bf);
 error:
  return NULL;
}

void 
bf_context_destroy(bf_context_t *bf)
{
  if (bf != NULL) {
    if (bf->heap != NULL) {
      free(bf->heap);
    }
    if (bf->code != NULL) {
      free(bf->code);
    }
    if (bf->loop_stack != NULL) {
      free(bf->loop_stack);
    }
  }
}

int
bf_exec(bf_context_t *bf)
{
  bf_inst_t *tmppc;
  bf_inst_t *endpc = bf->pc + bf->code_size;
  int *enddp = bf->dp + bf->heap_size;
  int *newdp;
  while (bf->pc < endpc) {
    switch (bf->pc->operator) {
    case BF_OP_LEFT:
      newdp = bf->dp - bf->pc->count;
      if (newdp >= bf->heap) {
        bf->dp = newdp;
      }
      else {
        fprintf(stderr, "Data pointer is less than heap.\n");
        return -1;
      }

      break;
    case BF_OP_RIGHT:
      newdp = bf->dp + bf->pc->count;
      if (newdp < enddp) {
        bf->dp = newdp;
      }
      else {
        fprintf(stderr, "Data pointer is greater than heap.\n");
        return -1;
      }
      break;
    case BF_OP_INCR:
      *bf->dp += bf->pc->count;
      break;
    case BF_OP_DECR:
      *bf->dp -= bf->pc->count;
      break;
    case BF_OP_LOOP:
      if (bf->loop_stack_length < bf->loop_stack_size) {
        if (*bf->dp == 0) {
          // find the jump to location.
          tmppc = locate_loop_skip_position(bf);
          if (tmppc == NULL) {
            fprintf(stderr, "Unmatched loop begin\n");
            return -1;
          }
          bf->pc = tmppc;
        }
        else {
          bf->loop_stack[bf->loop_stack_length++] = bf->pc;
        }
      }
      else {
        fprintf(stderr, "Loop nesting limit exceeded\n");
        return -1;
      }
      break;
    case BF_OP_LOOPEND:
      if (bf->loop_stack_length > 0) {
        if (*bf->dp != 0) {
          bf->pc = bf->loop_stack[--bf->loop_stack_length];
          continue; // we want to skip the pc increment
        }
      }
      else {
        fprintf(stderr, "Loop close found, but there's no matching loop begin\n");
        return -1;
      }
      break;
    case BF_OP_GET:
      *bf->dp = bf_getchar();
      break;
    case BF_OP_PUT:
      putchar(*bf->dp);
      break;
    }
    bf->pc++;
  }
}
