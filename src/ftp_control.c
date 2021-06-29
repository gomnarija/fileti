#include "ftp_control.h"
#include "utils/log.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"

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

	free(command_str);

	if(fres->code == 331)
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

	free(command_str);

	if(fres->code == 230)
	{
		log_message("ftpc_password: success.");
		log_message("Logged in.");	
		ftps->server_status |= FTPS_LOGGED_IN;		
		ftp_response_free(fres);
		return 0;
	}
	else if(fres->code == 430 || fres->code == 530)//some servers return 530
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
	//-1 - wrong user or password 
	//-2 - failed
	
	if(!(ftps->server_status & FTPS_CONTROL_CONNECTED))
	{
		log_error("ftp_login:ftp_server not connected.");
		return -2;
	}

	if(ftpc_user(ftps,user_name)==-1)
	{
		return -1;
	}

	return ftpc_password(ftps,password);


}

int ftpc_passive(struct ftp_server *ftps)
{
	//attempts initiating passive connection,
	//sends PASV command, and waits for port form server
	//return valie
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


	free(command_str);


	if(fres->code == 227)
	{
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
			}
		}

		ip[0] = strtol(startp,&endp,10);
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
			return -1; 
		}
		
		log_message("ftpc_passive: success.");
		ftp_response_free(fres);
		return 0;
	}
	else
	{
		
	
		char buf[16];
		snprintf(buf,16,"code:%d",fres->code);
		log_error("ftpc_passive: command PASV failed.");
		log_error(buf);
		ftp_response_free(fres);
		return -1;

	}	
	
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
		log_error("ftpc_pwd:mallo() failed.");
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

	if(fres->code != 257)
	{		
	
		char buf[16];
                snprintf(buf,16,"code:%d",fres->code);
                log_error("ftpc_pwd: command PWD failed.");
                log_error(buf);
                ftp_response_free(fres);
                return -1;
	}


	//pwd format 
	//"pwd" some text

	char *startp,*endp;
	startp = fres->message;
	while(*startp!='"')
	{
		startp++;
		if(startp-fres->message > strlen(fres->message))
		{
			log_error("ftpc_pwd: response message in wrong format.");
			return -1;
		}
	}

		


	endp = startp+1;
	while(*endp!='"')
	{
		endp++;
		if(endp-fres->message > strlen(fres->message))
		{
			log_error("ftpc_pwd: response message in wrong format.");
			return -1;
		}
	}
	
	*endp = '\0';

	if(((*ftfs)->pwd = (char *)malloc(strlen(startp)))==NULL)
	{
		log_error("ftpc_pwd: malloc failed");
		return -1;
	}
	snprintf((*ftfs)->pwd,strlen(startp),"%s",startp+1);
	
	ftp_response_free(fres);
	free(command_str);

	return 0;


}

