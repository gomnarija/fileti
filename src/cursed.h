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

#ifndef CURSED_H
#define CURSED_H

#include "curses.h"
#include "utils/log.h"
#include "locale.h"
#include "ftp/ftp.h"
#include "string.h"
#include "stdlib.h"

struct cur_sec
{
	int x,y,h,w;
};


void cur_init();
void cur_quit();
void cur_draw_frame(struct cur_sec *,struct cur_sec *,struct cur_sec *);
void cur_fs_fill(const struct cur_sec,const struct ftp_fs *,const int,const int,int *);
void cur_raw_fill(char **,struct cur_sec,int *,int,int *);
int cur_command(char **);

void cur_serv_info(struct ftp_server *);
#endif
