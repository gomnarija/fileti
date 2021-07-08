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

#include "commands.h"



int com_parse(char *raw,struct com_com **comic)
{

	//format
	//command arg arg
	
	if(*comic)
	{
		struct com_arg *cur;
			
		cur = (*comic)->args;
		while(cur)
		{
			struct com_arg *tmp;
			tmp = cur;
			cur = cur->next;
			free(tmp);
		}
		free(*comic);
	}
		

	char *sp,*ep;
	
	sp = raw;
	ep = strchr(raw,' ');

	if(ep == NULL)
		return -1;

	
	*ep='\0';
	
	
	if((*comic = (struct com_com*)malloc(sizeof(struct com_com)))==NULL ||
		((*comic)->command = (char*)malloc(strlen(sp)+1))==NULL)
		return -1;	

	snprintf((*comic)->command,strlen(sp)+1,"%s",sp);
		
	struct com_arg **args=&((*comic)->args);
	*args = NULL;
	
	do
	{
	
		sp = ep;sp++;
	        ep = strchr(sp,' ');
		if(!ep || *sp == ' ')
			break;
		
		*ep = '\0';
	
		if((*args = (struct com_arg*)malloc(sizeof(struct com_com)))==NULL ||
		     ((*args)->arg = (char*)malloc(strlen(sp)+1))==NULL)							return -1;
	
		snprintf((*args)->arg,strlen(sp)+1,"%s",sp);
		args = &((*args)->next);
		*args = NULL;		

	}while(1);
	
	return 0;
}
