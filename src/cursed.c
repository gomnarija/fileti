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

#include "cursed.h"

int cur_init()
{
	setlocale(LC_ALL,"");
	initscr();
	cbreak();
	timeout(0);
	noecho();
	keypad(stdscr, TRUE);
	curs_set(0);


	return 0;
	
}
int cur_quit()
{
	
	echo();
	keypad(stdscr, FALSE);
	curs_set(1);
	endwin();
	
	return 0;
}
int cur_draw_frame()
{

	
	int x,y;
	getmaxyx(stdscr,y,x);

	int sep=x/2,
		tri = (y/3)*2,
			bot = y-1;
		

	mvvline(0,sep,0,bot);	
	mvhline(tri,0,0,sep);	
	mvaddch(tri,sep,ACS_RTEE);
	
	attron(A_STANDOUT);
		mvhline(bot,0,' ',x);
		mvaddstr(bot,1,"fileTI");
	attroff(A_STANDOUT);




	return 0;

}















