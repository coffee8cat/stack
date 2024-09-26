#include "my_stack.h"

int main()
{
    stack_t stack = {};
    stack_init(&stack, 2);
    stack_dump(&stack);
    for (size_t i = 0; i < 18; i++)
        stack_push(&stack, i);
    //stack_pop(&stack);
    stack_dump(&stack);

    stack_delete(&stack);
    return 0;
}
