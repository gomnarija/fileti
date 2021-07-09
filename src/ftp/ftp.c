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
#include "stdlib.h"
#include "string.h"
#include "errno.h"
#include "stdio.h"
#include "unistd.h"
#include "poll.h"


int ftp_server_info(const char *server_name,const char *server_port,struct ftp_server **ftps)
{
	//fills struct ftp_server with server information
	//return value
	// 0 - success
	// -1 - failed



	

	struct addrinfo hints;

	memset(&hints,0,sizeof hints);
	
	hints.ai_family   = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags	  = AI_CANONNAME;

	if((*ftps = (struct ftp_server *)malloc(sizeof(struct ftp_server)))== NULL)
	{	
		log_error("ftp_server_info: malloc() failed.");
		return -1;
	}
	(*ftps)->server_status =  0;
	(*ftps)->cc_socket     = -1;	
	(*ftps)->dc_socket     = -1;
	(*ftps)->dc_info       = NULL;
	(*ftps)->dc_thread     = 0;	

	(*ftps)->dc_info = (struct addrinfo *)malloc(sizeof(struct addrinfo));
	memset((*ftps)->dc_info,0,sizeof(struct addrinfo));
	

	if(getaddrinfo(server_name,server_port,&hints,&((*ftps)->cc_info)))
	{
		log_error("ftp_server_info: getaddrinfo() failed.");
		return -1; 
	}

	return 0;
	
}

void ftp_server_free(struct ftp_server *ftps)
{
	if(ftps->server_status & FTPS_CONTROL_CONNECTED)
		ftpc_disconnect(ftps);
	if(ftps->server_status & FTPS_DATA_CONNECTED)
		ftpd_disconnect(ftps);


	
	freeaddrinfo(ftps->dc_info);
	freeaddrinfo(ftps->cc_info);
	free(ftps);
}

void ftp_response_free(struct ftp_response *fres)
{
	while(fres)
	{
		struct ftp_response *tmp;
		tmp = fres;
		fres = fres->next;
		if(tmp->message)
			free(tmp->message);

		free(tmp);
	}
}
void ftp_fs_free(struct ftp_fs *ftfs)
{
	while(ftfs->files)
	{
	
		struct ftp_file *tmp;
		tmp = ftfs->files;
	
		ftfs->files = ftfs->files->next;	

		if(tmp->name)
			free(tmp->name);
		if(tmp->type)
			free(tmp->type);
		
		if(tmp)
			free(tmp);
	}
	if(ftfs->pwd)
		free(ftfs->pwd);

	if(ftfs)
		free(ftfs);
}



int ftp_connect(struct addrinfo *server_info,int *socket_fd)
{
	//attempts a connection to the ftp server
	//return value:
	// 0 - success
	// -1- failed
	
	if(*socket_fd == -1 && 
		(*socket_fd = socket(server_info->ai_family,
				server_info->ai_socktype | SOCK_NONBLOCK,
					server_info->ai_protocol)) == -1)
	{
		log_error("ftp_connect: socket() failed.");
		return -1;
	}

	
	struct pollfd pfd;
	pfd.fd = *socket_fd;
	pfd.events = POLLOUT;//writing possible
	pfd.revents = 0;
	

	
	if(connect(*socket_fd,server_info->ai_addr,server_info->ai_addrlen) == -1)
	{
		if(errno != EINPROGRESS)
		{
			char buf[16];
			sprintf(buf,"errno: %d",errno);
			log_error("ftp_connect: connect() failed.");
			log_error(buf);
			return -1;
		}
		else
		{
			//wait for the connection
			int rt = poll(&pfd,1,FTP_TIMEOUT);
			if(rt == 0)//timedout
			{
				log_error("ftp_connect: connection timed out.");
				return -1;
			}
			if(rt == -1)
			{
				char buf[16];
				sprintf(buf,"errno: %d",errno);
				log_error("ftp_connect: poll() failed.");
				log_error(buf);
				return -1;
			}
		}
	}
	
	log_message("Ftp server connected.");
	return 0;	
}

int ftp_send(struct ftp_server *ftps,int socket_fd,const char *msg,const int size)
{
	//attempts to send a message over socket
	//return value:
	//0 - succes
	//-1- failed

	if(!(ftps->server_status & FTPS_CONTROL_CONNECTED))
	{
		log_error("ftp_send:ftp_server not connected.");
		return -1;
	}
	
	struct pollfd pfd;
	pfd.fd = socket_fd;
	pfd.events = POLLOUT;//writing possible
	pfd.revents = 0;
	//wait for the connection
	int rt = poll(&pfd,1,FTP_TIMEOUT);
	if(rt == 0)//timedout
	{
		log_error("ftp_send: connection timed out.");
		return -1;
	}
	if(rt == -1)
	{
		char buf[16];
		sprintf(buf,"errno: %d",errno);
		log_error("ftp_send: poll() failed.");
		log_error(buf);
		return -1;
	}



	int flags = 0,bytes_sent;

	if((bytes_sent=send(socket_fd,msg,size,flags))==-1)
	{
		char buf[16];
		sprintf(buf,"errno: %d",errno);
		log_error("ftp_send: send() failed.");
		log_error(buf);
		return -1;

	}
	return 0;

}
int ftp_receive(struct ftp_server *ftps,int socket_fd, char **buffer,int *rc)
{
	//attempts to receive a message over socket
	//return value:
	//0 - succes
	//-1- failed
	//-2- empty buffer, could be done reading

	
	if(!(ftps->server_status & FTPS_CONTROL_CONNECTED))
	{
		log_error("ftp_receive:ftp_server not connected.");
		return -1;
	}

	int flags=0,
		buff_size=0,//total number of bytes received
			rcv_size=*rc == -1 ? 256:*rc,//recv stream size
				bytes_received;//number of bytes received
	struct pollfd pfd;
	pfd.fd = socket_fd;
	pfd.events = POLLIN;//data to be read is in the socket
	pfd.revents = 0;


	*buffer = (char *)malloc(rcv_size);	
	memset(*buffer,0,rcv_size);

	int prt;
	
	//check if there is something to be read
	prt=poll(&pfd,1,FTP_TIMEOUT);
	if(prt == 0 || !(pfd.revents & POLLIN))//timedout or nothing to read
	{

		log_error("ftp_receive: nothing to receive.");
		return -1;
	}
	else if(prt == -1)
	{
		char buf[16];
		sprintf(buf,"errno: %d",errno);
		log_error("ftp_receive: poll() failed.");
		log_error(buf);
		return -1;
	}

	

	do
	{	

		if((bytes_received=recv(socket_fd,*buffer+buff_size,rcv_size,flags))==-1)
		{

			if(errno == 11)//EAGAIN, temporary busy, try again
				continue;

			char buf[16];
			sprintf(buf,"errno: %d",errno);
			log_error("ftp_receive: recv() failed.");
			log_error(buf);
			return -1;
		}

		buff_size += bytes_received;

		if((*buffer = (char*) realloc(*buffer,buff_size+rcv_size))==NULL)//expand buffer for
									//next recv
		{
			log_error("ftp_receive: realloc() failed.");
			free(*buffer);
			return -1;
		}


	}
	while(bytes_received > 0 &&
		(*rc == -1 || buff_size <= *rc));

	*rc = buff_size;


	if(buff_size != 0 && (*buffer = (char*) realloc(*buffer,buff_size))==NULL)//scale buffer down
	{
			free(*buffer);
			return -1;
	}
	else if(buff_size == 0)
	{
		free(*buffer);
		return -2;
	}
	//log_message(buff);
	
	return 0;



}


int ftp_command(struct ftp_server *ftps,struct ftp_response **fres,char *command)
{
	//sends command to ftp_server,and receives response
	//return value:
	//0 - succes
	//-1- failed

	
	char *response;//raw response string, returned from ftp_receive
	struct ftp_response *curr_res;

	*fres = NULL; 
	int response_size=-1;	

	if((curr_res = (struct ftp_response*)malloc(sizeof(struct ftp_response))) == NULL)
	{
		log_error("ftp_command: malloc failed.");
		if(command)free(command);
		return -1;
	} 
	
	*fres = curr_res;//head
	curr_res->message = NULL;
	curr_res->next = NULL;


	if(ftp_send(ftps,ftps->cc_socket,command,strlen(command)) == -1)
	{
		log_error("ftp_command: ftp_send() failed.");
		if(command)free(command);
		return -1;
	
	}
	command[strlen(command)-1]=' ';//terminate new line
	log_raw(">",0);	
	log_raw(command,1);
		
	if(command)free(command);

	
	int rcv;
	if((rcv = ftp_receive(ftps,ftps->cc_socket,&response,&response_size)) == -1)
	{
	
		log_error("ftp_command: ftp_receive() failed.");
		return -1;
	}
	else if(rcv == -2)
	{
		log_warning("ftp_command: ftp_receive() read nothing. ");
		return -1;
	}

	log_raw("<",0);	
	log_raw(response,1);
		
	//first 3 characters of the response are 
	// digits representing FTP response code,rest is the response text

	//if the 4th character is '-'response is multi-line, 
	//and will continue until the first line where
	//the 4th character is space ' '

	//terminate response
	if((response = (char*) realloc(response,response_size+1))==NULL)
	{	
		log_error("ftp_command: realloc() failed.");
		return -1;
	}
	response[response_size++] = '\0';



	char *curr_line,
		*new_line;

	curr_line = response;
	
	if((new_line=strchr(curr_line,'\n'))==NULL)//new_line at first instance of '\n'
	{
		log_error("ftp_command: response not terminated");
		return -1;
	}



	do//line by line, separating code and text
	{
	
		*new_line = '\0';
		

		if((curr_res->message = (char *)malloc(new_line-curr_line))==NULL)
		{
			log_error("ftp_command: malloc() failed.");
			free(response);
			return -1;
		}
	
		curr_res->code = strtol(curr_line,NULL,10);	
		
		if(curr_res->code < 100 || curr_res->code > 999)
		{
			log_error("ftp_command: wrong code format.");
			free(response);
			return -1;
		}

		snprintf(curr_res->message,new_line-curr_line-4,"%s",curr_line+4);

		curr_line = new_line + 1;
		new_line=strchr(curr_line,'\n');

		if(new_line != NULL)
		{
			if((curr_res->next = (struct ftp_response*)malloc(sizeof(struct ftp_response))) == NULL) 
			{
				log_error("ftp_command: malloc () failed.");
				free(response);
				return -1;
			}
			curr_res = curr_res->next;
			curr_res->message = NULL;	
			curr_res->next    = NULL;
		
		}
		else//no more '/n' 
		{
			break;
		}
	
	}while(1);
	
	free(response);
	
	return 0;
}


int ftp_command_str(char **cstr,const char *command,const char *arguments)
{
	//creates command string
	//return value
	//0  - success
	//-1 - failed


	*cstr = (char *)malloc(strlen(command)+strlen(arguments)+4);

	if(!(*cstr))
		return -1;

	if(strlen(arguments)!=0)
		snprintf(*cstr,strlen(command)+strlen(arguments)+4,"%s %s\r\n",command,arguments);
	else
	{
		snprintf(*cstr,strlen(command)+3,"%s\r\n",command);
	}
	return 0;

}

void *ftp_accept(void *arg)
{
	//new thread
	//accepts pending data connection, if something like that exists
	//return value:
	//0-success
	//-1-failed
	//probably should do something with this idk :/	
	socklen_t addrlen = (socklen_t)sizeof (struct sockaddr);


	struct ftp_server *ftps = (struct ftp_server*)arg;

	struct pollfd pfd;
	pfd.fd = ftps->dc_socket;
	pfd.events = POLLIN;//reading possible
	pfd.revents = 0;
	


	//wait for socket
	int rt = poll(&pfd,1,1000);
	if(rt == 0)
	{
		log_error("ftp_accept: timedout\n");
		close(ftps->dc_socket);
		ftps->dc_socket = -1;
		return NULL;
	}
	else if (rt==-1)
	{
		log_error("ftp_accept: poll failed\n");
		close(ftps->dc_socket);
		ftps->dc_socket = -1;
		return NULL;
	}



	int nfd;
	nfd = accept(ftps->dc_socket,(struct sockaddr*)ftps->dc_info->ai_addr,&addrlen);
	if(nfd == -1)
	{
		log_error("ftp_accept: accept failed. ");
		close(ftps->dc_socket);
		ftps->dc_socket = -1;
		return NULL;
	}
	else
	{
		close(ftps->dc_socket);
		ftps->dc_socket = nfd;	
	}
		
	ftps->server_status |= FTPS_DATA_CONNECTED;	
	log_message("ftp_accept: server connection accepted. ");

		
	return NULL;

}


int ftp_check_server_status(struct ftp_server *ftps,const int status,const char *caller)
{

	if(FTPS_CONTROL_CONNECTED & status && !(FTPS_CONTROL_CONNECTED & ftps->server_status))
	{
		log_error(caller);
		log_error("server status: no control connection. ");
	}
	if(FTPS_DATA_CONNECTED & status && !(FTPS_DATA_CONNECTED & ftps->server_status))
	{
		log_error(caller);
		log_error("server status: no data connection. ");
	}
	if(FTPS_LOGGED_IN & status && !(FTPS_LOGGED_IN & ftps->server_status))
	{
		log_error(caller);
		log_error("server status: not logged in. ");
	}
	return (ftps->server_status&status)==status;
}

void ftp_command_failed(const int code,char *message,const char *command)
{

	char buf[10];
        snprintf(buf,10,"code: %d",code);
        log_error("command failed:");
        log_error(command);
        log_error(buf);
        log_error(message);
}

void ftp_fs_select(struct ftp_fs *ftfs,struct ftp_file **fifi,int *sel)
{

	if(!ftfs)
	{
		*fifi = NULL;
		*sel  = 0;
		return;
	}
	if(*sel < 0)
		*sel = 0;


	int curr=0;
	*fifi = ftfs->files;
	if(!*fifi)
	{
		*sel = 0;
		return;
	}
	while((*fifi)->next)
	{
		if(curr==*sel)
			return;
		
		curr++;
		*fifi = (*fifi)->next;
	}
	*sel = curr;
		
}
