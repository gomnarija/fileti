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


#include "ftp.h"
#include "ftp_control.h"
#include "ftp_data.h"
#include "../utils/log.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"

int parse_mlsx(char *buffer,struct ftp_file **fifi)
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
	
	if((curr_file = (struct ftp_file*)malloc(sizeof(struct ftp_file)))==NULL)
		return -1;


	//clear if not empty
	while(*fifi)
	{
	
		struct ftp_file *tmp;
		tmp = *fifi;
	
		*fifi = (*fifi)->next;	

		if(tmp->name)
			free(tmp->name);
		if(tmp->type)
			free(tmp->type);
		
		if(tmp)
			free(tmp);
	}

	*fifi = curr_file;
	curr_file->next = NULL;
	curr_file->name = NULL;
	curr_file->type = NULL;




	
	//fact=value;fact=value;fact=value;fact=value; name\nfact=value;fact=value; name\n

	//c						    n
	//l   m     r


	if((new_line = strchr(buffer,'\n'))==NULL)
		return -1;

	do//line by line
	{
		*new_line = '\0';//terminate at '\n'	
	
		char *left,
			*mid,// '='
			  *right;
		
		left = curr_line;
		if((right = strchr(curr_line,';'))==NULL)
			return -1;
		
		if((mid = strchr(curr_line,'='))==NULL)
			return -1;

		do//expression by expression, fact=value;
		{//			      l   m     r
			
			*mid   = '\0';
			*right = '\0';

			char *fact,//left of '='
				*value;//right of '='

			if((fact = (char*)malloc(strlen(left)+1))==NULL ||
				(value = (char*)malloc(strlen(mid+1)+1))==NULL)
				return -1;

	

			snprintf(fact,strlen(left)+1,"%s",left);//left to mid
			snprintf(value,strlen(mid+1)+1,"%s",mid+1);//mid to right

			
			if(!strcmp(fact,"type"))
			{
				if((curr_file->type = (char *)malloc(strlen(value)+1))==NULL)
				{
					free(fact);
					free(value);
					return -1;
				}

				snprintf(curr_file->type,strlen(value)+1,"%s",value);
			}
			else if(!strcmp(fact,"sizd") || !strcmp(fact,"size"))
			{
				curr_file->size = strtol(value,NULL,10);
			}
			else if(!strcmp(fact,"UNIX.mode"))
			{
				curr_file->perms = strtol(value,NULL,8);

			}
			free(fact);
			free(value);

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
			if((curr_file->next = (struct ftp_file*)malloc(sizeof(struct ftp_file)))==NULL)
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
	//sends MLSD command,waits for server to confirm that data connection is closed
	//return value
	//0 -success
	//-1-failed


	

        if(!ftp_check_server_status(ftps,FTPS_CONTROL_CONNECTED | FTPS_LOGGED_IN,"ftpd_list"))
                return -1;


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
	 

	while(!(ftps->server_status & FTPS_DATA_CONNECTED))
	{
		if(ftps->dc_socket == -1)
                 {
			 log_error("ftpd_list: no data connection. ");
               	 	 return -1;
		}
		else
		{
			log_warning("ftpd_list: waiting for data connection");
			sleep(1);	
		}
        }



	
	if(fres->code != FTPC_DATA_OPENING)
	{
		ftp_command_failed(fres->code,fres->message,"MLSD");
                ftp_response_free(fres);
              
	        return -1;
	}




	ftp_response_free(fres);
	
	char *buffer;
	int response_size = -1;


	int rcv;
	if((rcv=ftp_receive(ftps,ftps->dc_socket,&buffer,&response_size)) == -1)
	{	
		log_error("ftpd_list:ftp_receive failed.");
		return -1;
	}
	else if(rcv==-2)
	{
		log_warning("ftpd_list: ftp_receive read nothing. ");
		ftpd_disconnect(ftps);
		return -1;
	}

	//terminate buffer
	if((buffer = (char*) realloc(buffer,response_size+1))==NULL)
	{	
		log_error("ftpd_list: realloc() failed.");
		ftpd_disconnect(ftps);
		return -1;
	}
	buffer[response_size++] = '\0';




	
	if(parse_mlsx(buffer,&(ftfs->files))==-1)
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

int ftpc_ent_info(struct ftp_server *ftps,const char *ent,struct ftp_file **fifi)
{

	//MLST command, returns info about file/dir
	//return value
	//0 -success
	//-1-failed

        if(!ftp_check_server_status(ftps,FTPS_CONTROL_CONNECTED,"ftpd_list"))
                return -1;


	char *command_str;
	
	if(ftp_command_str(&command_str,"MLST",ent) == -1)
		return -1;


	if(ftp_send(ftps,ftps->cc_socket,command_str,strlen(command_str)) == -1)
	{
		log_error("ftpc_ent_info: ftp_send() failed.");
		if(command_str)free(command_str);
		return -1;
	
	}	
	if(command_str)free(command_str);

	
	int rcv;
	int response_size = -1;
	char *buffer;
	if((rcv = ftp_receive(ftps,ftps->cc_socket,&buffer,&response_size)) == -1)
	{
	
		log_error("ftpc_ent_info: ftp_receive() failed.");
		return -1;
	}
	else if(rcv == -2)
	{
		log_warning("ftpc_ent_info: ftp_receive() read nothing. ");
		return -1;
	}

	//format:
	//250-Begin 
	//	mlsx-message 
	//250 End
	
	if(strtol(buffer,NULL,10) != FTPC_FILE_OK)
	{
		char *endp;
		ftp_command_failed(strtol(buffer,&endp,10),endp,"MLST");
		free(buffer);
                return -1;
	}

	char *startp,
		*endp;

	startp = strchr(buffer,'t');// type = ... \n250
				//     s 	   e 
	endp   = strchr(startp,'\n');	
	

	if(startp == NULL || endp == NULL)
	{
		log_error("ftpc_ent_info: wrong response format. ");
		free(buffer);
		return -1;
	}	
	*++endp = '\0';
	

	*fifi = (struct ftp_file *)malloc(sizeof(struct ftp_file));
	if(!*fifi)
	{
		log_error("ftpc_ent_info: malloc failed");
		return -1;
	}

	(*fifi)->next = NULL;
	(*fifi)->name = NULL;
	(*fifi)->type = NULL;	
	if(parse_mlsx(startp,fifi)==-1)
	{
		log_error("ftpc_ent_info: can't parse response.");
		free(buffer);
		return -1;
	}

	log_message("ftpc_ent_info: success. ");	
	free(buffer);

	return 0;

}
