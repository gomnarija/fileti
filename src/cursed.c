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



#include "stdlib.h"

void cur_init()
{
	setlocale(LC_ALL,"");
	initscr();
	cbreak();
	timeout(0);
	noecho();
	keypad(stdscr, TRUE);
	curs_set(0);


	
}
void cur_quit()
{
	
	echo();
	keypad(stdscr, FALSE);
	curs_set(1);
	endwin();
	
}
void  cur_draw_frame(struct cur_sec *lsup,struct cur_sec *lsdown,struct cur_sec *rs)
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


	//left side top panel
	lsup->x=  0;
	lsup->y=  0;
	lsup->h=  tri-1;
	lsup->w=  sep;

	//left side bottom panel
	lsdown->x=  0;
	lsdown->y=  tri+1;
	lsdown->h=  bot - tri-1;
	lsdown->w=  sep;

	//right side panel
	rs->x=  sep+1;
	rs->y=  0;
	rs->h=  y-1;
	rs->w=  x-sep-1;




}


void cur_fs_fill(const struct cur_sec cs,const struct ftp_fs *ftfs,const int sel)
{
	if(!ftfs)
	{

		attron(A_STANDOUT);
			mvaddstr(cs.y,cs.x,"/");
		attroff(A_STANDOUT);
		mvaddstr(cs.y+2,cs.x+1,"[empty]");
	
		return;
	}

	attron(A_STANDOUT);
		mvaddstr(cs.y,cs.x,ftfs->pwd);
	attroff(A_STANDOUT);




	struct ftp_file *fifi;

	int l=2,
		curr=0;



	static int offset = 0;

	if(sel < offset)
		offset = sel;
	if(sel >= offset+cs.h-l)
		offset = sel-(cs.h -l)+1; 

	mvaddch(0,0,offset+'0');

	fifi = ftfs->files;
	if(!fifi)
		mvaddstr(cs.y+2,cs.x+1,"[empty]");


	while(fifi)
	{
		if(curr >= offset)
		{
			if(curr == sel)
				attron(A_STANDOUT);

			mvaddstr(cs.y+l,cs.x+1,fifi->name);
	
			if(curr == sel)
				attroff(A_STANDOUT);
	
			l++;	
			if(l>=cs.h)
				return;

		}
		curr++;
		fifi = fifi->next;		
		
	}


}













