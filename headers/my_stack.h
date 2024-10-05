#ifndef _STACK_HEADER_H__
#define _STACK_HEADER_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "stack_config.h"

#ifdef DEBUG

#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#define DEBUG_PRINTF_BLUE(str, ...) PRINTF_BLUE(str, __VA_ARGS__)

#else

#define DEBUG_PRINTF(...) ;
#define DEBUG_PRINTF_BLUE(str, ...) ;

#endif

#ifdef CANARY_PROTECTION

#define ON_CANARY(...) __VA_ARGS__

#endif

#ifdef HASH_PROTECTION

#define ON_HASH(...) __VA_ARGS__

#endif

#define INIT(var) 0, #var, __FILE__, __LINE__
#define canary_const 0xD15EA5E

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define PRINTF_RED(str, ...)     printf(ANSI_COLOR_RED     str ANSI_COLOR_RESET, #__VA_ARGS__);
#define PRINTF_GREEN(str, ...)   printf(ANSI_COLOR_GREEN   str ANSI_COLOR_RESET, #__VA_ARGS__);
#define PRINTF_YELLOW(str, ...)  printf(ANSI_COLOR_YELLOW  str ANSI_COLOR_RESET, #__VA_ARGS__);
#define PRINTF_BLUE(str,...)     printf(ANSI_COLOR_BLUE    str ANSI_COLOR_RESET, #__VA_ARGS__);
#define PRINTF_MAGENTA(str, ...) printf(ANSI_COLOR_MAGENTA str ANSI_COLOR_RESET, #__VA_ARGS__);
#define PRINTF_CYAN(str,...)     printf(ANSI_COLOR_CYAN    str ANSI_COLOR_RESET, #__VA_ARGS__);

#define FPRINTF_RED(file,str,...)   fprintf(file, ANSI_COLOR_RED   str ANSI_COLOR_RESET, ##__VA_ARGS__);
#define FPRINTF_GREEN(file,str,...) fprintf(file, ANSI_COLOR_GREEN str ANSI_COLOR_RESET, ##__VA_ARGS__);

enum stack_realloc_state {INCREASE, DECREASE};
const size_t realloc_coeff = 2;
const uint64_t hash_coeff = 33;
const size_t default_stack_size = 8;

enum stack_err {
    OK          = 0,
    PTR_ERROR   = 1 << 0,
    ALLOC_ERROR = 1 << 1,
    SIZE_ERROR  = 1 << 2,
    STACK_HASH_ERROR  = 1 << 3,
    DATA_HASH_ERROR   = 1 << 4,
    LEFT_STACK_CANARY_DIED  = 1 << 5,
    RIGHT_STACK_CANARY_DIED = 1 << 6,
    LEFT_DATA_CANARY_DIED   = 1 << 7,
    RIGHT_DATA_CANARY_DIED  = 1 << 8
};

typedef uint64_t stack_elem_t;
typedef uint64_t canary_t;

struct stack_t {
    #ifdef CANARY_PROTECTION
    uint64_t left_canary;
    #endif

    #ifdef DEBUG
    const char* name;
    const char* file;
    size_t line;
    #endif

    size_t size;
    size_t capacity;
    size_t elem_size;
    uint64_t err_stat;
    void* data;

    #ifdef HASH_PROTECTION
    uint64_t data_hash_sum;
    uint64_t stack_hash_sum;
    #endif

    #ifdef CANARY_PROTECTION
    uint64_t right_canary;
    #endif
};

#define UP_TO_EIGHT(x) (x) + (8 - (x) % 8) % 8

#define STACK_DUMP(stack, func) stack_dump(stack, __FILE__, __LINE__, func, &dump_char)

stack_err stack_init   (stack_t* stack, size_t init_size, size_t elem_size);
stack_err stack_delete (stack_t* stack);

stack_err stack_dump(stack_t* stack, const char* call_file, size_t call_line, const char* call_func, int (* dump_func)(size_t, stack_t*));
stack_err stack_dump_errors(stack_t* stack);

uint64_t stack_verify(stack_t* stack);
static uint64_t calc_hash(char* start, char* end);

static stack_err stack_realloc(stack_t* stack, stack_realloc_state state);

stack_err stack_push(stack_t* stack, stack_elem_t elem);
stack_err stack_pop (stack_t* stack, void* temp);

static int dump_char(size_t i, stack_t* stack)
{
    char temp = *((char*)stack -> data + i * stack -> elem_size);
    return printf("%4d:[%c](%d)\n", i, temp, temp);
}
static int dump_int(size_t i, stack_t* stack)
{
    int temp = 0;
    memcpy(&temp, (char*)stack -> data + i * stack -> elem_size, stack -> elem_size);
    return printf("%4d:[%d]\n", i, temp);
}
static int dump_double(size_t i, stack_t* stack)
{
    double temp = 0;
    memcpy(&temp, (char*)stack -> data + i * stack -> elem_size, stack -> elem_size);
    return printf("%4d:[%lf]\n", i, temp);
}
static int dump_uint64_t(size_t i, stack_t* stack)
{
    uint64_t temp = 0;
    memcpy(&temp, (char*)stack -> data + i * stack -> elem_size, stack -> elem_size);
    return printf("%4d:[%llX]\n", i, temp);
}
#endif //_STACK_HEADER_H__
