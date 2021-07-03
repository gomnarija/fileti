#include "ftp_control.h"
#include "../utils/log.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "unistd.h"





int ftpc_user(struct ftp_server *ftps,const char *user_name)
{
	//attempts USER command
	//return value:
	//0 - success
	//-1 - failed
	
	if(!(ftps->server_status & FTPS_CONTROL_CONNECTED))
	{
		log_error("ftpc_user:ftp_server not connected.");
		return -1;
	}
	
	struct ftp_response *fres;
	
	char *command_str;
	if(ftp_command_str(&command_str,"USER",user_name) == -1)
		return -1;

	if(ftp_command(ftps,&fres,command_str)==-1)
	{
		log_error("ftpc_user: USER command failed.");
		ftp_response_free(fres);
		return -1;
	}


	if(fres->code == FTPC_USER_OK)
	{
		log_message("ftpc_user: success.");	
		ftp_response_free(fres);
		return 0;
	}
	else
	{
		char buf[16];
		snprintf(buf,16,"code:%d",fres->code);
		log_error("ftpc_user: command USER failed.");
		log_error(buf);
		ftp_response_free(fres);
		return -1;
	}

}

int ftpc_password(struct ftp_server *ftps,const char *password)
{
	//attempts PASS command
	//return value:
	//0  - success
	//-1 - failed 
	//-2 - wrong credentials
	
	if(!(ftps->server_status & FTPS_CONTROL_CONNECTED))
	{
		log_error("ftpc_password:ftp_server not connected.");
		return -1;
	}
	
	struct ftp_response *fres;
	
	char *command_str;

	if(ftp_command_str(&command_str,"PASS",password) == -1)
		return -1;


	if(ftp_command(ftps,&fres,command_str)==-1)
	{
		log_error("ftpc_password: PASS command failed.");
		ftp_response_free(fres);
		return -1;
	}


	if(fres->code == FTPC_LOGGED_IN)
	{
		log_message("ftpc_password: success.");
		log_message("Logged in.");	
		ftps->server_status |= FTPS_LOGGED_IN;		
		ftp_response_free(fres);
		return 0;
	}
	else if(fres->code == FTPC_INVALID_CRED 
		|| fres->code == FTPC_NOT_LOGGED)
	{
		log_error("ftpc_password: wrong credentials");
		ftp_response_free(fres);
		return -2;
	}
	else
	{
		char buf[16];
		snprintf(buf,16,"code:%d",fres->code);
		log_error("ftpc_password: command PASS failed.");
		log_error(buf);
		ftp_response_free(fres);
		return -1;
	}

}


int ftp_login(struct ftp_server *ftps,const char *user_name,const char *password)
{
	//attempts ftp login by sending USER and PWD commands
	//return value:
	//0 - success
	//-2 - wrong user or password 
	//-1 - failed
	
	if(!(ftps->server_status & FTPS_CONTROL_CONNECTED))
	{
		log_error("ftp_login:ftp_server not connected.");
		return -1;
	}

	if(ftpc_user(ftps,user_name)==-1)
	{
		return -1;
	}

	return ftpc_password(ftps,password);


}

int ftpc_passive(struct ftp_server *ftps)
{
	//attempts to initiate passive connection,
	//sends PASV command, and waits for port form server
	//return value
	//0 - success
	//-1- failed

	if(!(ftps->server_status & FTPS_CONTROL_CONNECTED))
	{
		log_error("ftpc_passive:ftp_server not connected.");
		return -1;
	}

	if(!(ftps->server_status & FTPS_LOGGED_IN))
	{
		log_error("ftpc_passive:user not logged in.");
		return -1;
	}


	
	char *command_str;
	struct ftp_response *fres;
	
	if(ftp_command_str(&command_str,"PASV","") == -1)
		return -1;

	if(ftp_command(ftps,&fres,command_str)==-1)
	{
		log_error("ftpc_passive: PASV command failed.");
		ftp_response_free(fres);
		return -1;
	}



	if(fres->code != FTPC_PASSIVE_MODE)
	{
		
	
		char buf[16];
		snprintf(buf,16,"code:%d",fres->code);
		log_error("ftpc_passive: command PASV failed.");
		log_error(buf);
		ftp_response_free(fres);
		return -1;

	}
	
	//get ip and port from response message
	//format:
	//some text (ip1,ip2,ip3,ip4,port1,port2)

	char *endp,
		*startp;
	int ip[4],port[2];	

	startp = fres->message;

	while(!(*startp >= '1' && *startp <= '9'))//place startp at first digit
	{
		startp++;
		if(startp-fres->message > strlen(fres->message))
		{
			log_error("ftpc_passive: invalid response message.");
			ftp_response_free(fres);
			return -1;
		}
	}

	ip[0] = strtol(startp,&endp,10);//finds number, places endp after number
	ip[1] = strtol(endp+1,&endp,10);//endp+1 to avoid ','
	ip[2] = strtol(endp+1,&endp,10);
	ip[3] = strtol(endp+1,&endp,10);

	port[0] = strtol(endp+1,&endp,10);
	port[1] = strtol(endp+1,&endp,10);

	char ip_str[64],port_str[32];
	snprintf(ip_str,64,"%d.%d.%d.%d",ip[0],ip[1],ip[2],ip[3]);
	snprintf(port_str,32,"%d",(port[0]*256)+port[1]);


	struct addrinfo hints;
	memset(&hints,0,sizeof hints);
	
	hints.ai_family   = AF_INET;
	hints.ai_protocol = IPPROTO_TCP;
	
	if(getaddrinfo(ip_str,port_str,&hints,&(ftps->dc_info)))
	{
		log_error("ftpc_passive: getaddrinfo() failed.");
		ftp_response_free(fres);
		return -1; 
	}
	
	log_message("ftpc_passive: success.");
	ftp_response_free(fres);
	return 0;
}
	
	


int ftpc_pwd(struct ftp_server *ftps,struct ftp_fs **ftfs)
{
	//attemps PWD command,
	//creates ftp_fs struct
	//return valie
	//0 - success
	//-1- failed

	if(!(ftps->server_status & FTPS_CONTROL_CONNECTED))
	{
		log_error("ftpc_pwd:ftp_server not connected.");
		return -1;
	}

	if(!(ftps->server_status & FTPS_LOGGED_IN))
	{
		log_error("ftpc_pwd:user not logged in.");
		return -1;
	}
	
	if((*ftfs = (struct ftp_fs *)malloc(sizeof(struct ftp_fs)))==NULL)
	{	
		log_error("ftpc_pwd:malloc() failed.");
		return -1;
	}
	(*ftfs)->files = NULL;

	char *command_str;
	struct ftp_response *fres;
	
	if(ftp_command_str(&command_str,"PWD","") == -1)
		return -1;

	if(ftp_command(ftps,&fres,command_str)==-1)
	{
		log_error("ftpc_pwd: PWD command failed.");
		ftp_response_free(fres);
		return -1;
	}

	if(fres->code != FTPC_PATH_NAME)
	{		
	
		char buf[16];
                snprintf(buf,16,"code:%d",fres->code);
                log_error("ftpc_pwd: command PWD failed.");
                log_error(buf);
                ftp_response_free(fres);
                return -1;
	}


	//pwd format 
	//"/pwd" some text

	char *startp,*endp;
	startp = fres->message;
	while(*startp!='"')
	{
		startp++;
		if(startp-fres->message > strlen(fres->message))
		{
			log_error("ftpc_pwd: response message is in wrong format.");
			ftp_response_free(fres);
			return -1;
		}
	}

		
	//	"/pwd"
	//	s    e 


	endp = startp+1;
	while(*endp!='"')
	{
		endp++;
		if(endp-fres->message > strlen(fres->message))
		{
			log_error("ftpc_pwd: response message is in wrong format.");
			ftp_response_free(fres);
			return -1;
		}
	}
	
	*endp = '\0';

	if(((*ftfs)->pwd = (char *)malloc(strlen(startp)))==NULL)
	{
		log_error("ftpc_pwd: malloc() failed");
		ftp_response_free(fres);
		return -1;
	}

	snprintf((*ftfs)->pwd,strlen(startp),"%s",startp+1);

	log_message("ftpc_pwd: success.");
	log_message((*ftfs)->pwd);

	ftp_response_free(fres);

	return 0;


}

int ftpc_type(struct ftp_server *ftps,const int type)
{
	//attemps TYPE command,
	//tells the server in what format to transmit files
	//return value
	//0 - success
	//-1- failed

	if(!(ftps->server_status & FTPS_CONTROL_CONNECTED))
	{
		log_error("ftpc_type:ftp_server not connected.");
		return -1;
	}

	if(!(ftps->server_status & FTPS_LOGGED_IN))
	{
		log_error("ftpc_type:user not logged in.");
		return -1;
	}

	char *command_str,
			type_str[32]={'\0'};
	struct ftp_response *fres;
	
	switch(type)
	{
		case FTP_TYPE_ASCII:
			snprintf(type_str,32,"%s","A");
			break;
	
		case FTP_TYPE_BINARY:
			snprintf(type_str,32,"%s","I");
			break;
		default:	
			log_error("ftcp_type: non existant type.");
			return -1;
	}

	if(ftp_command_str(&command_str,"TYPE",type_str) == -1)
		return -1;

	if(ftp_command(ftps,&fres,command_str)==-1)
	{
		log_error("ftpc_type: TYPE command failed.");
		ftp_response_free(fres);
		return -1;
	}
	
        log_message("ftpc_type: command TYPE sent.");
	
	

	ftp_response_free(fres);

	return 0;
}

int ftpc_connect(struct ftp_server *ftps)
{
	//attempts control connection,
	//return value
	//0 - success
	//-1- failed


	if(ftps->server_status & FTPS_CONTROL_CONNECTED)
	{
		log_warning("ftpc_connect:ftp_server already connected.");
		return -1;
	}
	
	if(ftp_connect(ftps->cc_info,&(ftps->cc_socket)) == -1)
	{
		log_error("ftpc_connect:connection failed.");
		return -1;
	}

	ftps->server_status |= FTPS_CONTROL_CONNECTED;

	//welcome message
	char *buff;
	if(ftp_receive(ftps,ftps->cc_socket,&buff,FTPT_CONTROL) == -1)
	{	
		log_error("ftpc_connect:ftp_receive failed.");
		return -1;
	}
	log_message(buff);
	free(buff);


	return 0;
}

int ftpc_disconnect(struct ftp_server *ftps)
{
	//stops control connection,
	//return value
	//0 - success
	//-1- failed


	if(!(ftps->server_status & FTPS_CONTROL_CONNECTED))
	{
		log_error("ftpc_disconnect:ftp_server not connected.");
		return -1;
	}

	char *command_str;
	struct ftp_response *fres;
	
	if(ftp_command_str(&command_str,"QUIT","") == -1)
		return -1;

	if(ftp_command(ftps,&fres,command_str)==-1)
	{
		log_error("ftpc_disconnect: QUIT command failed.");
		ftp_response_free(fres);
		return -1;
	}

	if(fres->code != FTPC_CONTROL_CLOSING)
	{		
	
		char buf[16];
                snprintf(buf,16,"code:%d",fres->code);
                log_error("ftpc_disconnect: command QUIT failed.");
		log_error("possible file transfer in progress.");
                log_error(buf);
                ftp_response_free(fres);
                return -1;
	}

	close(ftps->cc_socket);
	ftps->cc_socket = -1;
	ftps->server_status ^= FTPS_CONTROL_CONNECTED;	
		
	
	log_message("ftpc_disconnect: server disconnected. ");
	ftp_response_free(fres);
	return 0;
}


int active_str(char **buffer,char *ip,int port)
{
	//generates argument str for PORT command
	//return value:
	//0-success
	//-1-failed

	//format
	//(ip1,ip2,ip3,ip4,port1,port2)
	//port = (port1 * 256) + port2

	
	char *p;
	p = ip;
	while((p-ip)<strlen(ip))
	{
		if(*p=='.')
			*p=',';
		p++;
	}


	*buffer = (char *)malloc(strlen(ip) + 3 + 6);
	snprintf(*buffer,strlen(ip)+3+6,"%s,%d,%d",ip,port >> 8,port-((port>>8)*256));
	return 0;
}

int ftpc_active(struct ftp_server *ftps)
{
	//attempts to initiate active connection,
	//sends PORT command with ip and port information
	//return valie
	//0 - success
	//-1- failed

	if(!(ftps->server_status & FTPS_CONTROL_CONNECTED))
	{
		log_error("ftpc_active:ftp_server not connected.");
		return -1;
	}

	if(!(ftps->server_status & FTPS_LOGGED_IN))
	{
		log_error("ftpc_active:user not logged in.");
		return -1;
	}


	struct sockaddr_in *sok;
	struct addrinfo hints, *local;//temp, for creating socket
	memset(&hints,0,sizeof hints);
	
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	
	////////////////////////////////////////////////////////////////////
	if(getaddrinfo(NULL,"0",&hints,&local) == -1)
	{
		
		log_error("ftpc_active: getaddrinfo() failed");
		return -1;
	}
	
	//listening socket on random port
	if((ftps->dc_socket = socket(local->ai_family,
					local->ai_socktype | SOCK_NONBLOCK,
						local->ai_protocol)) == -1)

	{
		log_error("ftpc_active: socket() failed");
		return -1;
	}

	//get random avaliable port
	if(bind(ftps->dc_socket,local->ai_addr,local->ai_addrlen) == -1 ||
		getsockname(ftps->dc_socket,local->ai_addr,&(local->ai_addrlen)) == -1)
	{
		log_error("ftpc_active: binding failed");
		return -1;
	}
	
	//start listening for the server connection
	if(listen(ftps->dc_socket,20) == -1)
	{
		log_error("ftpc_active: listen() failed");
		return -1;
	}
	
	sok = (struct sockaddr_in *)local->ai_addr;
	
	
	////////////////////////////////////////
	

	char host[256];
	struct hostent *hent;

	//get local ip
	if(gethostname(host,sizeof host) == -1)
	{
		log_error("ftpc_active: gethostname() failed");
		return -1;
	}
	
	hent = gethostbyname(host);
	if(!hent)
	{
		log_error("ftpc_active: gethostbyname() failed. ");
		return -1;
	}

	
	////////////////////////////////////////////////////////

	char *ip;
	int port;
	
	ip = inet_ntoa(*((struct in_addr*)hent->h_addr_list[0]));
	port = htons(sok->sin_port);
	
	
	char *command_str;
	struct ftp_response *fres;

	char *arg_buff;
	
	if(active_str(&arg_buff,ip,port) == -1)
	{
		log_error("ftpc_active: argument string creation failed. ");
		return -1;
	}
	

	if(ftp_command_str(&command_str,"PORT",arg_buff) == -1)
		return -1;

	if(ftp_command(ftps,&fres,command_str)==-1)
	{
		log_error("ftpc_active: PORT command failed.");
		ftp_response_free(fres);
		return -1;
	}
	

	if(fres->code != FTPC_COMMAND_OK)
	{
		
	
		char buf[16];
		snprintf(buf,16,"code:%d",fres->code);
		log_error("ftpc_active: command PORT failed.");
		log_error(buf);
		ftp_response_free(fres);
		return -1;

	}
	
	ftp_response_free(fres);
	return 0;

}

