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
#include "sys/stat.h"



void s_connect(struct ftp_server **,struct com_com *);
void s_login(struct ftp_server **,struct com_com *);

void s_ls(struct ftp_server **,struct ftp_fs **);
void l_ls(struct ftp_fs **,char **);


void s_rm(struct ftp_server **,struct com_com *);
void l_rm(struct com_com *);
void s_rmdir(struct ftp_server **,struct com_com *);
void l_rmdir(struct com_com *);



void l_pwd(struct ftp_fs **,char **);

void s_rtr(struct ftp_server **,struct com_com *);
void s_snd(struct ftp_server **,struct com_com *);

void l_enter_dir(struct ftp_fs **,struct ftp_file *,int *);
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
	       selr=0,   //selected raw
	       *csel;	//curr sel index

	int    lof=0,//left offset
	       rof=0,//right offset
	       raof=0;//raw offset

	int    selside = 0;

	char   *cbuff=NULL;//command buffer
	char   *rawbuff=NULL;

	
	


	
	//default select left
	csel = &sell;


	//curr local dir	
	l_pwd(&lfs,&(lfs->pwd));



	raw_init();
	log_raw("fileTI ftp client.",1);
	log_raw("/help  :)",1);



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
	
		cur_raw_fill(&rawbuff,lsdown,&selr,csel == &selr,&raof);	
	
		cur_serv_info(ftps);

		////
	
		selside = csel == &sell?0:1;

	


		//keys
		int c = getch();
		
		if(c=='q')
			break;

		if(c==KEY_UP)
			(*csel)--;
		if(c==KEY_DOWN)
			(*csel)++;
	
		if(c==' ' && 
			ftps != NULL  && 
				ftps->server_status & FTPS_CONTROL_CONNECTED)
			csel = csel == &sell?&sels:&sell;

		if(c=='r')
			if(selside==1 && sifi)
			{
				ftpd_retrieve(ftps,sifi->name,sifi->name);
				l_ls(&lfs,&(lfs->pwd));
			}

		if(c=='z')
			csel = csel == &selr?&sell:&selr;



		if(c=='d')
		{
			if(selside==1)//server
			{
				ftpc_rm(ftps,sifi->name);
				s_ls(&ftps,&sfs);
			}
			else if(!selside)//local
			{
				remove(lifi->name);
				l_ls(&lfs,&(lfs->pwd));
			}
		}
		
		if(c=='s')
		{
			if(!selside)//local
			{
				ftpd_store_file(ftps,sifi->name,sifi->name);
				s_ls(&ftps,&sfs);
				
			}
		}
		if(c=='e')
		{
			if(selside==1)//server
				s_enter_dir(&sfs,sifi,&rof,ftps);
			else if(!selside)//local
				l_enter_dir(&lfs,lifi,&lof);
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
				if(!strcmp(comic->command,"ls"))
				{	
					if(selside==1)
						s_ls(&ftps,&sfs);
					if(!selside)
						l_ls(&lfs,&(lfs->pwd));

				}
				if(!strcmp(comic->command,"wd"))
				{	
					l_pwd(&lfs,&(lfs->pwd));

				}
				if(!strcmp(comic->command,"rtr"))
				{	
					s_rtr(&ftps,comic);
					l_ls(&lfs,&(lfs->pwd));
				}
				if(!strcmp(comic->command,"rm"))
				{	
					if(selside==1)
					{
						s_rm(&ftps,comic);
						s_ls(&ftps,&sfs);
					}
					else if(!selside)
					{
						l_rm(comic);
						l_ls(&lfs,&(lfs->pwd));	
					}
				}
				if(!strcmp(comic->command,"rmdir"))
				{	
					if(selside==1)
					{
						s_rmdir(&ftps,comic);
						s_ls(&ftps,&sfs);
					}
					else if(!selside)
					{
						l_rmdir(comic);
						l_ls(&lfs,&(lfs->pwd));	
					}
				}
				if(!strcmp(comic->command,"send"))
				{	
					s_snd(&ftps,comic);
					s_ls(&ftps,&sfs);	
				}




			}
			
		}



		////
		refresh();
		clear();
		msleep(25);		



	}
	cur_quit();
	
	if(ftps)
		ftpc_disconnect(ftps);
	if(sfs)
		ftp_fs_free(sfs);
	if(lfs)
		ftp_fs_free(lfs);

	
	return 0;



}


void l_enter_dir(struct ftp_fs **ftfs,struct ftp_file *fifi,int *off)
{
	if(!strcmp(fifi->type,"file"))
	{
		log_warning("l_enter_dir: not a dir.");
		log_warning(fifi->name);
		return;
	}
	
	chdir(fifi->name);
	l_pwd(ftfs,&((*ftfs)->pwd));
	io_list(ftfs,(*ftfs)->pwd);


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
	{
		log_raw("already connected. ",1);
		return;
	}

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

	if(!ftpc_login(*ftps,user,password))
		ftpc_type(*ftps,FTP_TYPE_BINARY);//temp i guess

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
	else
	{
		*ftfs = NULL;
	}
}

void l_ls(struct ftp_fs **ftfs,char **lwd)
{
	io_list(ftfs,(*ftfs)->pwd);
}

void l_pwd(struct ftp_fs **ftfs,char **lwd)
{
	char buff[PATH_MAX];

	io_pwd(buff);
	io_list(ftfs,buff);

}

void s_rtr(struct ftp_server **ftps,struct com_com *comic)
{
	if(!(*ftps) ||
	    !ftp_check_server_status(*ftps,FTPS_CONTROL_CONNECTED |
						FTPS_LOGGED_IN,"s_rtr"))
		return;




	char *src_name,
		*dst_name;

	if(!(comic->args) || !(comic->args->next))
		return;

	src_name = comic->args->arg;
	dst_name = comic->args->next->arg;


	ftpd_retrieve(*ftps,src_name,dst_name);


}
void s_rm(struct ftp_server **ftps,struct com_com *comic)
{
	if(!(*ftps) ||
	    !ftp_check_server_status(*ftps,FTPS_CONTROL_CONNECTED |
						FTPS_LOGGED_IN,"s_rm"))
		return;



	char *name;

	if(!(comic->args))
		return;

	name = comic->args->arg;


	ftpc_rm(*ftps,name);


}
void l_rm(struct com_com *comic)
{

	char *name;

	if(!(comic->args))
		return;


	struct stat ss;

	name = comic->args->arg;

	stat(name,&ss);
	if(S_ISDIR(ss.st_mode))
		return;
	
	remove(name);

}
void s_rmdir(struct ftp_server **ftps,struct com_com *comic)
{
	if(!(*ftps) ||
	    !ftp_check_server_status(*ftps,FTPS_CONTROL_CONNECTED |
						FTPS_LOGGED_IN,"s_rm"))
		return;



	char *name;

	if(!(comic->args))
		return;

	name = comic->args->arg;


	ftpc_rmdir(*ftps,name);


}
void l_rmdir(struct com_com *comic)
{

	char *name;

	if(!(comic->args))
		return;


	struct stat ss;

	name = comic->args->arg;

	stat(name,&ss);
	if(!S_ISDIR(ss.st_mode))
		return;

	rmdir(name);
}
void s_snd(struct ftp_server **ftps,struct com_com *comic)
{
	if(!(*ftps) ||
	    !ftp_check_server_status(*ftps,FTPS_CONTROL_CONNECTED |
						FTPS_LOGGED_IN,"s_snd"))
		return;



	char *src_name,
		*dst_name;

	if(!(comic->args) || !(comic->args->next))
		return;

	src_name = comic->args->arg;
	dst_name = comic->args->next->arg;

	ftpd_store_file(*ftps,src_name,dst_name);


}

