#ifndef FTP_H
#define FTP_H

#include "sys/types.h"
#include "sys/socket.h"
#include "netdb.h"
#include "arpa/inet.h"
#include "netinet/in.h"



#define FTPS_AVAILABLE		 1
#define FTPS_CONTROL_CONNECTED	 2
#define FTPS_DATA_CONNECTED	 4
#define FTPS_LOGGED_IN		 8


#define FTP_TYPE_ASCII   26
#define FTP_TYPE_BINARY  7

struct ftp_server
{
	struct addrinfo *cc_info;//control connection info
	struct addrinfo *dc_info;//data connection info
	int 	 cc_socket;//control socket
	int	 dc_socket;//data socket
	int 	 server_status;
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



void ftp_fs_free(struct ftp_fs *);
void ftp_response_free(struct ftp_response *);

int ftp_server_info(const char *,const char *,struct ftp_server **);
void ftp_server_free(struct ftp_server *);


int ftp_connect(struct addrinfo *,int *);

int ftp_send(struct ftp_server *,int,const char *);
int ftp_receive(struct ftp_server *,int, char **);

int ftp_command(struct ftp_server *,struct ftp_response **,const char *);

int ftp_command_str(char **,const char *,const char*);



#endif 
