#include "my_stack.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

stack_err stack_init(stack_t* stack, size_t init_capacity)
{
    assert(stack);

    stack -> data = (stack_elem_t*)calloc(init_capacity, sizeof(stack_elem_t));
    if (stack -> data == NULL)
    {
        stack -> err_stat = ALLOC_ERROR;
        return ALLOC_ERROR;
    }
    stack -> capacity = init_capacity;
    stack -> size = 0;

    DEBUG_PRINTF("Stack Initialised Succesfully\n");
    //stack_verify(stack);
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

stack_err stack_dump(stack_t* stack)
{
    assert(stack);

    //stack_verify();
    printf("\n--- STACK_INFO ---\n"
           "size:     [%d]\n"
           "capacity: [%d]\n"
           "err_stat: ", stack -> size, stack -> capacity);

    switch (stack -> err_stat)
    {
        case OK: printf("OK\n");
                 break;
        case ALLOC_ERROR: printf("ALLOC_ERROR\n");
                          break;
        case SIZE_ERROR:  printf("SIZE_ERROR\n");
                          break;
        case HASH_ERROR:  printf("HASH_ERROR\n");
                          break;
        default: printf("???default???\n");
                 break;
    }

    printf("stack data:\n");
    for (size_t i = 0; i < stack -> capacity; i++)
        printf("%3d:[%d]\n", i, stack -> data[i]);
    printf("\n");
    return stack -> err_stat;
}

stack_err stack_verify(stack_t* stack)
{
    assert(stack);
    if (stack == NULL || stack -> err_stat != OK || stack -> data == NULL)
    {
        fprintf(stderr, "VERIFICATION FAILED\n");
        return VERIF_ERROR;
    }
    if (stack -> size > stack -> capacity)
    {
        fprintf(stderr, "VERIFICATION FAILED: size > capacity\n");
        return SIZE_ERROR;
    }

    return stack -> err_stat;
}

stack_err stack_realloc(stack_t* stack, stack_realloc_state state)
{
    assert(stack);

    stack_verify(stack);

    size_t new_capacity = 0;
    switch(state)
    {
        case INCREASE: new_capacity = stack -> capacity * realloc_coeff;
                       break;
        case DECREASE: new_capacity = stack -> capacity / (realloc_coeff * realloc_coeff);
                       break;
        default: break;
    }

    stack_elem_t* ptr = (stack_elem_t*)calloc(new_capacity, sizeof(stack_elem_t));
    if (ptr == NULL)
    {
        fprintf(stderr, "Cannot reallocate memory for stack\n");
        return ALLOC_ERROR;
    }
    else
    {
        DEBUG_PRINTF("ptr start:    %p\n"
                     "ptr new area: %p\n"
                     "stack cap:    %d\n"
                     "new cap:      %d\n"
                     "count for memset - [%d]\n",
                     ptr, ptr + stack -> capacity, stack -> capacity, new_capacity,
                     (new_capacity - stack -> capacity) * sizeof(stack_elem_t));
        memcpy(ptr, stack -> data, stack -> capacity * sizeof(stack_elem_t));
        free(stack -> data);
        //memset(ptr + stack -> capacity, 0, (new_capacity - stack -> capacity) * sizeof(stack_elem_t));
        stack -> data = ptr;
        stack -> capacity = new_capacity;
    }
    stack_dump(stack);
    DEBUG_PRINTF("Reallocation completed\n");
    return stack -> err_stat;
}

stack_err stack_push(stack_t* stack, stack_elem_t elem)
{
    assert(stack);

    stack_verify(stack);
    if (stack -> capacity <= stack -> size)
    {
        DEBUG_PRINTF("Reallocation call\n"
                     "size:     %d\n"
                     "capacity: %d\n", stack -> size, stack -> capacity);
        stack_realloc(stack, INCREASE);
    }

    if (stack_verify(stack) != OK)
    {
        fprintf(stderr, "STACK PUSH FAILED\n");
        return stack -> err_stat;
    }

    stack -> data[stack -> size] = elem;
    (stack -> size)++;
    stack_dump(stack);

    DEBUG_PRINTF("Stack_push Executed\n");
    return stack -> err_stat;
}

stack_err stack_pop(stack_t* stack)
{
    assert(stack);
    stack_verify(stack);
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
