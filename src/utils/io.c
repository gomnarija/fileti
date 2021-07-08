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

int io_list(struct ftp_fs **ftfs,const char *dir_name)
{

	if((*ftfs = (struct ftp_fs *)malloc(sizeof(struct ftp_fs)))==NULL)
	{	
		log_error("io_list:malloc() failed.");
		*ftfs = NULL;
		return -1;
	}
	(*ftfs)->files = NULL;
	(*ftfs)->pwd   = NULL;

	DIR *dp;
	struct dirent **dire;

	dp = opendir(dir_name);
	if(!dp)	
	{
		free(*ftfs);
		*ftfs = NULL;
		log_error("io_list: couldn't open dir.");
		log_error(dir_name);
		return -1;
	}

	if(((*ftfs)->pwd = (char*)malloc(strlen(dir_name)+1))==NULL)
	{	
		log_error("io_list:malloc() failed.");
		*ftfs = NULL;
		return -1;
	}
	snprintf((*ftfs)->pwd,strlen(dir_name)+1,"%s",dir_name);




	struct ftp_file **fifi;
	fifi = &((*ftfs)->files);
	
	int n = scandir(dir_name,&dire,NULL,alphasort);


	int i=0;
	while(i<n)
	{
		if((*fifi = (struct ftp_file *)malloc(sizeof(struct ftp_file)))==NULL ||
			((*fifi)->name = (char *)malloc(strlen(dire[i]->d_name)+1))==NULL ||
			((*fifi)->type = (char *)malloc(5))==NULL)
		{	
			log_error("io_list:malloc() failed.");
			free(dire);
			closedir(dp);
			return -1;
		}
		(*fifi)->next = NULL;
		snprintf((*fifi)->name,strlen(dire[i]->d_name)+1,"%s",dire[i]->d_name);
		snprintf((*fifi)->type,5,"%s",dire[i]->d_type==DT_DIR?"dir":"file");
		fifi = &((*fifi)->next);
		i++;		
		
	}

	closedir(dp);
	free(dire);


	return 0;
}

int io_pwd(char **buff)
{
	if((*buff = (char *)malloc(PATH_MAX)) == NULL)
		return -1;


	if(getcwd(*buff,PATH_MAX) == NULL)
	{
		free(*buff);
		return -1;
	}
	return 0;
}
