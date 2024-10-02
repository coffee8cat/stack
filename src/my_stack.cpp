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

    //clear memory
    memset(&stack -> size, 0, ((char*)&stack -> right_canary - (char*)&stack -> size));

    stack -> capacity = init_capacity;
    stack -> size = 0;

    PRINTF_MAGENTA("INIT STACK\n");
    //check if stack is empty
    if (init_capacity > 0)
    {
        //allocate memory for data
        size_t log_byte_capacity = UP_TO_EIGHT(init_capacity * sizeof(stack_elem_t));
        printf("log cap = %ld\n", log_byte_capacity);
        char* ptr = (char*)calloc(log_byte_capacity + 2 * sizeof(canary_t), sizeof(char));
        //check allocated memory
        if (ptr == NULL)
        {
            //return error in case of failure
            stack -> err_stat = ALLOC_ERROR;
            return ALLOC_ERROR;
        }

        //init data pointer
        stack -> data = (stack_elem_t*)(ptr + sizeof(canary_t));

        //init data canaries
        #ifdef CANARY_PROTECT
            *(canary_t*)((char*)stack -> data - sizeof(canary_t))  =  canary_const;
            *(canary_t*)((char*)stack -> data + log_byte_capacity) =  canary_const;
            PRINTF_MAGENTA("data left canary  [%p]\n", (char*)stack -> data - sizeof(canary_t));
            PRINTF_MAGENTA("data right canary [%p]\n", (char*)stack -> data + log_byte_capacity);
        #endif
        PRINTF_MAGENTA("ptr[%p] data[%p]\n", ptr, stack -> data);
    }
    else
    {
        stack -> data = NULL;
    }

    //init canary protection
    #ifdef CANARY_PROTECT
        stack -> left_canary  =  canary_const;
        stack -> right_canary =  canary_const;
    #endif

    //init hash protection
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

    //check if stack pointer is valid
    //return error if not
    if (stack == NULL)
    {
        FPRINTF_RED(stderr, "ERROR: INVALID POINTER(NULL) given to stack_delete\n");
        return PTR_ERROR;
    }

    //clear memory and free allocated memory
    if (stack -> data ==NULL)
    {
        FPRINTF_RED(stderr, "ERROR: INVALID POINTER(NULL) to data in stack: cannot clear data\n");
    }
    else
        memset((char*)stack -> data - sizeof(canary_t), 0, UP_TO_EIGHT(stack -> capacity * sizeof(stack_elem_t)) + 2 * sizeof(canary_t));

    free(stack -> data);
    memset(stack, 0, (size_t)(&stack -> right_canary - &stack -> left_canary) * sizeof(stack_elem_t) + sizeof(canary_t));

    PRINTF_MAGENTA("Stack deleted\n");
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
    printf("hash         [%ld] at (%p)\n", stack -> stack_hash_sum, &stack -> stack_hash_sum);
    printf("size:        [%ld]\n", stack -> size);
    printf("capacity:    [%ld]\n", stack -> capacity);
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
    canary_t canary_value = (canary_t)canary_const;
    if (memcmp(&stack -> left_canary, &canary_value, sizeof(canary_t)) != 0)
    {
        FPRINTF_RED(stderr, "STACK LEFT CANARY DIED([%llx] != [%llx])\n", stack -> left_canary, canary_value);
        stack -> err_stat = STACK_CANARY_DIED;
        return STACK_CANARY_DIED;
    }

    if (memcmp(&stack -> right_canary, &canary_value, sizeof(canary_t)) != 0)
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

    canary_t data_canary_value = canary_const;
    size_t log_byte_capacity = UP_TO_EIGHT(stack -> capacity * sizeof(stack_elem_t));
    if (memcmp((char*)stack -> data - sizeof(canary_t), &data_canary_value, sizeof(canary_t)) != 0)
    {
        FPRINTF_RED(stderr, "DATA LEFT CANARY DIED([%llx] != [%llx])\n", *(canary_t*)((char*)stack -> data - sizeof(canary_t)), data_canary_value);
        stack -> err_stat = DATA_CANARY_DIED;
        return DATA_CANARY_DIED;
    }
    if (memcmp((char*)stack -> data + log_byte_capacity, &data_canary_value, sizeof(canary_t)) != 0)
    {
        FPRINTF_RED(stderr, "DATA RIGHT CANARY DIED([%llx](%p) != [%llx])\n",
                    *(canary_t*)((char*)stack -> data + log_byte_capacity), (char*)stack -> data + log_byte_capacity,
                    data_canary_value);
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

uint64_t murmur64(uint64_t h)
{
  h ^= h >> 33;
  h *= 31;
  h ^= h >> 33;
  h *= 31;
  h ^= h >> 33;
  return h;
}

uint64_t MurmurHash64A(const void* key, int len, uint64_t seed)
{
    const uint64_t m = 0xc6a4a7935bd1e995;
    const int r = 47;

    uint64_t h = seed ^(len * m);

    const uint64_t* data = (const uint64_t*) key;
    const uint64_t* end = data + (len / 8);

    while (data != end) {
        uint64_t k = *data++;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    const unsigned char* data2 = (const unsigned char*) data;

    switch (len & 7) {
        case 7:
            h ^= uint64_t(data2[6]) << 48;
        case 6:
            h ^= uint64_t(data2[5]) << 40;
        case 5:
            h ^= uint64_t(data2[4]) << 32;
        case 4:
            h ^= uint64_t(data2[3]) << 24;
        case 3:
            h ^= uint64_t(data2[2]) << 16;
        case 2:
            h ^= uint64_t(data2[1]) << 8;
        case 1:
            h ^= uint64_t(data2[0]);
            h *= m;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    return h;
}

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
    uint64_t hash_sum = 0;
    char* curr = start;
    while (curr < end)
    {
        hash_sum = hash_sum * hash_coeff + *curr;
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

    //calculating new capacity
    size_t new_capacity = 0;
    switch(state)
    {
        case INCREASE: new_capacity      = stack -> capacity * realloc_coeff;
                       break;
        case DECREASE: new_capacity = stack -> capacity / (realloc_coeff * realloc_coeff);
                       break;
        default: break;
    }
    size_t log_byte_capacity = UP_TO_EIGHT(new_capacity * sizeof(stack_elem_t));

    //Starting reallocation
    PRINTF_YELLOW("ptr to reallocate [%p]\n", (char*)stack -> data - sizeof(canary_t));
    char* ptr = (char*)realloc((char*)stack -> data - sizeof(canary_t), log_byte_capacity + 2 * sizeof(canary_t));
    //checking reallocated pointer
    if (ptr == NULL)
    {
        FPRINTF_RED(stderr, "Cannot reallocate memory for stack\n");
        CHECK_STACK(stack);
        stack -> err_stat = ALLOC_ERROR;
        return ALLOC_ERROR;
    }
    else
    {
        //print info about reallocation
        PRINTF_YELLOW("ptr start:    %p\n", ptr);
        PRINTF_YELLOW("data start:   %p\n", ptr + sizeof(canary_t));
        PRINTF_YELLOW("ptr new area: %p\n", ptr + sizeof(canary_t) + sizeof(stack_elem_t) * stack -> capacity);
        PRINTF_YELLOW("stack cap:    %ld\n", stack -> capacity);
        PRINTF_YELLOW("log cap:      %ld\n", log_byte_capacity);
        PRINTF_YELLOW("new cap:      %ld\n", new_capacity);
        PRINTF_YELLOW("offset - [%ld]\n",   (new_capacity - stack -> capacity) * sizeof(stack_elem_t) + sizeof(canary_t));

        //clear added memory in case of increasing
        if (state == INCREASE)
            memset(ptr + sizeof(canary_t) + stack -> capacity * sizeof(stack_elem_t), 0, (new_capacity - stack -> capacity) * sizeof(stack_elem_t));

        //updating data pointer and capacity
        stack -> data  = (stack_elem_t*)(ptr + sizeof(canary_t));
        stack -> capacity = new_capacity;

        //updating canary protection
        #ifdef CANARY_PROTECT
            *(canary_t*)((char*)stack -> data - sizeof(canary_t))  =  canary_const;
            *(canary_t*)((char*)stack -> data + log_byte_capacity) =  canary_const;
            PRINTF_YELLOW("data left canary  [%llx] at (%p)\n", *((char*)stack -> data - sizeof(canary_t)), (char*)stack -> data - sizeof(canary_t));
            PRINTF_YELLOW("data right canary [%llx] at (%p)\n", *((char*)stack -> data + log_byte_capacity), (char*)stack -> data + log_byte_capacity);
        #endif
    }

    //update hash
    #ifdef HASH_PROTECT
        //hash must not include itself in calculation
        stack -> data_hash_sum = calc_hash((char*)stack -> data, (char*)stack -> data + UP_TO_EIGHT(stack -> capacity * sizeof(stack_elem_t)));
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

    //check if there is enough memory for element
    //start reallocation if stack is full
    if (stack -> capacity <= stack -> size)
    {
        DEBUG_PRINTF("Reallocation call\n"
                     "size:     %d\n"
                     "capacity: %d\n", stack -> size, stack -> capacity);
        stack_realloc(stack, INCREASE);


        //update hash
        #ifdef HASH_PROTECT
            //hash must not include itself in calculation
            stack -> data_hash_sum = calc_hash((char*)stack -> data, (char*)stack -> data + UP_TO_EIGHT(stack -> capacity * sizeof(stack_elem_t)));
            stack -> stack_hash_sum = calc_hash((char*)stack, (char*)&stack -> stack_hash_sum);
        #endif
        CHECK_STACK(stack);
    }

    //actually push element
    stack -> data[stack -> size] = elem;
    (stack -> size)++;

    //update hash
    #ifdef HASH_PROTECT
        //hash must not include itself in calculation
        stack -> data_hash_sum = calc_hash((char*)stack -> data, (char*)stack -> data + UP_TO_EIGHT(stack -> capacity * sizeof(stack_elem_t)));
        stack -> stack_hash_sum = calc_hash((char*)stack, (char*)&stack -> stack_hash_sum);
    #endif

    STACK_DUMP(stack, __func__);
    PRINTF_RED("-----STACK PUSH END-----\n");
    CHECK_STACK(stack);

    return stack -> err_stat;
}

stack_elem_t stack_pop(stack_t* stack)
{
    assert(stack);
    PRINTF_RED("-----STACK POP-----\n");
    CHECK_STACK(stack);

    stack_elem_t temp = 0;
    //check if stack is empty
    if (stack -> size > 0)
    {
        //remove element
        temp = *(stack -> data + stack -> size - 1);
        *(stack -> data + stack -> size - 1) = 0;
        stack -> size--;
    }
    else
    {
        //stack is empty, return error
        stack -> err_stat = SIZE_ERROR;
        return SIZE_ERROR;
    }

    //update hash
    #ifdef HASH_PROTECT
        //hash must not include itself in calculation
        stack -> data_hash_sum  = calc_hash((char*)stack -> data, (char*)stack -> data + UP_TO_EIGHT(stack -> capacity * sizeof(stack_elem_t)));
        stack -> stack_hash_sum = calc_hash((char*)stack, (char*)&stack -> stack_hash_sum);
    #endif

    //check if reallocation needed
    printf("size           = %ld\n", stack -> size);
    printf("capacity       = %ld\n", stack -> capacity);
    printf("realloc border = %ld\n", stack -> capacity / (realloc_coeff * realloc_coeff));
    if (stack -> size <= stack -> capacity / (realloc_coeff * realloc_coeff))
    {
        PRINTF_RED("REALLOC CALLED FROM stack_pop\n");
        stack_realloc(stack, DECREASE);
    }

    CHECK_STACK(stack);
    STACK_DUMP(stack, __func__);
    PRINTF_RED("-----STACK POP END-----\n");
    return temp;
}
