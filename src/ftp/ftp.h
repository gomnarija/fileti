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


#ifndef FTP_H
#define FTP_H

#include "sys/types.h"
#include "sys/socket.h"
#include "netdb.h"
#include "arpa/inet.h"
#include "netinet/in.h"
#include "pthread.h"


//server status
#define FTPS_AVAILABLE		 1
#define FTPS_CONTROL_CONNECTED	 2
#define FTPS_DATA_CONNECTED	 4
#define FTPS_LOGGED_IN		 8


#define FTP_TYPE_ASCII  	 26
#define FTP_TYPE_BINARY 	 7

#define FTP_MODE_STREAM		 111
#define FTP_MODE_BLOCK		 112//not supported
#define FTP_MODE_COMPRESSED	 113//not supported


#define FTPD_ACTIVE		69
#define FTPD_PASSIVE		70

//response codes
#define FTPC_DATA_OPENING	150
#define FTPC_COMMAND_OK		200
#define FTPC_CONTROL_CLOSING	221
#define FTPC_DATA_CLOSING	226
#define FTPC_PASSIVE_MODE	227
#define FTPC_FILE_OK		250
#define FTPC_PATH_NAME		257
#define FTPC_LOGGED_IN		230
#define FTPC_USER_OK		331
#define FTPC_INVALID_CRED	430
#define FTPC_NOT_LOGGED		530


#define FTP_TIMEOUT		1000



//transfer modes
#define FTPT_CONTROL		600
#define FTPT_STREAM		601
#define FTPT_ASCII		602
#define FTPT_BLOCK		603
#define FTPT_HASP		604



struct ftp_server
{
	struct addrinfo *cc_info;//control connection info
	struct addrinfo *dc_info;//data connection info
	int 	 cc_socket;//control socket
	int	 dc_socket;//data socket
	int 	 server_status;
	pthread_t dc_thread;//used for accept(), which is placed in another thread
};


struct ftp_response
{
	char *message;
	int code;
	struct ftp_response *next;
};

struct ftp_file
{
	char *name;
	char *type;
	int size;
	struct ftp_file *next;	
};

struct ftp_fs
{
	char *pwd;
	struct ftp_file *files;
};


void *ftp_accept(void *);
int  ftp_connect(struct addrinfo *,int *);
int  ftp_command(struct ftp_server *,struct ftp_response **,char *);
void ftp_command_failed(const int,char *,const char *);
int  ftp_command_str(char **,const char *,const char*);
int  ftp_check_server_status(struct ftp_server *,const int,const char *);
void ftp_fs_free(struct ftp_fs *);
void ftp_response_free(struct ftp_response *);
int  ftp_receive(struct ftp_server *,int, char **,int *);
int  ftp_server_info(const char *,const char *,struct ftp_server **);
void ftp_server_free(struct ftp_server *);
int  ftp_send(struct ftp_server *,int,const char *);



#endif 
