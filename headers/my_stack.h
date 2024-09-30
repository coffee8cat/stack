#ifndef _STACK_HEADER_H__
#define _STACK_HEADER_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include "stack_config.h"

#define DEBUG
#ifdef DEBUG

#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#define DEBUG_PRINTF_CYAN(str, ...) printf_cyan(str, __VA_ARGS__)

#else

#define DEBUG_PRINTF(...) ;
#define DEBUG_PRINTF_CYAN(str, ...) ;

#endif

#define INIT(var) 0, #var, __FILE__, __LINE__
#define canary_const 0xD15EA5E

#define ANSI_COLOR_RED   "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_CYAN  "\x1b[34m"
#define ANSI_COLOR_RESET "\x1b[0m"

#define printf_red(str, ...)   printf(ANSI_COLOR_RED   str ANSI_COLOR_RESET, #__VA_ARGS__);
#define printf_green(str, ...) printf(ANSI_COLOR_GREEN str ANSI_COLOR_RESET, #__VA_ARGS__);
#define printf_cyan(str,...)   printf(ANSI_COLOR_CYAN   str ANSI_COLOR_RESET, #__VA_ARGS__);

#define fprintf_red(file,str,...)   fprintf(file, ANSI_COLOR_RED   str ANSI_COLOR_RESET, ##__VA_ARGS__);
#define fprintf_green(file,str,...) fprintf(file, ANSI_COLOR_GREEN str ANSI_COLOR_RESET, ##__VA_ARGS__);
/*
const char* log_file_name = "log_stack.txt";
FILE* log_thread = fopen(log_file_name, "w");
#define LOG_DUMP(...) fprintf(log_thread, ##__VA_ARGS__)
*/
enum stack_realloc_state {INCREASE, DECREASE};
const size_t realloc_coeff = 2;
const uint64_t hash_coeff = 1;

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
    size_t size;
    size_t capacity;
    stack_err err_stat;
    stack_elem_t* data;
    uint64_t data_hash_sum;
    uint64_t stack_hash_sum;
    uint64_t right_canary;
};

#define STACK_DUMP(stack, func) stack_dump(stack, __FILE__, __LINE__, func)
#define CHECK_STACK(stack) stack_check(stack, __FILE__, __LINE__, __func__)

stack_err stack_init   (stack_t* stack, size_t init_size);
stack_err stack_delete (stack_t* stack);

stack_err stack_dump(stack_t* stack, const char* call_file, size_t call_line, const char* call_func);
stack_err stack_verify(stack_t* stack);
inline void stack_check(stack_t* stack, const char* file_name, size_t line, const char* func);

//uint64_t DJB_hash(char* start, char* end);
uint64_t calc_hash(char* start, char* end);

stack_err stack_realloc(stack_t* stack, stack_realloc_state state);

stack_err stack_push(stack_t* stack, stack_elem_t elem);
stack_err stack_pop (stack_t* stack);

#endif //_STACK_HEADER_H__
