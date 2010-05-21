/* Controller of the R session + additional C chunks

   Copyright (c)2011 Miron Bartosz Kursa
 
   This file is part of triggr R package.

 Triggr is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 Triggr is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 You should have received a copy of the GNU General Public License along with triggr. If not, see http://www.gnu.org/licenses/. */

#include <R.h>
#include <Rdefines.h>
#include <Rinternals.h>
#include <unistd.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <sys/stat.h> 
#include <fcntl.h>  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h> 
 
#define EV_STANDALONE 7
#include "ev.c"

#define HAVE_UINTPTR_T 7
#define CSTACK_DEFNS 7  
#include <Rinterface.h>
  
#define MAX_CLIENTS 3
#define MAX_QSIZE 1024 

#include "defs.h"

//Request object
#include "InBuffer.c"
//Response object 
#include "OutBuffer.c"
//In-queue incarnation of request
#include "WorkBuffer.c"
//Connection object
#include "Connection.c"
//Trigger thread
#include "Trigger.c"

void makeGlobalQueue(){
 pthread_mutex_lock(&gqM);
 GlobalQueue.tailWork=
  GlobalQueue.headWork=
   GlobalQueue.tailCon=
    GlobalQueue.headCon=NULL;
 GlobalQueue.curCon=0;
 pthread_mutex_unlock(&gqM);
}

void sigpipeHandler(int sig){
 //Do very nothing
}

SEXP getCID(){
 SEXP ans;
 if(!GlobalQueue.headWork) error("getConID() called outside callback!\n");
 PROTECT(ans=allocVector(INTSXP,1));
 INTEGER(ans)[0]=curConID;
 UNPROTECT(1);
 return(ans); 
}

//Function running the trigger
SEXP startTrigger(SEXP port,SEXP wrappedCall,SEXP envir){
 R_CStackLimit=(uintptr_t)-1;
 active=1; count=0;
 port=INTEGER(port)[0];
 makeGlobalQueue(); 
 pthread_t thread;
 int rc;
 
 //Ignore SIGPIPE
 void *oldSigpipe=signal(SIGPIPE,sigpipeHandler);
 //Initiate network interface
 if((acceptFd=socket(AF_INET,SOCK_STREAM,0))<0) error("Cannot open listening socket!");
  
 struct sockaddr_in serverAddr;
 bzero((char*)&serverAddr,sizeof(serverAddr));
 serverAddr.sin_family=AF_INET;
 serverAddr.sin_addr.s_addr=INADDR_ANY; //Bind to localhost
 serverAddr.sin_port=htons(port);

 if(bind(acceptFd,(struct sockaddr*)&serverAddr,sizeof(serverAddr))<0) error("Cannot bind server!");

 //Starting listening for clients
 if(listen(acceptFd,MAX_CLIENTS)<0) error("Cannot listen with server!");
 //Libev will be binded to this interface in the trigger thread
 
 active=1; 

 pthread_mutex_lock(&idleM); //Hold the server from true staring 
 rc=pthread_create(&thread,NULL,trigger,NULL);
 
 Rprintf("Listening on port %d.\n",port);
 
 //Starting process loop
 for(processedJobs=0;active;){
  //We musn't lock the mutex if it was already locked in init 
  if(processedJobs>0) pthread_mutex_lock(&idleM);
  pthread_cond_wait(&idleC,&idleM);
  pthread_mutex_unlock(&idleM);  
       
  pthread_mutex_lock(&gqM); 
  while(GlobalQueue.headWork!=NULL){
   working=1;
   WorkBuffer *WB=GlobalQueue.headWork;
   WB->working=1;
   termCon=0;
   Connection *c=WB->c;
   SEXP arg;
   PROTECT(arg=allocVector(STRSXP,1));
   SET_STRING_ELT(arg,0,mkChar(WB->buffer));
   curConID=c->ID;
   pthread_mutex_unlock(&gqM);
   
   //Execute processing code on the GlobalQueue.headWork's contents
   SEXP response;
   const char *responseC=NULL;
   SEXP call;
   PROTECT(call=lang2(wrappedCall,arg));
   PROTECT(response=eval(call,envir));
   if(TYPEOF(response)!=STRSXP){
    if(TYPEOF(response)==INTSXP && LENGTH(response)==1){
     int respCode=INTEGER(response)[0];
     if(respCode==0) active=0; else termCon=1; 
     responseC=NULL;
    }else{
     error("PANIC: Bad callback wrapper result! Triggr will dirty-collapse now.\n");
    }
   }else{
    if(LENGTH(response)<0 || LENGTH(response)>2) error("PANIC: Bad callback wrapper result! Triggr will dirty-collapse now.\n");
    killConnectionAftrSend=(LENGTH(response)==2);
    responseC=CHAR(STRING_ELT(response,0));
   }
   UNPROTECT(3);

   //WORK DONE
   processedJobs++;
   //Locking gqM to update the global state 
   pthread_mutex_lock(&gqM);
   lastDoneConnection=c;
   if(active && !termCon){
    lastResult=malloc(strlen(responseC)+1);
    strcpy(lastResult,responseC);
   } else lastResult=NULL;
   working=0;
   WB->working=0;
   lastOrphaned=WB->orphaned;
   killWorkBuffer(WB);
   pthread_mutex_unlock(&gqM);
   
   //Notifying Triggr to initiate the output sending
   pthread_mutex_lock(&outSchedM);
   ev_async_send(lp,&idleAgain);
   //And wait till the idle callback ends
   pthread_cond_wait(&outSchedC,&outSchedM);
   pthread_mutex_unlock(&outSchedM);
   
   pthread_mutex_lock(&gqM);
  }
  pthread_mutex_unlock(&gqM);
  
 }
 //Wait for trigger thread
 pthread_join(thread,NULL);
 //Restore sigpipe
 signal(SIGPIPE,oldSigpipe);
 close(acceptFd);
 Rprintf("Clean exit of triggr. There was %d executed jobs.\n",processedJobs);
 return(R_NilValue);
}
