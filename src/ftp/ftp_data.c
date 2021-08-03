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


#include "ftp_data.h"
#include "ftp.h"
#include "ftp_control.h"
#include "../utils/io.h"
#include "../utils/log.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "unistd.h"
#include "sys/stat.h"


int ftpd_connect(struct ftp_server *ftps,int contype)
{
	//attempts data connection.
	//connection types:
	//ACTTIVE and PASSIVE
	//return value:
	//0  - success
	//-1 - failed



        if(!ftp_check_server_status(ftps,FTPS_CONTROL_CONNECTED |
					FTPS_LOGGED_IN,"ftpd_connect"))
                return -1;

	if(ftps->server_status & FTPS_DATA_CONNECTED)
	{
		log_warning("ftpd_connect: ftp_server already connected.");
		return 0;//i guess ? 
	}



	//active
	if(contype == FTPD_ACTIVE &&
		ftpc_active(ftps) != -1 &&
 			pthread_create(&(ftps->dc_thread),NULL,ftp_accept,(void*)ftps) == 0)
	{
		log_message("ftpd_connect: socket is listening. ");
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
	int response_size = -1;
	
	int rcv;
	if((rcv=ftp_receive(ftps,ftps->cc_socket,&buffer,&response_size)) == -1)
	{	
		log_error("ftpd_disconnect:ftp_receive failed.");
		return -1;
	}
	else if(rcv==-2)
	{
		log_warning("ftpd_disconnect: ftp_receive read nothing. ");
		return -1;
	}


	if(strtol(buffer,NULL,10) != FTPC_DATA_CLOSING)
	{
		char *endp;
		ftp_command_failed(strtol(buffer,&endp,10),endp,"[]");
		free(buffer);
                return -1;
	}
	

	ftps->server_status ^= FTPS_DATA_CONNECTED;

	close(ftps->dc_socket);
	
	pthread_join(ftps->dc_thread,NULL);
	ftps->dc_thread = 0;	

	free(buffer);

	log_message("ftpd_disconnect: data connection stopped. ");

	return 0;
}

int ftpd_retrieve_file(struct ftp_server *ftps,const char *src_name,const char *dst_name,const int perms)
{
	//RETR command, returns file over data connection
	//return value
	//fails if file exists
	//0 -success
	//-1-failed


	

	if(!ftp_check_server_status(ftps,FTPS_CONTROL_CONNECTED | FTPS_LOGGED_IN,"ftpd_retrieve_file"))
                return -1;


		
	struct stat sta = {0};
	if(stat(dst_name,&sta)!=-1)
	{
		log_error("ftpd_retrieve_file: file already exists. ");
		log_error(dst_name);
		return -1;	
	}

	//establish data connection
	if(ftpd_connect(ftps,FTPD_ACTIVE) == -1)
	{
		return -1;
	}
	
	struct ftp_response *fres;

        char *command_str;
        if(ftp_command_str(&command_str,"RETR",src_name) == -1)
        {
		 return -1;
		 ftpd_disconnect(ftps);
	}
        if(ftp_command(ftps,&fres,command_str)==-1)
        {
                log_error("ftpd_retrieve_file: RETR command failed.");
                ftp_response_free(fres);
               
		ftpd_disconnect(ftps);
	        return -1;
        }
	 

	while(!(ftps->server_status & FTPS_DATA_CONNECTED))
	{
		if(ftps->dc_socket == -1)
                 {
			 log_error("ftpd_retrieve_file: no data connection. ");
               	 	 return -1;
		}
		else
		{
			log_warning("ftpd_retrieve_file: waiting for data connection");
			sleep(1);	
		}
        }


	
	if(fres->code != FTPC_DATA_OPENING)
	{
		ftp_command_failed(fres->code,fres->message,"RETR");
                ftp_response_free(fres);
              
	        return -1;
	}




	ftp_response_free(fres);
	
	char *buffer;
	int rcv_size=8000,
		response_size = rcv_size;



	int rcv;
	do
	{
		if((rcv = ftp_receive(ftps,ftps->dc_socket,&buffer,&response_size)) == -1)
		{
			log_error("ftpd_retrieve_file: data ftp_receive() failed");
			free(buffer);
			ftpd_disconnect(ftps);
			return -1;
		}
		else if(rcv==-2)
		{		
			break;
		}

		if(io_write(dst_name,buffer,response_size) == -1)
		{
	
			free(buffer);
			return -1;
		}
		free(buffer);

	}
	while(response_size>=rcv_size);

	

	//close data connection
	if(ftpd_disconnect(ftps) == -1)
		return -1;


	log_message("ftpd_retrieve_file: success. ");	
	return 0;
}


int ftpd_retrieve_dir(struct ftp_server *ftps,const char *src_name,const char *dst_name,const int perms)
{
	//RETR command, returns dir over data connection
	//fails if dir exists
	//return value
	//0 -success
	//-1-failed

	if(!ftp_check_server_status(ftps,FTPS_CONTROL_CONNECTED | FTPS_LOGGED_IN,"ftpd_retrieve_dir"))
                return -1;


	struct ftp_fs	*ftfs;
	struct ftp_file *curr;
	
	ftfs = (struct ftp_fs*)malloc(sizeof(struct ftp_fs));

	ftfs->files = NULL;

	ftfs->pwd = (char*)malloc(strlen(src_name)+1);


	if(ftfs==NULL || ftfs->pwd == NULL)
	{
	
		log_error("ftpd_retrieve_dir: malloc() failed. ");
		return -1;
	}


	snprintf(ftfs->pwd,strlen(src_name)+1,"%s",src_name);

	if(ftpd_list(ftps,ftfs,src_name) == -1)
	{
		ftp_fs_free(ftfs);
		return -1;
	}
	curr = ftfs->files;



	if(curr)//exists on the server
	{
		struct stat sta = {0};
		if(stat(dst_name,&sta)==-1)
			mkdir(dst_name,perms);
		else
		{
			log_error("ftpd_retrieve_dir: dir already exists. ");
			log_error(dst_name);
			ftp_fs_free(ftfs);
			return -1;	
		}
	}
	else
	{
		log_error("ftpd_retrieve_dir: dir doesn't exist. ");
		ftp_fs_free(ftfs);
		return -1;
	}
	

	while(curr)
	{
		char *src = (char*)malloc(strlen(src_name)+strlen(curr->name)+2);
		char *dst = (char*)malloc(strlen(dst_name)+strlen(curr->name)+2);
		if(!src || !dst)
			break;

		snprintf(src,strlen(src_name)+strlen(curr->name)+2,"%s/%s",src_name,curr->name);
		snprintf(dst,strlen(dst_name)+strlen(curr->name)+2,"%s/%s",dst_name,curr->name);


		if(!strcmp(curr->type,"file"))
			ftpd_retrieve_file(ftps,src,dst,perms);	
	
		if(!strcmp(curr->type,"dir"))
			ftpd_retrieve_dir(ftps,src,dst,perms);
	
		curr = curr->next;
		free(src);
		free(dst);
	}
	
	

	ftp_fs_free(ftfs);
	return 0;
}

int ftpd_retrieve(struct ftp_server *ftps,const char *src_name,const char *dst_name)
{
	//RETR command, returns dir/file over data connection
	//fails if dir exists
	//return value
	//0 -success
	//-1-failed

	if(!ftp_check_server_status(ftps,FTPS_CONTROL_CONNECTED | FTPS_LOGGED_IN,"ftpd_retrieve"))
                return -1;


	struct ftp_file *fifi;
	if(ftpc_ent_info(ftps,src_name,&fifi) == -1)
	{
		log_error("ftpd_retrieve: ent_info failed");
		return -1;
	}	


	if(!strcmp(fifi->type,"file"))
		ftpd_retrieve_file(ftps,src_name,dst_name,fifi->perms);
	if(!strcmp(fifi->type,"dir"))
		ftpd_retrieve_dir(ftps,src_name,dst_name,fifi->perms);

	return 0;

}

int ftpd_store_file(struct ftp_server *ftps,const char *src_name,const char *dst_name)
{
	//STOR command, sends file over data connection
	//return value
	//overwrites if file exists
	//0 -success
	//-1-failed


	

	if(!ftp_check_server_status(ftps,FTPS_CONTROL_CONNECTED | FTPS_LOGGED_IN,"ftpd_store_file"))
                return -1;


	//establish data connection
	if(ftpd_connect(ftps,FTPD_ACTIVE) == -1)
	{
		return -1;
	}
	
	struct ftp_response *fres;

        char *command_str;
        if(ftp_command_str(&command_str,"STOR",dst_name) == -1)
        {
		 return -1;
		 ftpd_disconnect(ftps);
	}
        if(ftp_command(ftps,&fres,command_str)==-1)
        {
                log_error("ftpd_store_file: STOR command failed.");
                ftp_response_free(fres);
		ftpd_disconnect(ftps);
	        return -1;
        }
	 

	if(!(ftps->server_status & FTPS_DATA_CONNECTED))
	{
                log_error("ftpd_retrieve_file: no data connection. ");
                return -1; 
        }


	
	if(fres->code != FTPC_DATA_OPENING)
	{
		ftp_command_failed(fres->code,fres->message,"STOR");
                ftp_response_free(fres);
              
	        return -1;
	}

	ftp_response_free(fres);
	char *buffer;
	int snt_size=8000;
	FILE *fp = NULL;

	int snt = snt_size;
	do
	{
		if(!(buffer = (char *)malloc(snt_size)))
		{
			log_error("ftpd_store_file: malloc() failed.");
			break;
		}

		if(io_read(src_name,&buffer,&snt,&fp) == -1)
		{
			free(buffer);
			break;
		}

		if((ftp_send(ftps,ftps->dc_socket,buffer,snt)) == -1)
		{
			log_error("ftpd_store_file: ftp_send() failed");
			free(buffer);
			ftpd_disconnect(ftps);
			break;
		}
		
		free(buffer);

	}
	while(1);



	close(ftps->dc_socket);

	//close data connection
	if(ftpd_disconnect(ftps) == -1)
		return -1;


	log_message("ftpd_retrieve_file: success. ");	
	return 0;

}
