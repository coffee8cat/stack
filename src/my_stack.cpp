#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "my_stack.h"
#include "stack_config.h"

stack_err stack_init(stack_t* stack, int init_capacity)
{
    if (init_capacity < 0)
    {
        FPRINTF_RED(stderr, "ERROR: Invalid value for stack capacity : [%d] < 0\n"
                            "INITIALISING EMPTY STACK\n", init_capacity);
        init_capacity = default_stack_size;
    }
    memset(&stack -> size, 0, ((char*)&stack -> right_canary - (char*)&stack -> size));

    stack -> capacity = init_capacity;
    if (init_capacity > 0)
    {
        size_t log_byte_capacity = UP_TO_EIGHT(init_capacity * sizeof(stack_elem_t));

        char* ptr = (char*)calloc(log_byte_capacity + 2 * sizeof(canary_t), sizeof(char));
        if (ptr == NULL)
        {
            stack -> err_stat = ALLOC_ERROR;
            return ALLOC_ERROR;
        }
        stack -> data = (stack_elem_t*)(ptr + sizeof(canary_t));

        #ifdef CANARY_PROTECT
            *(canary_t*)((char*)stack -> data - sizeof(canary_t))  =  canary_const;
            *(canary_t*)((char*)stack -> data + log_byte_capacity) =  canary_const;
        #endif
    }
    else
        stack -> data = NULL;

    #ifdef CANARY_PROTECT
        stack -> left_canary  =  canary_const;
        stack -> right_canary =  canary_const;
    #endif

    #ifdef HASH_PROTECT
    stack -> stack_hash_sum = calc_hash((char*)stack, (char*)&stack -> stack_hash_sum);
    #endif

    return OK;
}

stack_err stack_delete(stack_t* stack)
{
    if (stack == NULL)
    {
        FPRINTF_RED(stderr, "ERROR: INVALID POINTER(NULL) given to stack_delete\n");
        return PTR_ERROR;
    }

    if (stack -> data != NULL)
        free(stack -> data);

    memset(stack, 0, (size_t)(&stack -> right_canary - &stack -> left_canary) * sizeof(stack_elem_t) + sizeof(canary_t));
    return OK;
}

stack_err stack_dump(stack_t* stack, const char* call_file, size_t call_line, const char* call_func)
{
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
    printf("hash         [%llX] at (%p)\n", stack -> stack_hash_sum, &stack -> stack_hash_sum);
    printf("size:        [%ld]\n", stack -> size);
    printf("capacity:    [%ld]\n", stack -> capacity);
    printf(ANSI_COLOR_RESET);

    if (stack -> data == NULL)
    {
        if (stack -> capacity == 0)
        {
            PRINTF_CYAN("stack is not initialised\n");
            return OK;
        }
        else
        {
            FPRINTF_RED(stderr, "ERROR: INVALID POINTER(NULL) stack -> data, cannot print info about stack -> data\n");
            return PTR_ERROR;
        }
    }

    printf(ANSI_COLOR_CYAN);
    printf("stack data[%p]:\n{\n", stack -> data);
    printf("left  canary [%llX] at (%p)\n",
           *(canary_t*)((char*)stack -> data - sizeof(canary_t)), (char*)stack -> data - sizeof(canary_t));
    printf("right canary [%llX] at (%p)\n",
           *(canary_t*)((char*)stack -> data + UP_TO_EIGHT(stack -> capacity * (sizeof(stack_elem_t)))),
           (char*)stack -> data + UP_TO_EIGHT(stack -> capacity * sizeof(stack_elem_t)));
    printf("hash         [%lld] at (%p)\n", stack -> data_hash_sum, &stack -> data_hash_sum);

    for (int i = 0; i < stack -> capacity; i++)
        printf("%4d:[%lld][%llx]\n", i, stack -> data[i], stack -> data[i]);
    printf("}\n");
    printf(ANSI_COLOR_RESET);

    return OK;
}

stack_err stack_dump_errors(stack_t* stack)
{
    if (stack == NULL)
    {
        FPRINTF_RED(stderr, "INVALID POINTER(NULL) given to stack_dump_errors\n");
        return PTR_ERROR;
    }
    uint64_t errnum = stack -> err_stat;
    if (errnum & PTR_ERROR)
        FPRINTF_RED(stderr, "INVALID POINTER\n");
    if (errnum & ALLOC_ERROR)
        FPRINTF_RED(stderr, "ALLOCATION ERROR\n");
    if (errnum & SIZE_ERROR)
        FPRINTF_RED(stderr, "SIZE ERROR\n");
    if (errnum & STACK_HASH_ERROR)
        FPRINTF_RED(stderr, "STACK HASH ERROR\n");
    if (errnum & DATA_HASH_ERROR)
        FPRINTF_RED(stderr, "DATA HASH ERROR\n");
    if (errnum & LEFT_STACK_CANARY_DIED)
        FPRINTF_RED(stderr, "LEFT STACK CANARY DIED\n");
    if (errnum & RIGHT_STACK_CANARY_DIED)
        FPRINTF_RED(stderr, "RIGHT STACK CANARY DIED\n");
    if (errnum & LEFT_DATA_CANARY_DIED)
        FPRINTF_RED(stderr, "LEFT DATA CANARY DIED\n");
    if (errnum & RIGHT_DATA_CANARY_DIED)
        FPRINTF_RED(stderr, "RIGHT DATA CANARY DIED\n");

    return OK;
}

uint64_t stack_verify(stack_t* stack)
{
    if (stack == NULL)
        return PTR_ERROR;

    if (stack -> size > stack -> capacity)
        stack -> err_stat += SIZE_ERROR;

    #ifdef HASH_PROTECT
    uint64_t hash = 0;
    if (stack -> stack_hash_sum != calc_hash((char*)stack, (char*)&stack -> stack_hash_sum))
    {
        stack -> err_stat = stack -> err_stat | STACK_HASH_ERROR;
        return stack -> err_stat;
    }
    if (stack -> data_hash_sum != calc_hash((char*)stack -> data, (char*)stack -> data + UP_TO_EIGHT(stack -> capacity * sizeof(stack_elem_t))))
    {
        stack -> err_stat = stack -> err_stat | DATA_HASH_ERROR;
        return stack -> err_stat;
    }
    #endif

    #ifdef CANARY_PROTECT
    //CHECKING STACK CANARIES-----------------------------------------------------------------------------------------------------------------
    canary_t canary_value = (canary_t)canary_const;
    if (memcmp(&stack -> left_canary, &canary_value, sizeof(canary_t)) != 0)
        stack -> err_stat = stack -> err_stat | LEFT_STACK_CANARY_DIED;

    if (memcmp(&stack -> right_canary, &canary_value, sizeof(canary_t)) != 0)
        stack -> err_stat = stack -> err_stat | RIGHT_STACK_CANARY_DIED;

    //CHECKING DATA CANARIES-------------------------------------------------------------------------------------------------------------------
    if (stack -> data == NULL)
    {
        if (stack -> capacity == 0)
            return OK;
        else
            return PTR_ERROR;
    }

    canary_t data_canary_value = canary_const;
    size_t log_byte_capacity = UP_TO_EIGHT(stack -> capacity * sizeof(stack_elem_t));
    if (memcmp((char*)stack -> data - sizeof(canary_t), &data_canary_value, sizeof(canary_t)) != 0)
        stack -> err_stat = stack -> err_stat | LEFT_DATA_CANARY_DIED;

    if (memcmp((char*)stack -> data + log_byte_capacity, &data_canary_value, sizeof(canary_t)) != 0)
        stack -> err_stat = stack -> err_stat | RIGHT_DATA_CANARY_DIED;
    #endif

    return stack -> err_stat;
}

uint64_t calc_hash(char* start, char* end)
{
    if (start == NULL || end == NULL)
    {
        FPRINTF_RED(stderr, "ERROR: Invalid adress(NULL) came to %s in %s:%d\n", __func__, __FILE__, __LINE__);
        return OK;
    }
    uint64_t hash_sum = 0;
    char* curr = start;
    while (curr < end)
    {
        hash_sum += hash_sum * hash_coeff + (unsigned char)(*curr);
        curr++;
    }
    return hash_sum;
}

stack_err stack_realloc(stack_t* stack, stack_realloc_state state)
{
    if (stack_verify(stack) != OK)
    {
        STACK_DUMP(stack, __func__);
        stack_dump_errors(stack);
        return OK;
    }
    if (stack -> capacity == 0)
        return stack_init(stack, default_stack_size);

    size_t new_capacity = 0;
    if (state == INCREASE)
        new_capacity      = stack -> capacity * realloc_coeff;
    else
        new_capacity = stack -> capacity / (realloc_coeff * realloc_coeff);

    size_t log_byte_capacity = UP_TO_EIGHT(new_capacity * sizeof(stack_elem_t));

    char* ptr = (char*)realloc((char*)stack -> data - sizeof(canary_t), log_byte_capacity + 2 * sizeof(canary_t));
    if (ptr == NULL)
    {
        FPRINTF_RED(stderr, "Cannot reallocate memory for stack\n");
        stack -> err_stat = ALLOC_ERROR;
        #ifdef HASH_PROTECT
            stack -> stack_hash_sum = calc_hash((char*)stack, (char*)&stack -> stack_hash_sum);
        #endif
        return ALLOC_ERROR;
    }

    if (state == INCREASE)
        memset(ptr + sizeof(canary_t) + stack -> capacity * sizeof(stack_elem_t), 0, (new_capacity - stack -> capacity) * sizeof(stack_elem_t));

    stack -> data  = (stack_elem_t*)(ptr + sizeof(canary_t));
    stack -> capacity = new_capacity;

    #ifdef CANARY_PROTECT
        *(canary_t*)((char*)stack -> data - sizeof(canary_t))  =  canary_const;
        *(canary_t*)((char*)stack -> data + log_byte_capacity) =  canary_const;
    #endif

    #ifdef HASH_PROTECT
        stack -> data_hash_sum  = calc_hash((char*)stack -> data, (char*)stack -> data + UP_TO_EIGHT(stack -> capacity * sizeof(stack_elem_t)));
        stack -> stack_hash_sum = calc_hash((char*)stack, (char*)&stack -> stack_hash_sum);
    #endif

    return OK;
}

stack_err stack_push(stack_t* stack, stack_elem_t elem)
{
    if (stack_verify(stack) != OK)
    {
        STACK_DUMP(stack, __func__);
        stack_dump_errors(stack);
        return OK;
    }

    if (stack -> capacity <= stack -> size)
    {
        stack_realloc(stack, INCREASE);

        #ifdef HASH_PROTECT
            stack -> data_hash_sum  = calc_hash((char*)stack -> data, (char*)stack -> data + UP_TO_EIGHT(stack -> capacity * sizeof(stack_elem_t)));
            stack -> stack_hash_sum = calc_hash((char*)stack, (char*)&stack -> stack_hash_sum);
        #endif
    }

    stack -> data[stack -> size] = elem;
    (stack -> size)++;

    #ifdef HASH_PROTECT
        stack -> data_hash_sum  = calc_hash((char*)stack -> data, (char*)stack -> data + UP_TO_EIGHT(stack -> capacity * sizeof(stack_elem_t)));
        stack -> stack_hash_sum = calc_hash((char*)stack, (char*)&stack -> stack_hash_sum);
    #endif

    return OK;
}

stack_err stack_pop(stack_t* stack)
{
    if (stack_verify(stack) != OK)
    {
        STACK_DUMP(stack, __func__);
        stack_dump_errors(stack);
        return OK;
    }
    if (stack -> size > 0)
    {
        *(stack -> data + stack -> size - 1) = 0;
        stack -> size--;
    }
    else
    {
        printf("--------------------SIZE ERROR--------------------------\n");
        stack -> err_stat = SIZE_ERROR;
        #ifdef HASH_PROTECT
        stack -> stack_hash_sum = calc_hash((char*)stack, (char*)&stack -> stack_hash_sum);
        #endif
        return SIZE_ERROR;
    }
    #ifdef HASH_PROTECT
        stack -> data_hash_sum  = calc_hash((char*)stack -> data, (char*)stack -> data + UP_TO_EIGHT(stack -> capacity * sizeof(stack_elem_t)));
        stack -> stack_hash_sum = calc_hash((char*)stack, (char*)&stack -> stack_hash_sum);
    #endif

    if (stack -> size <= stack -> capacity / (realloc_coeff * realloc_coeff))
    {
        PRINTF_RED("REALLOC CALLED FROM stack_pop\n");
        stack_realloc(stack, DECREASE);
    }
    return OK;
}
