#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "my_stack.h"
#include "stack_config.h"

const char* err_stats[] = {
    "OK",
    "SIZE_ERROR",
    "ALLOC_ERROR",
    "HASH_ERROR",
    "PTR_ERROR",
    "STACK_CANARY_DIED",
    "DATA_CANARY_DIED"
};


stack_err stack_init(stack_t* stack, size_t init_capacity)
{
    assert(stack);

    memset(stack, 0, ((char*)&stack -> right_canary - (char*)stack));
    stack -> capacity = init_capacity;
    stack -> size = 0;

    PRINTF_MAGENTA("INIT STACK\n");
    if (init_capacity > 0)
    {
        char* ptr = (char*)calloc(init_capacity * sizeof(stack_elem_t) + 2 * sizeof(uint64_t), sizeof(char));
        if (ptr == NULL)
        {
            stack -> err_stat = ALLOC_ERROR;
            return ALLOC_ERROR;
        }

        stack -> data = (stack_elem_t*)(ptr + sizeof(uint64_t));
        //data canaries
        #ifdef CANARY_PROTECT
            *(uint64_t*)((char*)stack -> data - sizeof(uint64_t)) =  canary_const;
            *(uint64_t*)(stack -> data + stack -> capacity)       =  canary_const;
            PRINTF_MAGENTA("data left canary  [%llx]\n", *(uint64_t*)((char*)stack -> data - sizeof(uint64_t)));
            PRINTF_MAGENTA("data right canary [%llx]\n", *(uint64_t*)(stack -> data + stack -> capacity));
        #endif

        PRINTF_MAGENTA("ptr[%p] data[%p]\n", ptr, stack -> data);
    }
    else
    {
        stack -> data = NULL;
    }

    #ifdef CANARY_PROTECT
        stack -> left_canary  =  canary_const;
        stack -> right_canary =  canary_const;
    #endif

    #ifdef HASH_PROTECT
    //hash must not include itself in calculation
    stack -> stack_hash_sum = calc_hash((char*)stack, (char*)&stack -> stack_hash_sum);
    #endif

    PRINTF_MAGENTA("Stack Initialised\n");
    STACK_DUMP(stack, __func__);
    CHECK_STACK(stack);
    return stack -> err_stat;
}

stack_err stack_delete(stack_t* stack)
{
    assert(stack);

    if (stack == NULL)
    {
        FPRINTF_RED(stderr, "ERROR: INVALID POINTER(NULL) given to stack_delete\n");
        return PTR_ERROR;
    }

    PRINTF_MAGENTA("Deleting stack\n"
                   "stack size = [%ld]\n",
                   (size_t)(&stack -> right_canary - &stack -> left_canary) * sizeof(stack_elem_t) + sizeof(uint64_t));
    memset(stack, 0, (size_t)(&stack -> right_canary - &stack -> left_canary) * sizeof(stack_elem_t) + sizeof(uint64_t));
    free(stack -> data);
/*
    stack -> data = NULL;
    stack -> size = 0;
    stack -> capacity = 0;
    stack -> err_stat = OK;
    stack -> stack_hash_sum = 0;
    stack -> data_hash_sum  = 0;
*/
    return stack -> err_stat;
}

stack_err stack_dump(stack_t* stack, const char* call_file, size_t call_line, const char* call_func)
{
    assert(stack);

    if (stack == NULL)
    {
        FPRINTF_RED(stderr, "ERROR: INVALID POINTER(NULL) stack, cannot print info about stack\n");
        return PTR_ERROR;
    }
    printf(ANSI_COLOR_BLUE);
    printf("stack[%p] created in %s:%ld\n", stack, stack -> file, stack -> line);
    printf("name [%s], called from %s:%d (%s)\n",  stack -> name, call_file, call_line, call_func);
    printf("left canary  [%llX] at (%p)\n",  stack -> left_canary,  &stack -> left_canary);
    printf("right canary [%llX] at (%p)\n",  stack -> right_canary, &stack -> right_canary);
    printf("hash         [%7lld] at (%p)\n", stack -> stack_hash_sum, &stack -> stack_hash_sum);
    printf("size:        [%lld]\n", stack -> size);
    printf("capacity:    [%lld]\n", stack -> capacity);
    printf("err_stat:    [%s]\n", err_stats[stack -> err_stat]);
    printf(ANSI_COLOR_RESET);
/*
    LOG_DUMP("stack[%p] created in %s:%d\n", stack, stack -> file, stack -> line);
    LOG_DUMP("name [%s], called from %s:%d (%s)\n",  stack -> name, call_file, call_line, call_func);
    LOG_DUMP("left canary  [%llx] at (%p)\n", stack -> left_canary,  &stack -> left_canary);
    LOG_DUMP("right canary [%llx] at (%p)\n", stack -> right_canary, &stack -> right_canary);
    LOG_DUMP("hash         [%lld]\n", stack -> stack_hash_sum);
    LOG_DUMP("size:        [%lld]\n", stack -> size);
    LOG_DUMP("capacity:    [%lld]\n", stack -> capacity);
    LOG_DUMP("err_stat:    [%s]\n", err_stats[stack -> err_stat]);
*/
    if (stack -> data == NULL)
    {
        if (stack -> capacity == 0)
        {
            PRINTF_CYAN("stack is not initialised\n");
            return stack -> err_stat;
        }
        else
        {
            FPRINTF_RED(stderr, "ERROR: INVALID POINTER(NULL) stack -> data, cannot print info about stack -> data\n");
            return PTR_ERROR;
        }
    }

    printf(ANSI_COLOR_CYAN);
    printf("stack data[%p]:\n{\n", stack -> data);
    printf("left  canary [%llX] at (%p)\n", *(stack -> data - 1), stack -> data - 1);
    printf("right canary [%llX] at (%p)\n", stack -> data[stack -> capacity], stack -> data + stack -> capacity);
    printf("hash         [%lld] at (%p)\n", stack -> data_hash_sum, &stack -> data_hash_sum);

    for (int i = 0; i <= stack -> capacity; i++)
        printf("%4d:[%lld][%llx]\n", i, stack -> data[i], stack -> data[i]);
    printf("}\n");
    printf(ANSI_COLOR_RESET);

    return stack -> err_stat;
}

stack_err stack_verify(stack_t* stack)
{
    assert(stack);

    if (stack == NULL || (stack -> data == NULL && stack -> capacity != 0))
    {
        FPRINTF_RED(stderr, "VERIFICATION FAILED\n");
        return PTR_ERROR;
    }
    if (stack -> size > stack -> capacity)
    {
        FPRINTF_RED(stderr, "VERIFICATION FAILED: size > capacity\n");
        stack -> err_stat = SIZE_ERROR;
        return SIZE_ERROR;
    }

    #ifdef CANARY_PROTECT
    //CHECKING STACK CANARIES-----------------------------------------------------------------------------------------------------------------
    uint64_t canary_value = (uint64_t)canary_const;
    if (memcmp(&stack -> left_canary, &canary_value, sizeof(uint64_t)) != 0)
    {
        FPRINTF_RED(stderr, "STACK LEFT CANARY DIED([%llx] != [%llx])\n", stack -> left_canary, canary_value);
        stack -> err_stat = STACK_CANARY_DIED;
        return STACK_CANARY_DIED;
    }

    if (memcmp(&stack -> right_canary, &canary_value, sizeof(uint64_t)) != 0)
    {
        FPRINTF_RED(stderr, "STACK RIGHT CANARY DIED([%llx] != [%llx])\n", stack -> right_canary, canary_value);
        stack -> err_stat = STACK_CANARY_DIED;
        return STACK_CANARY_DIED;
    }

    //CHECKING DATA CANARIES-------------------------------------------------------------------------------------------------------------------
    if (stack -> data == NULL)
    {
        printf("stack is not initialised\n");
        return stack -> err_stat;
    }

    uint64_t data_canary_value = canary_const;
    if (memcmp(&stack -> data[-1], &data_canary_value, sizeof(uint64_t)) != 0)
    {
        FPRINTF_RED(stderr, "DATA LEFT CANARY DIED([%llx] != [%llx])\n", stack -> data[-1], data_canary_value);
        stack -> err_stat = DATA_CANARY_DIED;
        return DATA_CANARY_DIED;
    }
    if (memcmp(&stack -> data[stack -> capacity], &data_canary_value, sizeof(uint64_t)) != 0)
    {
        FPRINTF_RED(stderr, "DATA RIGHT CANARY DIED([%llx] != [%llx])\n", stack -> data[stack -> capacity], data_canary_value);
        stack -> err_stat = DATA_CANARY_DIED;
        return DATA_CANARY_DIED;
    }
    #endif

    #ifdef HASH_PROTECT
    uint64_t hash = 0;
    DEBUG_PRINTF("stack -> hash sum[%p]\n");
    if (stack -> stack_hash_sum != (hash = calc_hash((char*)stack, (char*)&stack -> stack_hash_sum)))
    {
        FPRINTF_RED(stderr, "UNDETERMINED STACK HASH CHANGE [%lld] != [%lld]\n", stack -> stack_hash_sum, hash);
        return HASH_ERROR;
    }
    #endif

    return stack -> err_stat;
}

inline void stack_check(stack_t* stack, const char* file_name, size_t line, const char* func)
{
    if (stack_verify(stack) != OK || stack -> err_stat != OK)
    {
        STACK_DUMP(stack, func);
        assert(0);
    }
}
/*
uint64_t DJB_hash(char* start, char* end)
{
    if (start == NULL || end == NULL)
    {
        FPRINTF_RED(stderr, "ERROR: Invalid adress(NULL) came to %s in %s:%d\n", __func__, __FILE__, __LINE__);
        return 0;
    }

    uint64_t hash = 5381;
    char* curr = start;
    while (curr <= end)
    {
        hash = ((hash << 5) + hash) + (unsigned char)(*curr);
    }

    return hash;
}
*/
uint64_t calc_hash(char* start, char* end)
{
    if (start == NULL || end == NULL)
    {
        FPRINTF_RED(stderr, "ERROR: Invalid adress(NULL) came to %s in %s:%d\n", __func__, __FILE__, __LINE__);
        return 0;
    }
    DEBUG_PRINTF("HASH CALCULATION START\n");
    DEBUG_PRINTF("start [%p]\n", start);
    DEBUG_PRINTF("end   [%p]\n", end);
    uint64_t p = hash_coeff;
    uint64_t hash_sum = 0;
    char* curr = start;
    while (curr < end)
    {
        hash_sum += p * (unsigned char)(*curr);
        p = p * hash_coeff;
        //DEBUG_PRINTF("hash sum = [%lld]\n", hash_sum);
        //DEBUG_PRINTF("p        = [%lld]\n", p);
        //DEBUG_PRINTF("pointer  = [%lX]\n", curr);
        curr++;
    }

    return hash_sum;
}

stack_err stack_realloc(stack_t* stack, stack_realloc_state state)
{
    assert(stack);

    CHECK_STACK(stack);

    if (stack -> capacity == 0)
    {
        PRINTF_YELLOW("Allocation of non-initialised stack\n");
        stack_init(stack, 4);
        return stack -> err_stat;
    }
    size_t new_capacity = 0;
    switch(state)
    {
        case INCREASE: new_capacity = stack -> capacity * realloc_coeff;
                       break;
        case DECREASE: new_capacity = stack -> capacity / (realloc_coeff * realloc_coeff);
                       break;
        default: break;
    }

    PRINTF_YELLOW("ptr to reallocate [%p]\n", (char*)stack -> data - sizeof(uint64_t));
    char* ptr = (char*)realloc((char*)stack -> data - sizeof(uint64_t), new_capacity * sizeof(stack_elem_t) + 2 * sizeof(uint64_t));
    if (ptr == NULL)
    {
        FPRINTF_RED(stderr, "Cannot reallocate memory for stack\n");
        CHECK_STACK(stack);
        stack -> err_stat = ALLOC_ERROR;
        return ALLOC_ERROR;
    }
    else
    {
        PRINTF_YELLOW("ptr start:    %p\n", ptr);
        PRINTF_YELLOW("data start:   %p\n", ptr + sizeof(uint64_t));
        PRINTF_YELLOW("ptr new area: %p\n", ptr + sizeof(uint64_t) + sizeof(stack_elem_t) * stack -> capacity);
        PRINTF_YELLOW("stack cap:    %ld\n", stack -> capacity);
        PRINTF_YELLOW("new cap:      %ld\n", new_capacity);
        PRINTF_YELLOW("offset - [%ld]\n",   (new_capacity - stack -> capacity) * sizeof(stack_elem_t) + sizeof(uint64_t));

        if (state == INCREASE)
            memset(ptr + sizeof(uint64_t) + stack -> capacity * sizeof(stack_elem_t), 0, (new_capacity - stack -> capacity) * sizeof(stack_elem_t));

        stack -> data  = (stack_elem_t*)(ptr + sizeof(uint64_t));
        stack -> capacity = new_capacity;

        #ifdef CANARY_PROTECT
            stack -> data[-1]           =  canary_const;
            stack -> data[new_capacity] =  canary_const;
            PRINTF_YELLOW("data left canary  [%llx] at (%p)\n", stack -> data[-1], stack -> data - 1);
            PRINTF_YELLOW("data right canary [%llx] at (%p)\n", stack -> data[new_capacity], &stack -> data[new_capacity]);
        #endif
    }
    #ifdef HASH_PROTECT
    //hash must not include itself in calculation
    stack -> stack_hash_sum = calc_hash((char*)stack, (char*)&stack -> stack_hash_sum);
    #endif

    STACK_DUMP(stack, __func__);
    PRINTF_YELLOW("Reallocation completed\n");

    CHECK_STACK(stack);
    return stack -> err_stat;
}

stack_err stack_push(stack_t* stack, stack_elem_t elem)
{
    assert(stack);

    PRINTF_RED("-----PUSH START-----\n");
    CHECK_STACK(stack);
    if (stack -> capacity <= stack -> size)
    {
        DEBUG_PRINTF("Reallocation call\n"
                     "size:     %d\n"
                     "capacity: %d\n", stack -> size, stack -> capacity);
        stack_realloc(stack, INCREASE);
    }

    #ifdef HASH_PROTECT
        //hash must not include itself in calculation
        stack -> stack_hash_sum = calc_hash((char*)stack, (char*)&stack -> stack_hash_sum);
    #endif

    CHECK_STACK(stack);

    stack -> data[stack -> size] = elem;
    (stack -> size)++;
    STACK_DUMP(stack, __func__);

    #ifdef HASH_PROTECT
        //hash must not include itself in calculation
        stack -> stack_hash_sum = calc_hash((char*)stack, (char*)&stack -> stack_hash_sum);
    #endif

    CHECK_STACK(stack);
    PRINTF_RED("-----STACK PUSH END-----\n");
    return stack -> err_stat;
}

stack_err stack_pop(stack_t* stack)
{
    assert(stack);
    PRINTF_RED("-----STACK POP-----\n");
    CHECK_STACK(stack);
    if (stack -> size > 0)
    {
        *(stack -> data + stack -> size - 1) = 0;
        stack -> size--;
    }
    else
    {
        stack -> err_stat = SIZE_ERROR;
        return SIZE_ERROR;
    }

    #ifdef HASH_PROTECT
        //hash must not include itself in calculation
        stack -> stack_hash_sum = calc_hash((char*)stack, (char*)&stack -> stack_hash_sum);
    #endif

    printf("size           = %ld\n", stack -> size);
    printf("capacity       = %ld\n", stack -> capacity);
    printf("realloc border = %ld\n", stack -> capacity / (realloc_coeff * realloc_coeff));
    if (stack -> size <= stack -> capacity / (realloc_coeff * realloc_coeff))
    {
        PRINTF_RED("REALLOC CALLED FROM stack_pop\n");
        stack_realloc(stack, DECREASE);
    }

    #ifdef HASH_PROTECT
        //hash must not include itself in calculation
        stack -> stack_hash_sum = calc_hash((char*)stack, (char*)&stack -> stack_hash_sum);
    #endif

    CHECK_STACK(stack);
    STACK_DUMP(stack, __func__);
    PRINTF_RED("-----STACK POP END-----\n");
    return stack -> err_stat;
}
