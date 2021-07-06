//   Copyright 2021 gomnarija 
//
//   This file is part of fileTI.
//
//   fileTI is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
//
//   fileTI is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with fileTI; if not see <http://www.gnu.org/licenses/>.
//


#include "io.h"
#include "stdlib.h"


int io_write(const char * file_name,const char *buffer,int buff_size)
{
	FILE *fp = fopen(file_name,"a+");

	if(!fp)
		return -1;
	if(fwrite(buffer,sizeof(char),buff_size,fp) <= 0)
		return -1;
	
	fclose(fp);	

	return 0;
}


int io_read(const char * file_name,char **buffer,int *buff_size,FILE **fp)
{
	if(!(*fp))
	{
		*fp = fopen(file_name,"r");
		if(!(*fp))
			return -1;
	}
	if((*buff_size=fread(*buffer,sizeof(char),*buff_size,*fp)) <= 0)
	{
		fclose(*fp);
		return -1;
	}
	
	if(!(*buffer=realloc(*buffer,*buff_size)))
		return -1;	


	return 0;
	
	

}
