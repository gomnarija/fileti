#include "log.h"

#include "stdio.h"
#include "time.h"
#include "stdlib.h"
#include "string.h"

void log_log(int log_level,const char *message)
{
	FILE *fp;
	time_t t  = time(NULL);
	struct tm log_time =  *localtime(&t);	

	char lvl_msg[16];
	switch(log_level)
	{
		case LOG_WARNING:
			strcpy(lvl_msg,"[WARNING]");break;

		case LOG_ERROR:
			strcpy(lvl_msg,"[ERROR]");break;
		default:
			strcpy(lvl_msg,"[MESSAGE]");
	}

		
	if((fp  = fopen(LOG_FILE,"a+")) != NULL)
	{
		if(fprintf(fp,"[%d-%02d-%02d %02d:%02d:%02d]", //current time
						 	log_time.tm_year + 1900, 
							log_time.tm_mon + 1, 
							log_time.tm_mday, 
							log_time.tm_hour, 
							log_time.tm_min, 
							log_time.tm_sec))
		
	
			if(fprintf(fp,lvl_msg))//log level
				if(fprintf(fp,">"))//>
					fprintf(fp,message);//message
	
	
		fprintf(fp,"\n");//new line always
		
		fclose(fp);		
	}
}

