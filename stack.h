/*Stack code borrowed from 
 *http://groups.csail.mit.edu/graphics/classes/6.837/F04/cpp_notes/stack1.html
*/
#define STACK_MAX 1000
struct Stack {
    long int     data[STACK_MAX];
    unsigned int     size;
};

typedef struct Stack Stack;

void Stack_Init(Stack *S);
int Stack_Top(Stack *S);
void Stack_Push(Stack *S, long int d);
void Stack_Pop(Stack *S);
