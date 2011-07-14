#ifndef _BF_H_
#define _BF_H_

// how much loop nesting can we do?
#define BF_LOOP_STACK_SIZE 1024

// standard heap size is 30000 cells.
#define BF_HEAP_SIZE 30000

#define BF_ASCII_OP_LEFT '<'
#define BF_ASCII_OP_RIGHT '>'
#define BF_ASCII_OP_INCR '+'
#define BF_ASCII_OP_DECR '-'
#define BF_ASCII_OP_LOOP '['
#define BF_ASCII_OP_LOOPEND ']'
#define BF_ASCII_OP_GET ','
#define BF_ASCII_OP_PUT '.'

typedef enum {
  BF_OP_NOP = 0x00,
  BF_OP_LEFT = 0x01,
  BF_OP_RIGHT,
  BF_OP_INCR,
  BF_OP_DECR,
  BF_OP_LOOP,
  BF_OP_LOOPEND,
  BF_OP_GET,
  BF_OP_PUT
} bf_op_t;

typedef struct bf_inst_t {
  bf_op_t operator;
  int count;
} bf_inst_t;

typedef struct bf_context_t {
  bf_inst_t *code; 
  bf_inst_t *pc; 
  int code_size; 
  int *heap;
  int heap_size;
  int *dp;
} bf_context_t;

bf_op_t bf_op_from_ascii(unsigned char);
int bf_load(bf_context_t *, char *filename);
int bf_load_optimized(bf_context_t *, char *filename);
bf_context_t *bf_make_context(int hsize, int lssize);
void bf_context_destroy(bf_context_t *);
int bf_exec(bf_context_t *);

#endif
