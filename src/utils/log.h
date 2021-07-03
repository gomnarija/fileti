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


#ifndef LOG_H
#define LOG_H

#define LOG_MESSAGE 1
#define LOG_WARNING 2
#define LOG_ERROR   3


#define log_message(msg) log_log(LOG_MESSAGE,msg)
#define log_warning(msg) log_log(LOG_WARNING,msg)
#define log_error(msg)   log_log(LOG_ERROR,msg)

#define LOG_FILE   "fileti.log" //temp, i guess

void log_log(int log_level,const char *message);


#endif
