#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv){

    int c, nc = 0, nl = 0;

    while ( (c = getchar()) != EOF )
    {
	nc++;
	if (c == '\n') nl++;
    }

    printf("Number of characters = %d, number of lines = %d\n",
	   nc, nl);
}
