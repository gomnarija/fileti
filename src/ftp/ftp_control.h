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


#ifndef FTP_CONTROL_H
#define FTP_CONTROL_H

#include "ftp.h"



int ftpc_active(struct ftp_server *);
int ftpc_connect(struct ftp_server *);
int ftpc_cwd(struct ftp_server *,const char *);
int ftpc_disconnect(struct ftp_server *);
int ftpc_ent_info(struct ftp_server *,const char *,struct ftp_file **);
int ftpc_login(struct ftp_server *,const char *,const char *);
int ftpc_mode(struct ftp_server *,const int);
int ftpc_mkdir(struct ftp_server *,const char *);
int ftpc_password(struct ftp_server *,const char *);
int ftpc_passive(struct ftp_server *);
int ftpc_pwd(struct ftp_server *,struct ftp_fs **);
int ftpc_rm(struct ftp_server *,const char *);
int ftpc_rmdir(struct ftp_server *,const char *);
int ftpc_type(struct ftp_server *,const int);
int ftpc_user(struct ftp_server *,const char *);






#endif
