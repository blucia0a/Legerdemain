#include "stack.h"
void Stack_Init(Stack *S)
{
    S->size = 0;
}

int Stack_Top(Stack *S)
{
    if (S->size == 0) {
        return -1;
    } 

    return S->data[S->size-1];
}

void Stack_Push(Stack *S, int d)
{
    if (S->size < STACK_MAX)
        S->data[S->size++] = d;
    else
        return;
}

void Stack_Pop(Stack *S)
{
    if (S->size == 0)
        return;
    else
        S->size--;
}
