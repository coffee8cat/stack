#include "my_stack.h"
#include <TXLib.h>

int main()
{
    stack_t stack = {INIT(stack)};
    stack_init(&stack, 0);
    STACK_DUMP(&stack, __func__);
    for (size_t i = 0; i < 5; i++)
        stack_push(&stack, 10 * i);
    for (size_t i = 5; i > 0; i--)
    {
        stack_pop(&stack);
    }
    STACK_DUMP(&stack, __func__);

    stack_delete(&stack);
    //fclose(log_thread);
    return 0;
}
