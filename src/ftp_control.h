#ifndef FTP_CONTROL_H
#define FTP_CONTROL_H

#include "ftp.h"


int ftpc_user(struct ftp_server *,const char *);
int ftpc_password(struct ftp_server *,const char *);
int ftpc_passive(struct ftp_server *);
int ftpc_pwd(struct ftp_server *,struct ftp_fs **);


int ftp_login(struct ftp_server *,const char *,const char *);




#endif
