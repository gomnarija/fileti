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


#ifndef FTP_DATA_H
#define FTP_DATA_H

#include "ftp.h"

int ftpd_connect(struct ftp_server* ,int);
int ftpd_disconnect(struct ftp_server *);
int ftpd_list(struct ftp_server *,struct ftp_fs *,const char *);
int ftpd_retrieve(struct ftp_server *,const char *,const char *);
int ftpd_retrieve_file(struct ftp_server *,const char *,const char *);
int ftpd_retrieve_dir(struct ftp_server *,const char *,const char *);


#endif
