#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc,char* argv[])
{
	printf("by");
	int id= fork();
	printf("hi how are you %d",id);
	return 0;
}