/* 
 * Copyright (C) 2002 - David W. Durham
 * 
 * This file is part of ReZound, an audio editing application.
 * 
 * ReZound is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 * 
 * ReZound is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 */

/*
	I discovered what I think to be a bug in glibc where the following
	program will fail ONLY when gcc is giving --static which causes glibc
	to be linked to statically.

		#include <stdio.h>
		#include <pthread.h>

		void *func(void *temp)
		{
			return NULL;
		}

		int main()
		{
			pthread_t threadID;

			popen("ls","r");

			pthread_create(&threadID,NULL,func,NULL);

			return 0;
		}

	So, when ReZound is being linked statically, mypopen is used instead of
	popen.  I had to write this mypopen function for a college operating 
	systems lab class, so I thought I'd put it to good use.  Also, I might 
	start using this exclusively which would give me more control like 
	changing it to also let me capture the spawned process's stderr stream 
	as well as stdout.

	Also, this would be mypopen.c instead of mypopen.cpp except that I need
	to include ../../config/common.h which has some C++ in it.
	
 */

#include "mypopen.h"

#ifdef LINKING_STATICALLY

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MAX_OPENS 100
int child_pids[MAX_OPENS]; /* init to zero */
FILE *pipe_streams[MAX_OPENS];

/* 
	passed:	process id -- pid
		stream *   -- s
	returns: 0 -- success
		 1 -- failure
*/
int add_piped_process(int pid,FILE *s)
{
	int t;
	for(t=0;t<MAX_OPENS;t++)
	{
		if(child_pids[t]==0)
		{
			child_pids[t]=pid;
			pipe_streams[t]=s;
			return(0);
		}
	}
	return(1);
}

/*
	passed:	stream * -- s;
	returns: 0 -- not in list
		 else -- pid
*/
int remove_piped_process(FILE *s)
{
	int t;
	for(t=0;t<MAX_OPENS;t++)
	{
		if(pipe_streams[t]==s)
		{
			int pid=child_pids[t];
			child_pids[t]=0;
			return(pid);
		}
	}
	return(0);
}



FILE *mypopen(const char cmd[],const char type[])
{
	int pid;
	int pipe_ends[2];

	if(pipe(pipe_ends)!=0)
		return(NULL);

	if(strcmp(type,"w")==0)
	{
		if((pid=fork())==0)
		{ /* child */
			close(0);
			if(dup(pipe_ends[0])!=0)
			{
				fprintf(stderr,"dup didn't assign to stdin\n");
				return(NULL);
			}
			close(pipe_ends[0]);
			close(pipe_ends[1]);
			execlp("/bin/sh","/bin/sh","-c",cmd,NULL);
			return(NULL); // just to make compiler happy
		}
		else
		{ /* parent */
			FILE *pipe_end=fdopen(pipe_ends[1],type);
			add_piped_process(pid,pipe_end);
			close(pipe_ends[0]);
			return(pipe_end);
		}
	}
	else if(strcmp(type,"r")==0)
	{
		/*
		Using vfork might be better but I wrote it originally
		for fork and ReZound doesn't use popen for much and 
		with linux's copy-on-write paging it shouldn't be too
		expensive to fork
		*/
		if((pid=fork())==0)
		{ /* child */
			close(1);
			dup(pipe_ends[1]);
			close(pipe_ends[1]);
			close(pipe_ends[0]);
			close(0);
			execlp("/bin/sh","/bin/sh","-c",cmd,NULL);
			return(NULL); // just to make compiler happy
		}
		else
		{ /* parent */
			FILE *pipe_end=fdopen(pipe_ends[0],type);
			add_piped_process(pid,pipe_end);
			close(pipe_ends[1]);
			return(pipe_end);
		}
	}
	else
		return(NULL);

}


int mypclose(FILE *p)
{
	int pid=remove_piped_process(p);
	int statval;

	if(pid==0)
		return(-1);

	fclose(p);
	waitpid(pid,&statval,0);
	return(0);
}

#endif // LINKING_STATICALLY
