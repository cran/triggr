/* Trigger thread

   Copyright (c)2011 Miron Bartosz Kursa
 
   This file is part of triggr R package.

 Triggr is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 Triggr is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 You should have received a copy of the GNU General Public License along with triggr. If not, see http://www.gnu.org/licenses/. */

static void cbIdleAgain(struct ev_loop *loop,ev_async *this,int revent){
 pthread_mutex_lock(&gqM);
 //Put the output on the write queue of the connection
 if(active && !lastOrphaned && !termCon) makeOutputBuffer(lastResult,lastDoneConnection,killConnectionAftrSend);//This also starts writer watchers
 if(!active) ev_break(lp,EVBREAK_ONE);
 if(termCon) killConnection(lastDoneConnection);
 if(lastResult) free(lastResult);//Malloc'ed in code.c
 
 pthread_mutex_unlock(&gqM);
 
 //Allow new jobs to be processed
 pthread_mutex_lock(&outSchedM);
 pthread_cond_signal(&outSchedC);
 pthread_mutex_unlock(&outSchedM);
}

static void onTim(struct ev_loop *lp,ev_timer *this,int revents){
 Rprintf("Triggr is alive; %d jobs form %d clients processed, %d in processing now, %d clients connected.\n",processedJobs,clients,working,curClients);
}

void* trigger(void *arg){
 lp=ev_loop_new(EVFLAG_AUTO);
 ev_async_init(&idleAgain,cbIdleAgain);
 ev_async_start(lp,&idleAgain);
 
 //Wait for the R part to go into idleLock
 pthread_mutex_lock(&idleM);
 pthread_mutex_unlock(&idleM); 
 
 //Installing server-is-alive timer
 struct ev_timer timer;
 ev_timer_init(&timer,onTim,5.,35.);
 ev_timer_start(lp,&timer);

 //Installing accept watcher
 ev_io acceptWatcher;
 ev_io_init(&acceptWatcher,cbAccept,acceptFd,EV_READ);
 ev_io_start(lp,&acceptWatcher);

 //Run loop
 ev_run(lp,0);

 //Clean up; we don't need any locks since main thread is waiting for a join now
 Rprintf("Cleaning up...\n");
 while(GlobalQueue.headCon!=NULL) killConnection(GlobalQueue.headCon);
 ev_loop_destroy(lp);
 Rprintf("Trigger thread terminates NOW...\n");
 return(NULL);
}
