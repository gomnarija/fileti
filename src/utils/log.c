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


#include "log.h"
#include "io.h"

#include "stdio.h"
#include "time.h"
#include "stdlib.h"
#include "string.h"



int RAW_READY;

void log_log(int log_level,const char *message)
{
	FILE *fp;
	time_t t  = time(NULL);
	struct tm log_time =  *localtime(&t);	

	char lvl_msg[16],time_msg[22],*msg;

	switch(log_level)
	{
		case LOG_WARNING:
			snprintf(lvl_msg,10,"%s","[WARNING]");break;

		case LOG_ERROR:
			snprintf(lvl_msg,8,"%s","[ERROR]");break;
		default:
			snprintf(lvl_msg,10,"%s","[MESSAGE]");
	}
	
	snprintf(time_msg,22,"[%d-%02d-%02d %02d:%02d:%02d]", //current time
						 	log_time.tm_year + 1900, 
							log_time.tm_mon + 1, 
							log_time.tm_mday, 
							log_time.tm_hour, 
							log_time.tm_min, 
							log_time.tm_sec);
		


	if((msg = (char *)malloc(strlen(message)+strlen(lvl_msg)+strlen(time_msg)+3))==NULL)
		return;


	snprintf(msg,
		strlen(message)+strlen(lvl_msg)+strlen(time_msg)+3,
			"%s%s>%s\n",time_msg,lvl_msg,message);

	if((fp  = fopen(LOG_FILE,"a+")) != NULL)
	{
			
		fwrite(msg,sizeof(char),strlen(msg)+1,fp);

		fclose(fp);		
	}
	free(msg);
}
void log_raw(const char *message,const int nl)
{
	RAW_READY = 1;

	FILE *fp;
	if((fp  = fopen(RAW_FILE,"a+")) != NULL)
	{
			
		fprintf(fp,"%s",message);
		if(nl)
			fprintf(fp,"\n");
		fclose(fp);		
	}
}

void raw_init()
{	
	remove(RAW_FILE);
}


void get_raw(char **buffer)
{
	int sz=8000;

	
	*buffer = (char *)malloc(sz);
	
	FILE *fp;
	if((fp  = fopen(RAW_FILE,"rb")) != NULL)
	{
		
		io_read(RAW_FILE,buffer,&sz,&fp);
		(*buffer)[sz-1] =  '\0';
	}
	fclose(fp);
	
}
