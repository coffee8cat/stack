#include "my_stack.h"
#include <TXLib.h>

int main()
{
    stack_t stack = {INIT(stack)};
    stack_init(&stack, 8);
    // STACK_DUMP(&stack, __func__);
    // for (size_t i = 0; i < 5; i++)
    // {
    //     stack_push(&stack, 10 * i);
    //     STACK_DUMP(&stack, __func__);
    // }
    // for (size_t i = 0; i < 1; i++)
    // {
    //     stack_pop(&stack);
    //     STACK_DUMP(&stack, __func__);
    // }
    // STACK_DUMP(&stack, __func__);
    printf()
    stack_delete(&stack);
    return 0;
}
