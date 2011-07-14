// A simple (optimizing) brainfuck interpreter.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bf.h"

#define CODE_BUFFER_SIZE 1024

static void
dump_code_offsets(bf_context_t *bf)
{
  int i;
  for (i = 0; i < bf->code_size; i++) {
    printf("%p = %d\n", bf->code + i, bf->code[i].operator);
  }
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
  default:
    return BF_OP_NOP;
  }
}


int
bf_load(bf_context_t *bf, char *filename)
{
  int loop_stack[BF_LOOP_STACK_SIZE];
  int opcount, tmp, loop_stack_length;
  long fsize;
  FILE *fin;
  bf_inst_t *code;
  long cp;
  bf_op_t curop;

  cp = 0;
  loop_stack_length = 0;

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

  memset(code, 0, fsize);

  while (!feof(fin)) {
    curop = bf_op_from_ascii((unsigned char) fgetc(fin));
    opcount = 1;
    if (curop != BF_OP_NOP) {
      if (loop_stack_length < BF_LOOP_STACK_SIZE) {
        if (curop == BF_OP_LOOP) {
          loop_stack[loop_stack_length++] = cp;
          opcount = -1;
        }
        else if (curop == BF_OP_LOOPEND) {
          tmp = loop_stack[--loop_stack_length];
          opcount = cp - tmp;
          code[tmp].count = opcount - 1;
        }
      }
      else {
        fprintf(stderr, "Too many nested loops.\n");
        return -1;
      }
      code[cp].operator = curop;
      code[cp].count = opcount;
      cp++;
    }
  }

  bf->code = realloc(code, (cp + 1) * sizeof(*code)); // resize code block
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
  int loop_stack[BF_LOOP_STACK_SIZE];
  int opcount, tmp, loop_stack_length;
  long fsize;
  FILE *fin;
  bf_inst_t *code;
  long cp;
  bf_op_t curop, lastop, tmpop;

  cp = 0;
  loop_stack_length = 0;

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
        if (curop == BF_OP_LOOP) {
          loop_stack[loop_stack_length++] = cp;
        }
        else if (curop == BF_OP_LOOPEND) {
          tmp = loop_stack[--loop_stack_length];
          opcount = cp - tmp;
          code[tmp].count = opcount - 1;
        }

        code[cp].operator = curop;
        code[cp].count = opcount;
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

  bf->code = realloc(code, (cp + 1) * sizeof(*code)); // resize code block
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
    }
  }
  return bf;

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
  }
}

int
bf_exec(bf_context_t *bf)
{
  bf_inst_t *tmppc;
  bf_inst_t *endpc = bf->pc + bf->code_size;
  int *enddp = bf->dp + bf->heap_size;
  int *newdp;

#if DEBUG
  dump_code_offsets(bf);
#endif

  while (bf->pc < endpc) {
    switch (bf->pc->operator) {
    case BF_OP_LEFT:
#if DEBUG
      printf("Instr '<'. DP = %p (value: %d), PC = %p (heap bottom: %p)\n", \
             bf->dp, *bf->dp, bf->pc, bf->heap);
#endif
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
#if DEBUG
      printf("Instr '>'. DP = %p (value: %d), PC = %p (heap bottom: %p)\n", \
             bf->dp, *bf->dp, bf->pc, bf->heap);
#endif
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
#if DEBUG
      printf("Instr '+'. DP = %p (value: %d), PC = %p (heap bottom: %p)\n", \
             bf->dp, *bf->dp, bf->pc, bf->heap);
#endif

      *bf->dp += bf->pc->count;
      break;
    case BF_OP_DECR:
#if DEBUG
      printf("Instr '-'. DP = %p (value: %d), PC = %p (heap bottom: %p)\n", \
             bf->dp, *bf->dp, bf->pc, bf->heap);
#endif

      *bf->dp -= bf->pc->count;
      break;
    case BF_OP_LOOP:
      if (*bf->dp == 0) {
        bf->pc += bf->pc->count;
      }
      break;
    case BF_OP_LOOPEND:
      if (*bf->dp != 0) {
        bf->pc -= bf->pc->count;
      }
      break;
    case BF_OP_GET:
#if DEBUG
      printf("Getting a char\n");
#endif
      *bf->dp = bf_getchar();
      break;
    case BF_OP_PUT:
#if DEBUG
      printf("putting a char\n");
#endif

      putchar(*bf->dp);
      break;
    }
    bf->pc++;
  }
}
