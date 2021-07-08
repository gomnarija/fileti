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


#include "ftp/ftp.h"
#include "ftp/ftp_control.h"
#include "ftp/ftp_data.h"
#include "utils/log.h"
#include "utils/io.h"
#include "utils/msleep.h"
#include "cursed.h"
#include "commands.h"
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"



void s_connect(struct ftp_server **,struct com_com *);
void s_login(struct ftp_server **,struct com_com *);

void s_ls(struct ftp_server **,struct ftp_fs **);
void l_ls(struct ftp_fs **,char **);


void l_enter_dir(struct ftp_fs **,struct ftp_file *,int *,char *);
void s_enter_dir(struct ftp_fs **,struct ftp_file *,int *,struct ftp_server *);




int main()
{


	struct ftp_server *ftps=NULL;
	
	struct ftp_fs 	  *sfs=NULL,//server fs
			  *lfs=NULL;//local  fs

	struct ftp_file   *sifi=NULL,//server selected
			  *lifi=NULL;//local selected


		
	struct cur_sec    lsup,   //left top panel
			  lsdown, //left bottom panel
			  rs;    //right panel

	struct com_com *comic=NULL;//command
	
	int    sels=0,	//selected server index
	       sell=0,    //selected local index
	       *csel;	//curr sel index

	int    lof=0,//left offset
	       rof=0;//right offset

	int    selside = 0;

	char   *lwd=NULL;//local working dir

	char   *cbuff=NULL;//command buffer


	
	


	
	//default select left
	csel = &sell;

	
	l_ls(&lfs,&lwd);






	////

	cur_init();
	while(1)
	{
	
		//return curr ftp_file from index
		ftp_fs_select(sfs,&sifi,&sels);
		ftp_fs_select(lfs,&lifi,&sell);

	
		////
		cur_draw_frame(&lsup,&lsdown,&rs);
		
		cur_fs_fill(lsup,lfs,sell,csel == &sell,&lof);
		cur_fs_fill(rs,sfs,sels,csel == &sels,&rof);
		
	
		cur_serv_info(ftps);

		////
	
		selside = csel == &sell?0:1;

	


		//
		int c = getch();
		
		if(c=='q')
			break;

		if(c==KEY_UP)
			(*csel)--;
		if(c==KEY_DOWN)
			(*csel)++;
	
		if(c==' ')
			csel = csel == &sell?&sels:&sell;
	
		if(c=='e')
		{
			if(selside)//server
				s_enter_dir(&sfs,sifi,&rof,ftps);
			else//local
				l_enter_dir(&lfs,lifi,&lof,lwd);
		}
	

		////
		if(c=='/')//commands
		{
			if(!cur_command(&cbuff) &&
				!com_parse(cbuff,&comic))
			{
				if(!strcmp(comic->command,"connect"))
					s_connect(&ftps,comic);
				
				if(!strcmp(comic->command,"login"))
				{	
					s_login(&ftps,comic);
					s_ls(&ftps,&sfs);
				}
			}
			
		}



		////
		refresh();
		clear();
		msleep(10);	
		



	}
	cur_quit();
	
	if(ftps)
		ftpc_disconnect(ftps);
	return 0;



}


void l_enter_dir(struct ftp_fs **ftfs,struct ftp_file *fifi,int *off,char *lwd)
{
	if(!strcmp(fifi->type,"file"))
	{
		log_warning("l_enter_dir: not a dir.");
		log_warning(fifi->name);
		return;
	}

	strcat(lwd,"/");
	strcat(lwd,fifi->name);
	io_list(ftfs,lwd);

	*off = 0;
}
void s_enter_dir(struct ftp_fs **ftfs,struct ftp_file *fifi,int *off,struct ftp_server *ftps)
{
		
	if(!ftps ||
		!ftp_check_server_status(ftps,FTPS_CONTROL_CONNECTED,"s_enter_dir"))
                return;
	
	if(!strcmp(fifi->type,"file"))
	{
		log_warning("s_enter_dir: not a dir.");
		log_warning(fifi->name);
		return;
	}

	if(ftpc_cwd(ftps,fifi->name) ||
		ftpc_pwd(ftps,ftfs)  ||
			ftpd_list(ftps,*ftfs,""))
	{
		free(*ftfs);
		*ftfs = NULL;
		return;
	}
	*off = 0;
}
void s_connect(struct ftp_server **ftps,struct com_com *comic)
{
		
	if(*ftps &&
	    ftp_check_server_status(*ftps,FTPS_CONTROL_CONNECTED,"s_connect"))
		return;


	char *ip,
		*port;

	if(!(comic->args) || !(comic->args->next))
		return;

	ip = comic->args->arg;
	port = comic->args->next->arg;

	if(!ftp_server_info(ip,port,ftps) &&
		!ftpc_connect(*ftps))
		return;

}
void s_login(struct ftp_server **ftps,struct com_com *comic)
{
	if(!(*ftps) ||
	    !ftp_check_server_status(*ftps,FTPS_CONTROL_CONNECTED,"s_login"))
		return;

	char *user,
		*password;

	if(!(comic->args) || !(comic->args->next))
		return;

	user = comic->args->arg;
	password = comic->args->next->arg;

	ftpc_login(*ftps,user,password);

}
void s_ls(struct ftp_server **ftps,struct ftp_fs **ftfs)
{
	if(!(*ftps) ||
	    !ftp_check_server_status(*ftps,FTPS_CONTROL_CONNECTED |
						FTPS_LOGGED_IN,"s_ls"))
		return;

	if(!ftpc_pwd(*ftps,ftfs) &&
		!ftpd_list(*ftps,*ftfs,""))
		return;

}

void l_ls(struct ftp_fs **ftfs,char **lwd)
{
	io_pwd(lwd);
	io_list(ftfs,*lwd);

}

