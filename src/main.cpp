#include "my_stack.h"
#include <TXLib.h>

int main()
{
    stack_t stack = {INIT(stack)};
    stack_init(&stack, 13, 1);
    for (size_t i = 0; i < 20; i++)
        stack_push(&stack, i * 10);

    stack_dump(&stack, __FILE__, __LINE__, __func__, &dump_char); dump(stack, dump_har);
    uint64_t temp = 0;
    for (size_t i = 0; i < 20; i++)
    {
        stack_pop(&stack, &temp);
        printf("temp = [%c](%d)\n", temp, temp);
    }
    stack_delete(&stack);
    return 0;
}
