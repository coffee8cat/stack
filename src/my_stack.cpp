#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "my_stack.h"
#include "stack_config.h"

stack_err stack_init(stack_t* stack, size_t init_capacity, size_t elem_size)
{
    memset(&stack -> size, 0, ((char*)&stack -> data + sizeof(stack -> data) - (char*)&stack -> size));

    stack -> capacity  = init_capacity;
    stack -> elem_size = elem_size;
    if (init_capacity > 0)
    {
        size_t aligned_byte_capacity = UP_TO_EIGHT(init_capacity * elem_size);

        char* ptr = (char*)calloc(aligned_byte_capacity ON_CANARY(+ 2 * sizeof(canary_t)), sizeof(char));
        if (ptr == NULL)
        {
            stack -> err_stat = stack -> err_stat | ALLOC_ERROR;
            return ALLOC_ERROR;
        }
        stack -> data = ptr ON_CANARY(+ sizeof(canary_t));

        #ifdef CANARY_PROTECTION
            *(canary_t*)((char*)stack -> data - sizeof(canary_t))       =  canary_const;
            *(canary_t*)((char*)stack -> data + aligned_byte_capacity) =  canary_const;
        #endif

        #ifdef HASH_PROTECTION
            stack -> data_hash_sum  = calc_hash((char*)stack -> data, (char*)stack -> data + UP_TO_EIGHT(stack -> capacity * stack -> elem_size));
        #endif
    }
    else
        stack -> data = NULL;

    #ifdef CANARY_PROTECTION
        stack -> left_canary  =  canary_const;
        stack -> right_canary =  canary_const;
    #endif

    #ifdef HASH_PROTECTION
    stack -> stack_hash_sum = calc_hash((char*)stack, (char*)&stack -> stack_hash_sum);
    #endif

    STACK_DUMP(stack, __func__);
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
        free((char*)stack -> data ON_CANARY(- sizeof(canary_t)));

    memset(stack, 0, (size_t)((char*)&stack -> data - (char*)stack)
                      ON_HASH(+ 2 * sizeof(stack-> stack_hash_sum)) ON_CANARY(+ sizeof(canary_t)));
    return OK;
}

stack_err stack_dump(stack_t* stack, const char* call_file, size_t call_line, const char* call_func, int (* dump_func)(size_t, stack_t*))
{
    if (stack == NULL)
    {
        FPRINTF_RED(stderr, "ERROR: INVALID POINTER(NULL) stack, cannot print info about stack\n");
        return PTR_ERROR;
    }
    printf(ANSI_COLOR_BLUE);
    printf("stack[%p] created in %s:%d\n", (void*)stack, stack -> file, stack -> line);
    printf("name [%s], called from %s:%d (%s)\n",  stack -> name, call_file, call_line, call_func);
    printf("left canary  [%llX] at (%p)\n",  stack -> left_canary,  (void*)&stack -> left_canary);
    printf("right canary [%llX] at (%p)\n",  stack -> right_canary, (void*)&stack -> right_canary);
    printf("hash         [%llX] at (%p)\n",  stack -> stack_hash_sum, (void*)&stack -> stack_hash_sum);
    printf("size:        [%d]\n", stack -> size);
    printf("capacity:    [%d]\n", stack -> capacity);
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
    #ifdef CANARY_PROTECTION
    printf("left  canary [%llX] at (%p)\n",
           *(canary_t*)((char*)stack -> data - sizeof(canary_t)), (void*)((char*)stack -> data - sizeof(canary_t)));
    printf("right canary [%llX] at (%p)\n",
           *(canary_t*)((char*)stack -> data + UP_TO_EIGHT(stack -> capacity * (stack -> elem_size))),
           (void*)((char*)stack -> data + UP_TO_EIGHT(stack -> capacity * stack -> elem_size)));
    #endif

    #ifdef HASH_PROTECTION
    printf("hash         [%llX] at (%p)\n", stack -> data_hash_sum, (void*)&stack -> data_hash_sum);
    #endif

    for (size_t i = 0; i < stack -> capacity; i++)
    {
        dump_func(i, stack);
    }
    printf("}\n");
    printf(ANSI_COLOR_RESET);

    return OK;
}


const char* errors[9] = {
    "PTR_ERROR",
    "ALLOC_ERROR",
    "SIZE_ERROR",
    "STACK_HASH_ERROR",
    "DATA_HASH_ERROR",
    "LEFT_STACK_CANARY_DIED",
    "RIGHT_STACK_CANARY_DIED",
    "LEFT_DATA_CANARY_DIED",
    "RIGHT_DATA_CANARY_DIED"
};

stack_err stack_dump_errors(stack_t* stack)
{
    if (stack == NULL)
    {
        FPRINTF_RED(stderr, "INVALID POINTER(NULL) given to stack_dump_errors\n");
        return PTR_ERROR;
    }
    uint64_t errnum = stack -> err_stat;
    for (size_t i = 0; i < 9; i++)
    {
        if (errnum & (1 << i))
        {
            FPRINTF_RED(stderr, "%s\n", errors[i]);
        }
    }

    return OK;
}

uint64_t stack_verify(stack_t* stack)
{
    if (stack == NULL)
        return PTR_ERROR;

    if (stack -> size > stack -> capacity)
        stack -> err_stat |= SIZE_ERROR;

    #ifdef HASH_PROTECTION
    if (stack -> stack_hash_sum != calc_hash((char*)stack, (char*)&stack -> stack_hash_sum))
    {
        stack -> err_stat = stack -> err_stat | STACK_HASH_ERROR;
        return stack -> err_stat;
    }
    #endif

    #ifdef CANARY_PROTECTION
    //CHECKING STACK CANARIES-----------------------------------------------------------------------------------------------------------------
    canary_t canary_value = (canary_t)canary_const;
    if (memcmp(&stack -> left_canary, &canary_value, sizeof(canary_t)) != 0)
        stack -> err_stat = stack -> err_stat | LEFT_STACK_CANARY_DIED;

    if (memcmp(&stack -> right_canary, &canary_value, sizeof(canary_t)) != 0)
        stack -> err_stat = stack -> err_stat | RIGHT_STACK_CANARY_DIED;
    #endif
    //CHECKING DATA CANARIES-------------------------------------------------------------------------------------------------------------------
    if (stack -> data == NULL)
    {
        if (stack -> capacity == 0)
            return OK;
        else
            return PTR_ERROR;
    }

    #ifdef HASH_PROTECTION
    if (stack -> data_hash_sum != calc_hash((char*)stack -> data, (char*)stack -> data + UP_TO_EIGHT(stack -> capacity * stack -> elem_size)))
    {
        stack -> err_stat = stack -> err_stat | DATA_HASH_ERROR;
        return stack -> err_stat;
    }
    #endif

    #ifdef CANARY_PROTECTION
    canary_t data_canary_value = canary_const;
    size_t aligned_byte_capacity = UP_TO_EIGHT(stack -> capacity * stack -> elem_size);

    if (memcmp((char*)stack -> data - sizeof(canary_t), &data_canary_value, sizeof(canary_t)) != 0)
        stack -> err_stat = stack -> err_stat | LEFT_DATA_CANARY_DIED;

    if (memcmp((char*)stack -> data + aligned_byte_capacity, &data_canary_value, sizeof(canary_t)) != 0)
        stack -> err_stat = stack -> err_stat | RIGHT_DATA_CANARY_DIED;
    #endif

    return stack -> err_stat;
}

static uint64_t calc_hash(char* start, char* end)
{
    if (start == NULL || end == NULL)
    {
        FPRINTF_RED(stderr, "ERROR: Invalid adress(NULL) came to %s in %s:%d\n", __func__, __FILE__, __LINE__);
        return OK;
    }
    uint64_t hash_sum = 5381;
    char* curr = start;
    while (curr < end)
    {
        hash_sum += hash_sum * hash_coeff + (unsigned char)(*curr);
        curr++;
    }
    return hash_sum;
}

static stack_err stack_realloc(stack_t* stack, stack_realloc_state state)
{
    if (stack -> capacity == 0)
        return stack_init(stack, default_stack_size, stack -> elem_size);

    size_t realloc_capacity = 0;
    if (state == INCREASE)
        realloc_capacity = stack -> capacity * realloc_coeff;
    else
        realloc_capacity = stack -> capacity / realloc_coeff;

    size_t aligned_byte_capacity = UP_TO_EIGHT(realloc_capacity * stack -> elem_size);

    char* ptr = (char*)realloc((char*)stack -> data ON_CANARY(- sizeof(canary_t)),
                               aligned_byte_capacity ON_CANARY(+ 2 * sizeof(canary_t)));
    if (ptr == NULL)
    {
        FPRINTF_RED(stderr, "Cannot reallocate memory for stack\n");
        stack -> err_stat = ALLOC_ERROR;
        return ALLOC_ERROR;
    }

    stack -> capacity = realloc_capacity;
    memset(ptr ON_CANARY(+ sizeof(canary_t)) + stack -> size * stack -> elem_size, 0,
           (realloc_capacity - stack -> size) * stack -> elem_size);

    stack -> data = ptr ON_CANARY(+ sizeof(canary_t));

    #ifdef CANARY_PROTECTION
        *(canary_t*)((char*)stack -> data - sizeof(canary_t))      =  canary_const;
        *(canary_t*)((char*)stack -> data + aligned_byte_capacity) =  canary_const;
    #endif

    #ifdef HASH_PROTECTION
        stack -> data_hash_sum  = calc_hash((char*)stack -> data, (char*)stack -> data + UP_TO_EIGHT(stack -> capacity * stack -> elem_size));
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
        stack_realloc(stack, INCREASE);

    memcpy((char*)stack -> data + stack -> size * stack -> elem_size, &elem, stack -> elem_size);
    (stack -> size)++;

    #ifdef HASH_PROTECTION
        stack -> data_hash_sum  = calc_hash((char*)stack -> data, (char*)stack -> data + UP_TO_EIGHT(stack -> capacity * stack -> elem_size));
        stack -> stack_hash_sum = calc_hash((char*)stack, (char*)&stack -> stack_hash_sum);
    #endif

    return OK;
}

stack_err stack_pop(stack_t* stack, void* temp)
{
    if (stack_verify(stack) != OK)
    {
        //STACK_DUMP(stack, __func__);
        stack_dump_errors(stack);
        return OK;
    }

    if (temp == NULL)
    {
        FPRINTF_RED(stderr, "INVALID POINTER(NULL) given to stack_pop\n");
        return PTR_ERROR;
    }
    if (stack -> size > 0)
    {
        memcpy(temp, (char*)stack -> data + (stack -> size - 1) * stack -> elem_size,  stack -> elem_size);
        memset((char*)stack -> data + (stack -> size - 1) * stack -> elem_size, 0, stack -> elem_size);
        stack -> size--;
    }
    else
    {
        stack -> err_stat = SIZE_ERROR;
        #ifdef HASH_PROTECTION
        stack -> stack_hash_sum = calc_hash((char*)stack, (char*)&stack -> stack_hash_sum);
        #endif
        return SIZE_ERROR;
    }
    #ifdef HASH_PROTECTION
        stack -> data_hash_sum  = calc_hash((char*)stack -> data, (char*)stack -> data + UP_TO_EIGHT(stack -> capacity * stack -> elem_size));
        stack -> stack_hash_sum = calc_hash((char*)stack, (char*)&stack -> stack_hash_sum);
    #endif

    if (stack -> size <= stack -> capacity / (realloc_coeff * realloc_coeff))
        stack_realloc(stack, DECREASE);

    return OK;
}
