#ifndef _STACK_HEADER_H__
#define _STACK_HEADER_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include "stack_config.h"

#ifdef DEBUG

#define DEBUG_PRINTF(...) printf(__VA_ARGS__);
#define DEBUG_PRINTF_CYAN(str, ...) printf_cyan(str, __VA_ARGS__);

#else

#define DEBUG_PRINTF(...) ;
#define DEBUG_PRINTF_CYAN(str, ...) ;

#endif

#define INIT(var) 0, #var, __FILE__, __LINE__
#define canary_const 0xD15EA5E

#define ANSI_COLOR_RED   "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_CYAN  "\e[46m"
#define ANSI_COLOR_RESET "\x1b[0m"

#define printf_red(str, ...)   printf(ANSI_COLOR_RED   str ANSI_COLOR_RESET, __VA_ARGS__);
#define printf_green(str, ...) printf(ANSI_COLOR_GREEN str ANSI_COLOR_RESET, __VA_ARGS__);
#define printf_cyan(str,...)   printf(ANSI_COLOR_CYAN   str ANSI_COLOR_RESET, __VA_ARGS__);

#define fprintf_red(file,str,...)   fprintf(file, ANSI_COLOR_RED   str ANSI_COLOR_RESET, ##__VA_ARGS__);
#define fprintf_green(file,str,...) fprintf(file, ANSI_COLOR_GREEN str ANSI_COLOR_RESET, ##__VA_ARGS__);

enum stack_realloc_state {INCREASE, DECREASE};
const double realloc_coeff = 2;

enum stack_err {
    OK,
    SIZE_ERROR,
    ALLOC_ERROR,
    HASH_ERROR,
    PTR_ERROR,
    STACK_CANARY_DIED,
    DATA_CANARY_DIED
};

typedef int64_t stack_elem_t;

struct stack_t {
    uint64_t left_canary;
    const char* name;
    const char* file;
    size_t line;
    stack_elem_t* data;
    size_t size;
    size_t capacity;
    stack_err err_stat;
    int hash_sum;
    uint64_t right_canary;
};

#define STACK_DUMP(stack, func) stack_dump(stack, __FILE__, __LINE__, func)

stack_err stack_init   (stack_t* stack, size_t init_size);
stack_err stack_delete (stack_t* stack);

stack_err stack_dump(stack_t* stack, const char* call_file, size_t call_line, const char* call_func);
stack_err stack_verify(stack_t* stack);
inline void stack_check(stack_t* stack, const char* file_name, size_t line, const char* func);

stack_err stack_realloc(stack_t* stack, stack_realloc_state state);

stack_err stack_push(stack_t* stack, stack_elem_t elem);
stack_err stack_pop (stack_t* stack);

#ifndef CHECK_STACK
#define CHECK_STACK(stack) stack_check(stack, __FILE__, __LINE__, __func__);

#endif
#endif //_STACK_HEADER_H__
