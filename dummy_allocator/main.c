#include <stdio.h>
#include "allocation.h"




int main(int argc, char **argv)
{
	int* i = NULL;
	//printf("%d", *i);
	//my_free(i);
		i = my_calloc(sizeof(int)*13);
	
	my_free(i);
	//out();
	printf("KEK\n");
	i = my_malloc(sizeof(int)*3);
	my_free(i);
	i = my_realloc(i, 4);
	my_free(i);
	i = my_realloc(i, 100);
	my_free(i);
	i = my_realloc(i, 150);
	my_free(i);
	i = my_realloc(i, 138);
	
	
	
	
	//out();
	return 0;
}
