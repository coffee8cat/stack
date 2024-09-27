#include "my_stack.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>


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

    DEBUG_PRINTF("INIT STACK\n");
    char* ptr = (char*)calloc(init_capacity * sizeof(stack_elem_t) + 2 * sizeof(uint64_t), sizeof(char));
    if (ptr == NULL)
    {
        stack -> err_stat = ALLOC_ERROR;
        return ALLOC_ERROR;
    }
    stack -> left_canary  = (uint64_t)stack ^ canary_const;
    stack -> right_canary = (uint64_t)stack ^ canary_const;



    stack -> data = (stack_elem_t*)(ptr + sizeof(uint64_t));
    *(uint64_t*)((char*)stack -> data - sizeof(uint64_t)) = (uint64_t)ptr ^ canary_const;
    *(uint64_t*)(stack -> data + stack -> capacity) = (uint64_t)ptr ^ canary_const;
    DEBUG_PRINTF("data canary [%x]\n", *(uint64_t*)ptr);
    DEBUG_PRINTF("ptr[%p] data[%p]\n", ptr, stack -> data);

    stack -> capacity = init_capacity;
    stack -> size = 0;

    STACK_DUMP(stack, __func__);
    CHECK_STACK(stack);

    DEBUG_PRINTF("Stack Initialised Succesfully\n");
    return stack -> err_stat;
}

stack_err stack_delete(stack_t* stack)
{
    assert(stack);

    memset(stack, 0, sizeof(stack));
    DEBUG_PRINTF("Deleting stack\n"
                 "stack size = [%d]\n", sizeof(stack));

    free(stack -> data);
    stack -> data = NULL;
    stack -> size = 0;
    stack -> capacity = 0;
    stack -> err_stat = OK;
    stack -> hash_sum = 0;


    return stack -> err_stat;
}

stack_err stack_dump(stack_t* stack, const char* call_file, size_t call_line, const char* call_func)
{
    assert(stack);

    if (stack == NULL)
    {
        fprintf_red(stderr, "ERROR: INVALID POINTER(NULL) stack, cannot print info about stack\n");
        return PTR_ERROR;
    }
    fprintf_red(stdout,
        "\nstack[%p] created in %s:%d\n"
        "name [%s], called from %s:%d (%s)\n"
        "left canary  [%x]\n"
        "right canary [%x]\n"
        "size:        [%x]\n"
        "capacity:    [%x]\n"
        "err_stat:    [%s]\n",
        stack, stack -> file, stack -> line,
        stack -> name, call_file, call_line, call_func,
        stack -> left_canary, stack -> right_canary,
        stack -> size, stack -> capacity, err_stats[stack -> err_stat]);

    printf_green("stack data[%p]:\n"
                "{\n", stack -> data);
    if (stack -> data == NULL)
    {
        fprintf_red(stderr, "ERROR: INVALID POINTER(NULL) stack -> data, cannot print info about stack -> data\n");
        return PTR_ERROR;
    }
    printf_green("left  canary [%x]\n", *(uint64_t*)((char*)stack -> data - sizeof(uint64_t)));
    printf_green("right canary [%x]\n", *(uint64_t*)(stack -> data + stack -> capacity));
    for (size_t i = 0; i < stack -> capacity; i++)
        printf_green("%4d:[%10d][%x]\n", i, stack -> data[i], stack -> data[i]);
    printf_green("}\n");

    return stack -> err_stat;
}

stack_err stack_verify(stack_t* stack)
{
    assert(stack);

    if (stack == NULL || stack -> data == NULL)
    {
        fprintf_red(stderr, "VERIFICATION FAILED\n");
        return PTR_ERROR;
    }
    if (stack -> size > stack -> capacity)
    {
        fprintf_red(stderr, "VERIFICATION FAILED: size > capacity\n");
        stack -> err_stat = SIZE_ERROR;
        return SIZE_ERROR;
    }
    uint64_t canary_value = (uint64_t)stack ^ canary_const;
    if (memcmp(&stack -> left_canary, &canary_value, sizeof(uint64_t)) != 0)
    {
        fprintf_red(stderr, "STACK LEFT CANARY DIED([%x] != [%x])\n", stack -> left_canary, canary_value);
        stack -> err_stat = STACK_CANARY_DIED;
        return STACK_CANARY_DIED;
    }

    if (memcmp(&stack -> right_canary, &canary_value, sizeof(uint64_t)) != 0)
    {
        fprintf_red(stderr, "STACK RIGHT CANARY DIED([%x] != [%x])\n", stack -> right_canary, (uint64_t)stack ^ canary_const);
        stack -> err_stat = STACK_CANARY_DIED;
        return STACK_CANARY_DIED;
    }

    uint64_t data_canary_value = *(uint64_t*)((char*)stack -> data - sizeof(uint64_t));
    if (memcmp((uint64_t*)((char*)stack -> data - sizeof(uint64_t)), &data_canary_value, sizeof(uint64_t)) != 0)
    {
        fprintf_red(stderr, "DATA LEFT CANARY DIED([%x] != [%x])\n", *(uint64_t*)((char*)stack -> data - sizeof(uint64_t)), data_canary_value);
        stack -> err_stat = DATA_CANARY_DIED;
        return DATA_CANARY_DIED;
    }

    if (memcmp((uint64_t*)(stack -> data + stack -> capacity), &data_canary_value, sizeof(uint64_t) != 0))
    {
        fprintf_red(stderr, "DATA RIGHT CANARY DIED([%x] != [%x])\n", *(uint64_t*)(stack -> data + stack -> capacity), data_canary_value);
        stack -> err_stat = DATA_CANARY_DIED;
        return DATA_CANARY_DIED;
    }

    return stack -> err_stat;
}

inline void stack_check(stack_t* stack, const char* file_name, size_t line, const char* func)
{
    if (stack_verify(stack) != OK)
    {
        STACK_DUMP(stack, func);
        assert(0);
    }
}

stack_err stack_realloc(stack_t* stack, stack_realloc_state state)
{
    assert(stack);

    CHECK_STACK(stack);

    size_t new_capacity = 0;
    switch(state)
    {
        case INCREASE: new_capacity = stack -> capacity * realloc_coeff;
                       break;
        case DECREASE: new_capacity = stack -> capacity / (realloc_coeff * realloc_coeff);
                       break;
        default: break;
    }

    char* ptr = (char*)calloc(new_capacity * sizeof(stack_elem_t) + 2 * sizeof(uint64_t), sizeof(char));
    if (ptr == NULL)
    {
        fprintf_red(stderr, "Cannot reallocate memory for stack\n");
        return ALLOC_ERROR;
    }
    else
    {
        DEBUG_PRINTF_CYAN("ptr start:    %p\n"
                     "ptr new area: %p\n"
                     "stack cap:    %d\n"
                     "new cap:      %d\n"
                     "count for memset - [%d]\n",
                     ptr, ptr + sizeof(stack_elem_t) * stack -> capacity, stack -> capacity, new_capacity,
                     (new_capacity - stack -> capacity) * sizeof(stack_elem_t));
        memcpy(ptr, stack -> data, stack -> capacity * sizeof(stack_elem_t));
        free(stack -> data);
        //memset(ptr + stack -> capacity, 0, (new_capacity - stack -> capacity) * sizeof(stack_elem_t));

        stack -> data  = (stack_elem_t*)(ptr + sizeof(uint64_t));
        stack -> capacity = new_capacity;

        *(uint64_t*)ptr                 = (uint64_t)stack -> data ^ canary_const;
        *(stack -> data + new_capacity) = (uint64_t)stack -> data ^ canary_const;
        free(ptr);
    }
    STACK_DUMP(stack, __func__);
    DEBUG_PRINTF("Reallocation completed\n");
    return stack -> err_stat;
}

stack_err stack_push(stack_t* stack, stack_elem_t elem)
{
    assert(stack);

    DEBUG_PRINTF("PUSH START\n");
    CHECK_STACK(stack);
    if (stack -> capacity <= stack -> size)
    {
        DEBUG_PRINTF("Reallocation call\n"
                     "size:     %d\n"
                     "capacity: %d\n", stack -> size, stack -> capacity);
        stack_realloc(stack, INCREASE);
    }

    if (stack_verify(stack) != OK)
    {
        fprintf_red(stderr, "STACK PUSH FAILED\n");
        return stack -> err_stat;
    }

    stack -> data[stack -> size] = elem;
    (stack -> size)++;
    STACK_DUMP(stack, __func__);

    DEBUG_PRINTF("Stack_push Executed\n");
    return stack -> err_stat;
}

stack_err stack_pop(stack_t* stack)
{
    assert(stack);
    CHECK_STACK(stack);
    if (stack -> size > 0)
    {
        *(stack -> data + stack -> size) = 0;
        (stack -> size)--;
    }
    else
        if (stack -> capacity > 0)
            stack -> err_stat = SIZE_ERROR;

    return stack -> err_stat;
}
