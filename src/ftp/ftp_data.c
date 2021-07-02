#include "ftp_data.h"
#include "ftp.h"
#include "ftp_control.h"
#include "../utils/log.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "unistd.h"


int parse_mlsx(char *buffer,struct ftp_fs *ftfs)
{
	//fills ftfs with parsed MLSD/MLST response info
	//return value
	//0 -success
	//-1-failed
	
	//MLSX response format:
	//fact=value;fact=value; pathname\n
	//fact=value;fact=value; pathname\n

	char *curr_line,
		*new_line;
	curr_line = buffer;
	


	struct ftp_file *curr_file;
	
	if((curr_file = (struct ftp_file*)malloc(sizeof curr_file))==NULL)
		return -1;
	
	ftfs->files = curr_file;
	ftfs->files->next = NULL;
	
	//fact=value;fact=value;fact=value;fact=value; name\nfact=value;fact=value; name\n

	//c						    n
	//l   m     r



	if((new_line = strchr(buffer,'\n'))==NULL)
	{
		return -1;
	}
	do//line by line
	{
		*new_line = '\0';//terminate at '\n'	
	
		char *left,
			*mid,// '='
			  *right;
		
		left = curr_line;
		if((right = strchr(curr_line,';'))==NULL)
		{
			return -1;
		}
		if((mid = strchr(curr_line,'='))==NULL)
		{
			return -1;
		}

		do//expression by expression, fact=value;
		{//			      l   m     r
			*mid   = '\0';
			*right = '\0';

			char *fact,//left of '='
				*value;//right of '='

			if((fact = (char*)malloc(strlen(left)))==NULL ||
				(value = (char*)malloc(strlen(mid+1)))==NULL)
				return -1;

	

			snprintf(fact,strlen(left)+1,"%s",left);//left to mid
			snprintf(value,strlen(mid+1)+1,"%s",mid+1);//mid to right

			
			if(!strcmp(fact,"type"))
			{
				curr_file->type = value;	
			}
			else if(!strcmp(fact,"sizd") || !strcmp(fact,"size"))
			{
				curr_file->size = strtol(value,NULL,10);
			}

			left = right+1;
			right = strchr(left,';');
			mid = strchr(left,'=');
			
		
		}while(right!=NULL);
		
		if(*left==' ')//name, format is face=value; name
				//			   l   
		{
			if((curr_file->name = (char *)malloc(strlen(left+1)))==NULL)
				return -1;
		
			snprintf(curr_file->name,strlen(left+1),"%s",left+1);
		}
		

		curr_line = new_line +1;
		new_line = strchr(curr_line,'\n');
	
			
		if(new_line != NULL)
		{
			if((curr_file->next = (struct ftp_file*)malloc(sizeof curr_file))==NULL)
					return -1;
			else
			{
				curr_file = curr_file->next;
			}
		}
		else
		{
			curr_file->next =NULL;
			break;
		}	


	}while(1);
	
	return 0;

	
	
}




int ftpd_list(struct ftp_server *ftps,struct ftp_fs *ftfs,const char *dir)
{
	//MLSD command, fills ftp_fs
	//sends LIST command,waits for server to confirm that data connection is closed
	//return value
	//0 -success
	//-1-failed

        if(!(ftps->server_status & FTPS_CONTROL_CONNECTED))
	{
                log_error("ftpd_list:ftp_server not connected.");
                return -1;
        }




	//establish data connection
	if(ftpd_connect(ftps,FTPD_ACTIVE) == -1)
	{
		return -1;
	}
	
	struct ftp_response *fres;

        char *command_str;
        if(ftp_command_str(&command_str,"MLSD",dir) == -1)
        {
		 return -1;
		 ftpd_disconnect(ftps);
	}
        if(ftp_command(ftps,&fres,command_str)==-1)
        {
                log_error("ftpc_list: MLSD command failed.");
                ftp_response_free(fres);
               
		ftpd_disconnect(ftps);
	        return -1;
        }
	
	
	//temp
	if(ftp_accept(ftps)==-1)
	{
		log_error("ftpd_list: accept failed");
		return -1;
	}	
	if(fres->code != FTPC_DATA_OPENING)
	{
		char buf[16];
                snprintf(buf,16,"code:%d",fres->code);
                log_error("ftpd_list: command MLSD failed.");
                log_error(buf);
                ftp_response_free(fres);
              
	        return -1;
	}




	ftp_response_free(fres);
	
	char *buffer;

	if(ftp_receive(ftps,ftps->dc_socket,&buffer,FTPT_CONTROL) == -1)
	{
		log_error("ftpd_list: data ftp_receive() failed");
		free(buffer);
		ftpd_disconnect(ftps);
		return -1;
	}


	if(parse_mlsx(buffer,ftfs)==-1)
	{
		log_error("ftpd_list: can't parse response.");
		free(buffer);
		ftpd_disconnect(ftps);
		return -1;
	}

	//close data connection
	if(ftpd_disconnect(ftps) == -1)
		return -1;


	log_message("ftpd_list: success. ");	
	free(buffer);
	return 0;
}


int ftpd_connect(struct ftp_server *ftps,int contype)
{
	//attempts data connection.
	//connection types:
	//ACTTIVE and PASSIVE
	//return value:
	//0  - success
	//-1 - failed

        if(!(ftps->server_status & FTPS_CONTROL_CONNECTED))
        {
                log_error("ftpd_connect:no control connection.");
                return -1;
        }
	if(!(ftps->server_status & FTPS_LOGGED_IN))
        {
                log_error("ftpd_connect:not logged in. ");
                return -1;
        }

	if(ftps->server_status & FTPS_DATA_CONNECTED)
	{
		log_warning("ftpd_connect: ftp_server already connected.");
		return 0;//i guess ? 
	}



	//active
	if(contype == FTPD_ACTIVE &&
		ftpc_active(ftps) != -1)
	{
		
		log_message("ftpd_connect: active connection established.");
		ftps->server_status |= FTPS_DATA_CONNECTED;	
		return 0;
	}
	else if(contype == FTPD_ACTIVE)
	{
		
		log_error("ftpd_connect: active data connection failed.");
		return -1;
	}


	//passive
	if(contype == FTPD_PASSIVE &&
		ftpc_passive(ftps) != -1 &&
			ftp_connect(ftps->dc_info,&(ftps->dc_socket)) != -1)
	{
		log_message("ftpd_connect: passive connection established.");
		ftps->server_status |= FTPS_DATA_CONNECTED;	
		return 0;
	}
	else if(contype == FTPD_PASSIVE)
	{
		log_error("ftpd_connect: passive data connection failed.");
		return -1;
	}


	return -1;	
}


int ftpd_disconnect(struct ftp_server *ftps)
{
	//stops data connection by checking if server send 
	//the response saying that the transfer is done
	//return value:
	//0 - success
	//-1 - failed
	
	if(!(ftps->server_status & FTPS_DATA_CONNECTED))
	{
		log_warning("ftpd_disconnect: no data connection. ");
		return -1;
	}

	
	char *buffer;
	
	if(ftp_receive(ftps,ftps->cc_socket,&buffer,FTPT_CONTROL) == -1)
	{
		log_error("ftpd_disconnect: ftp_receive() failed");
		return -1;
	}

	if(strtol(buffer,NULL,10) != FTPC_DATA_CLOSING)
	{
		char buf[16];
                snprintf(buf,16,"code:%ld",strtol(buffer,NULL,10));
                log_error("ftpd_disconnect: error.");
                log_error(buf);
		free(buffer);
                return -1;
	}


	close(ftps->dc_socket);
	ftps->server_status ^= FTPS_DATA_CONNECTED;

	free(buffer);
	return 0;
}






