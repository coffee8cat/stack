#ifndef _STACK_HEADER_H__
#define _STACK_HEADER_H__

#include <stdio.h>
#include <stdint.h>

#ifdef DEBUG

#define DEBUG_PRINTF(...) printf(__VA_ARGS__);

#else

#define DEBUG_PRINTF(...) ;

#endif

enum stack_realloc_state {INCREASE, DECREASE};
const double realloc_coeff = 2;

enum stack_err {
    OK,
    SIZE_ERROR,
    ALLOC_ERROR,
    HASH_ERROR,
    VERIF_ERROR
};

typedef int64_t stack_elem_t;

struct stack_t {
    stack_elem_t* data;
    size_t size;
    size_t capacity;
    stack_err err_stat;
    int hash_sum;
};

stack_err stack_init   (stack_t* stack, size_t init_size);
stack_err stack_delete (stack_t* stack);

stack_err stack_dump  (stack_t* stack);
stack_err stack_verify(stack_t* stack);

stack_err stack_realloc(stack_t* stack, stack_realloc_state state);

stack_err stack_push(stack_t* stack, stack_elem_t elem);
stack_err stack_pop (stack_t* stack);
#endif //_STACK_HEADER_H__
