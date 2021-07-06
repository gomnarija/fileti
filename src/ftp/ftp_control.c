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
	
	if(!ftp_check_server_status(ftps,FTPS_CONTROL_CONNECTED,"ftpc_user"))
		return -1;
	
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
		ftp_command_failed(fres->code,fres->message,"USER");
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


	if(!ftp_check_server_status(ftps,FTPS_CONTROL_CONNECTED,"ftpc_password"))
		return -1;
	
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
		ftp_command_failed(fres->code,fres->message,"PASS");
		ftp_response_free(fres);
		return -1;
	}

}


int ftpc_login(struct ftp_server *ftps,const char *user_name,const char *password)
{
	//attempts ftp login by sending USER and PWD commands
	//return value:
	//0 - success
	//-2 - wrong user or password 
	//-1 - failed
	
	if(!ftp_check_server_status(ftps,FTPS_CONTROL_CONNECTED,"ftpc_login"))
		return -1;
	
	
	if(ftpc_user(ftps,user_name)==-1)
		return -1;

	return ftpc_password(ftps,password);


}

int ftpc_passive(struct ftp_server *ftps)
{
	//attempts to initiate passive connection,
	//sends PASV command, and waits for port form server
	//return value
	//0 - success
	//-1- failed

	
	if(!ftp_check_server_status(ftps,FTPS_CONTROL_CONNECTED | 
						FTPS_LOGGED_IN,"ftpc_passive"))
		return -1;
	

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
		
	
		ftp_command_failed(fres->code,fres->message,"PASV");
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
	

	if(ftps->dc_info)
		freeaddrinfo(ftps->dc_info);

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
	//return value
	//0 - success
	//-1- failed

	if(!ftp_check_server_status(ftps,FTPS_CONTROL_CONNECTED | 
						FTPS_LOGGED_IN,"ftpc_pwd"))
		return -1;
	
	
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
	
		ftp_command_failed(fres->code,fres->message,"PWD");
                ftp_response_free(fres);
                return -1;
	}


	//pwd format 
	//"/wd" some text

	char *startp,*endp;
	startp = strchr(fres->message,'"');
	endp = strchr(startp+1,'"');

	
	//	"/wd"
	//	s   e 

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

	if(!ftp_check_server_status(ftps,FTPS_CONTROL_CONNECTED | 
						FTPS_LOGGED_IN,"ftpc_pwd"))
		return -1;


	char *command_str,
			type_str[2];
	struct ftp_response *fres;
	
	switch(type)
	{
		case FTP_TYPE_ASCII:
			snprintf(type_str,2,"%s","A");
			break;
	
		case FTP_TYPE_BINARY:
			snprintf(type_str,2,"%s","I");
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

	if(fres->code != FTPC_COMMAND_OK)
	{		
	
		ftp_command_failed(fres->code,fres->message,"TYPE");
                ftp_response_free(fres);
                return -1;
	}

        log_message("ftpc_type: success.");
	log_message(fres->message);
	

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
	int response_size = -1;
	int rcv;
	if((rcv=ftp_receive(ftps,ftps->cc_socket,&buff,&response_size)) == -1)
	{	
		log_error("ftpc_connect:ftp_receive failed.");
		return -1;
	}
	else if(rcv==-2)
	{
		log_warning("ftpc_connect: ftp_receive read nothing. ");
		return -1;
	}

	buff[response_size-1]='\0';
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
	
		ftp_command_failed(fres->code,fres->message,"QUIT");
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

	if(!ftp_check_server_status(ftps,FTPS_CONTROL_CONNECTED | 
						FTPS_LOGGED_IN,"ftpc_active"))
		return -1;


	struct sockaddr_in sok;
	memset(&sok,0,sizeof sok);
	sok.sin_port = htons(0);//0 port will be bound to random
	socklen_t ssize = sizeof sok;
	
	//listening socket on random port
	if((ftps->dc_socket = socket(AF_INET,
					 SOCK_STREAM | SOCK_NONBLOCK,
						IPPROTO_TCP)) == -1)
	{
		log_error("ftpc_active: socket() failed");
		return -1;
	}

	//get random avaliable port
	if(bind(ftps->dc_socket,(struct sockaddr*)&sok,ssize) == -1 ||
		getsockname(ftps->dc_socket,(struct sockaddr*)&sok,&ssize) == -1)
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

	
	char *ip;
	int port;
	
	ip = inet_ntoa(*((struct in_addr*)hent->h_addr_list[0]));
	port = htons(sok.sin_port);
	

	
	

	char *command_str;
	struct ftp_response *fres;

	char *arg_buff;
	
	if(active_str(&arg_buff,ip,port) == -1)
	{
		log_error("ftpc_active: argument string creation failed. ");
		free(arg_buff);
		return -1;
	}
	

	if(ftp_command_str(&command_str,"PORT",arg_buff) == -1)
		return -1;

	if(ftp_command(ftps,&fres,command_str)==-1)
	{
		log_error("ftpc_active: PORT command failed.");
		ftp_response_free(fres);
		free(arg_buff);
		return -1;
	}
	

	if(fres->code != FTPC_COMMAND_OK)
	{
		
	
		ftp_command_failed(fres->code,fres->message,"PORT");
		ftp_response_free(fres);
		free(arg_buff);
		return -1;

	}
	
	free(arg_buff);
	ftp_response_free(fres);
	return 0;

}

int ftpc_mode(struct ftp_server *ftps,const int type)
{
	//attemps MODE command,
	//tells the server in what mode to transmit files
	//return value
	//0 - success
	//-1- failed

	if(!ftp_check_server_status(ftps,FTPS_CONTROL_CONNECTED | 
						FTPS_LOGGED_IN,"ftpc_mode"))
		return -1;


	char *command_str,
			mode_str[2]={'\0'};
	struct ftp_response *fres;
	
	switch(type)
	{
		case FTP_MODE_STREAM:
			snprintf(mode_str,2,"%s","S");
			break;
	
		case FTP_MODE_BLOCK:
			snprintf(mode_str,2,"%s","B");
			break;
		case FTP_MODE_COMPRESSED:
			snprintf(mode_str,2,"%s","C");
			break;
		default:	
			log_error("ftcp_mode: non existant mode.");
			return -1;
	}

	if(ftp_command_str(&command_str,"MODE",mode_str) == -1)
		return -1;

	if(ftp_command(ftps,&fres,command_str)==-1)
	{
		log_error("ftpc_mode: MODE command failed.");
		ftp_response_free(fres);
		return -1;
	}
	
	if(fres->code != FTPC_COMMAND_OK)
	{		
	
		ftp_command_failed(fres->code,fres->message,"MODE");
                ftp_response_free(fres);
                return -1;
	}


        log_message("ftpc_mode: success.");
	log_message(fres->message);

	ftp_response_free(fres);

	return 0;
}


int ftpc_cwd(struct ftp_server *ftps,const char *wd)
{
	//attemps CWD command,
	//return value
	//0 - success
	//-1- failed
	

	if(!ftp_check_server_status(ftps,FTPS_CONTROL_CONNECTED | 
						FTPS_LOGGED_IN,"ftpc_cwd"))
		return -1;

	
	char *command_str;
	struct ftp_response *fres;
	
	if(ftp_command_str(&command_str,"CWD",wd) == -1)
		return -1;

	if(ftp_command(ftps,&fres,command_str)==-1)
	{
		log_error("ftpc_cwd: CWD command failed.");
		ftp_response_free(fres);
		return -1;
	}

	if(fres->code != FTPC_FILE_OK)
	{		
	
		ftp_command_failed(fres->code,fres->message,"CWD");
                ftp_response_free(fres);
                return -1;
	}


	log_message("ftpc_cwd: success.");
	log_message(fres->message);

	ftp_response_free(fres);

	return 0;


}

int ftpc_mkdir(struct ftp_server *ftps,const char *dir)
{
	//attemps MKD command,
	//return value
	//0 - success
	//-1- failed
	

	if(!ftp_check_server_status(ftps,FTPS_CONTROL_CONNECTED | 
						FTPS_LOGGED_IN,"ftpc_mkdir"))
		return -1;

	
	char *command_str;
	struct ftp_response *fres;
	
	if(ftp_command_str(&command_str,"MKD",dir) == -1)
		return -1;

	if(ftp_command(ftps,&fres,command_str)==-1)
	{
		log_error("ftpc_mkdir: MKD command failed.");
		ftp_response_free(fres);
		return -1;
	}

	if(fres->code != FTPC_PATH_NAME)
	{		
	
		ftp_command_failed(fres->code,fres->message,"MKD");
                ftp_response_free(fres);
                return -1;
	}


	log_message("ftpc_mkdir: success.");

	ftp_response_free(fres);

	return 0;

}

int ftpc_rm(struct ftp_server *ftps,const char *file)
{
	//attemps DELE command,
	//return value
	//0 - success
	//-1- failed
	

	if(!ftp_check_server_status(ftps,FTPS_CONTROL_CONNECTED | 
						FTPS_LOGGED_IN,"ftpc_rm"))
		return -1;

	
	char *command_str;
	struct ftp_response *fres;
	
	if(ftp_command_str(&command_str,"DELE",file) == -1)
		return -1;

	if(ftp_command(ftps,&fres,command_str)==-1)
	{
		log_error("ftpc_rm: DELE command failed.");
		ftp_response_free(fres);
		return -1;
	}

	if(fres->code != FTPC_FILE_OK)
	{		
	
		ftp_command_failed(fres->code,fres->message,"DELE");
                ftp_response_free(fres);
                return -1;
	}


	log_message("ftpc_rm: success.");

	ftp_response_free(fres);

	return 0;


}

int ftpc_rmdir(struct ftp_server *ftps,const char *dir)
{
	//attemps RMD command,
	//return value
	//0 - success
	//-1- failed
	

	if(!ftp_check_server_status(ftps,FTPS_CONTROL_CONNECTED | 
						FTPS_LOGGED_IN,"ftpc_rmdir"))
		return -1;

	
	char *command_str;
	struct ftp_response *fres;
	
	if(ftp_command_str(&command_str,"RMD",dir) == -1)
		return -1;

	if(ftp_command(ftps,&fres,command_str)==-1)
	{
		log_error("ftpc_rmdir: RMD command failed.");
		ftp_response_free(fres);
		return -1;
	}

	if(fres->code != FTPC_FILE_OK)
	{		
	
		ftp_command_failed(fres->code,fres->message,"RMD");
                ftp_response_free(fres);
                return -1;
	}


	log_message("ftpc_rmdir: success.");

	ftp_response_free(fres);

	return 0;


}
int ftpc_structure(struct ftp_server *ftps,const int stru)
{
	//attemps STRU command,
	//return value
	//0 - success
	//-1- failed

	if(!ftp_check_server_status(ftps,FTPS_CONTROL_CONNECTED | 
						FTPS_LOGGED_IN,"ftpc_structure"))
		return -1;


	char *command_str,
			stru_str[2]={'\0'};
	struct ftp_response *fres;
	
	switch(stru)
	{
		case FTP_STRUCT_FILE:
			snprintf(stru_str,2,"%s","F");
			break;
	
		case FTP_STRUCT_RECORD:
			snprintf(stru_str,2,"%s","R");
			break;
		case FTP_STRUCT_PAGE:
			snprintf(stru_str,2,"%s","P");
			break;
		default:	
			log_error("ftcp_structure: non existant mode.");
			return -1;
	}

	if(ftp_command_str(&command_str,"STRU",stru_str) == -1)
		return -1;

	if(ftp_command(ftps,&fres,command_str)==-1)
	{
		log_error("ftpc_structure: STRU command failed.");
		ftp_response_free(fres);
		return -1;
	}
	
	if(fres->code != FTPC_COMMAND_OK)
	{		
	
		ftp_command_failed(fres->code,fres->message,"STRU");
                ftp_response_free(fres);
                return -1;
	}


        log_message("ftpc_structure: success.");
	log_message(fres->message);

	ftp_response_free(fres);

	return 0;

}

int ftpc_cdup(struct ftp_server *ftps)
{
	//attemps CDUP command,
	//return value
	//0 - success
	//-1- failed

	if(!ftp_check_server_status(ftps,FTPS_CONTROL_CONNECTED | 
						FTPS_LOGGED_IN,"ftpc_cdup"))
		return -1;
	
	
	char *command_str;
	struct ftp_response *fres;
	
	if(ftp_command_str(&command_str,"CDUP","") == -1)
		return -1;

	if(ftp_command(ftps,&fres,command_str)==-1)
	{
		log_error("ftpc_cdup: CDUP command failed.");
		ftp_response_free(fres);
		return -1;
	}

	if(fres->code != FTPC_COMMAND_OK && fres->code != FTPC_FILE_OK)
	{		
	
		ftp_command_failed(fres->code,fres->message,"CDUP");
                ftp_response_free(fres);
                return -1;
	}


	log_message("ftpc_cdup: success.");
	ftp_response_free(fres);

	return 0;



}
