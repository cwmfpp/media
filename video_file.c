#include <stdio.h>

int write_file(char *name, char *src, int count)
{
	FILE *file;
	file = fopen(name, "a+");

	fwrite(src, 1, count, file);
	if(fclose(file) == 0)
	{
		printf("write file ok\n");
	}
}

