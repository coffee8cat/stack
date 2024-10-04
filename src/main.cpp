#include "my_stack.h"
#include <TXLib.h>

int main()
{
    stack_t stack = {INIT(stack)};
    stack_init(&stack, 8, 8);
    STACK_DUMP(&stack, __func__);
    for (size_t i = 0; i < 20; i++)
        stack_push(&stack, i * 10);

    STACK_DUMP(&stack, __func__);
    stack_delete(&stack);
    return 0;
}
