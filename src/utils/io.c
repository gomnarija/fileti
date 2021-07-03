#include "io.h"



int io_write(const char * file_name,const char *buffer,int buff_size)
{
	FILE *fp = fopen(file_name,"a+");

	printf("asd %d",buff_size);
	if(!fp)
		return -1;
	if(fwrite(buffer,sizeof(char),buff_size,fp) <= 0)
		return -1;
	
	fclose(fp);	

	return 0;
}
