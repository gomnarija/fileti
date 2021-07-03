#ifndef FTP_DATA_H
#define FTP_DATA_H

#include "ftp.h"

int ftpd_connect(struct ftp_server* ,int);
int ftpd_disconnect(struct ftp_server *);
int ftpd_list(struct ftp_server *,struct ftp_fs *,const char *);
int ftpd_retrieve_file(struct ftp_server *,const char *,const char *);


#endif
