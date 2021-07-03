#include "ftp.h"
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
		close(ftps->cc_socket);
	if(ftps->server_status & FTPS_DATA_CONNECTED)
		close(ftps->dc_socket);


	
	freeaddrinfo(ftps->dc_info);
	freeaddrinfo(ftps->cc_info);
	free(ftps);
}

void ftp_response_free(struct ftp_response *fres)
{
	if(fres->message)
		free(fres->message);

	if(fres)
		free(fres);

	return;

	while(fres)
	{
		struct ftp_response *tmp;
		tmp = fres;
		fres = fres->next;
		free(tmp);
	}
}
void ftp_fs_free(struct ftp_fs *ftfs)
{
	struct ftp_file *fifi;
	fifi = ftfs->files;
	while(fifi->next)
	{
		if(fifi->next->name)
			free(fifi->next->name);
		if(fifi->next->type)
			free(fifi->next->type);
		fifi = fifi->next;
	
	}
	if(fifi)
		free(fifi);
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

int ftp_send(struct ftp_server *ftps,int socket_fd,const char *msg)
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
	

	int flags = 0,bytes_sent;

	if((bytes_sent=send(socket_fd,msg,strlen(msg),flags))==-1)
	{
		char buf[16];
		sprintf(buf,"errno: %d",errno);
		log_error("ftp_send: send() failed.");
		log_error(buf);
		return -1;

	}
	//log_message(msg);
	return 0;

}
int ftp_receive(struct ftp_server *ftps,int socket_fd, char **buffer,int *rc)
{
	//attempts to receive a message over socket
	//return value:
	//0 - succes
	//-1- failed


	
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
			return -1;
		}


	}
	while(bytes_received > 0 &&
		(*rc == -1 || buff_size <= *rc));

	*rc = buff_size;

	if((*buffer = (char*) realloc(*buffer,buff_size))==NULL)//scale buffer down
	{
			log_error("ftp_receive: realloc() scaling down failed.");
			free(*buffer);
			return -1;
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

	if(!(ftps->server_status & FTPS_CONTROL_CONNECTED))
	{
		log_error("ftp_command:ftp_server not connected.");
		if(command)free(command);
		return -1;
	}

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



	if(ftp_send(ftps,ftps->cc_socket,command) == -1)
	{
		log_error("ftp_command: ftp_send() failed.");
		if(command)free(command);
		return -1;
	
	}	
	if(command)free(command);

	
	if(ftp_receive(ftps,ftps->cc_socket,&response,&response_size) == -1)
	{
	
		log_error("ftp_command: ftp_receive() failed.");
		return -1;
	}

	
	//first 3 characters of the response are 
	// digits representing FTP response code,rest is the response text

	//if the 4th character is '-'response is multi-line, 
	//and will continue until the first line where
	//the 4th character is space ' '


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


	*cstr = (char *)malloc(strlen(command)+strlen(arguments)+3);

	if(!(*cstr))
		return -1;

	snprintf(*cstr,strlen(command)+strlen(arguments)+3,"%s %s\n",command,arguments);

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
		return NULL;
	}
	else if (rt==-1)
	{
		log_error("ftp_accept: poll failed\n");
		return NULL;
	}



	int nfd;
	nfd = accept(ftps->dc_socket,(struct sockaddr*)ftps->dc_info->ai_addr,&addrlen);
	if(nfd == -1)
	{
		log_error("ftp_accept: accept failed. ");
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
