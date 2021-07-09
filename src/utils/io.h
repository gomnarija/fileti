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


#ifndef IO_H
#define IO_H

#include "../ftp/ftp.h"
#include "log.h"
#include "stdio.h"
#include "stdlib.h"
#include "sys/types.h"
#include "dirent.h"
#include "string.h"
#include "unistd.h"
#include "limits.h"

int io_write(const char *,const char *,int);
int io_read(const char *,char **,int *,FILE **);
int io_list(struct ftp_fs **,const char *);
int io_pwd(char *);

#endif
