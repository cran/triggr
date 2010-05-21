/* Connection object implementation, IO callbacks

   Copyright (c)2011 Miron Bartosz Kursa
 
   This file is part of triggr R package.

 Triggr is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 Triggr is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 You should have received a copy of the GNU General Public License along with triggr. If not, see http://www.gnu.org/licenses/. */


inline void rolloutOutBuffers(Connection* c){
 while(c->headOut!=NULL) killOutputBuffer(c->headOut);
}

inline void rolloutWorkBuffers(Connection* c){
 while(c->headWork!=NULL) killWorkBuffer(c->headWork);
}


inline void killConnection(Connection *c){
 //Killing connection removes all its outBuffers (even uncompleted, but those have no place to be transfered now)
 rolloutOutBuffers(c);
 //...and WorkBuffers; if one of them is the work currently done, it stays in memory with connection set to NULL yet removed from connection's work queue
 rolloutWorkBuffers(c);
 
 ev_io_stop(lp,&c->qWatch);
 ev_io_stop(lp,&c->aWatch);
 close(c->qWatch.fd);
 if(c==GlobalQueue.tailCon) GlobalQueue.tailCon=c->prv;
 if(c==GlobalQueue.headCon) GlobalQueue.headCon=c->nxt;
 if(c->prv) c->prv->nxt=c->nxt;
 if(c->nxt) c->nxt->prv=c->prv;
 curClients--;
 freeIB(&c->IB);
 free(c);
}

void tryResolveConnection(Connection* c){
 
 //Try to close and destroy connection if it is not needed
 if(c->headWork==NULL){
  //Ok, nothing is here to compute 
  if(c->headOut==NULL){
   if(!c->canRead){
    //printf("Clean exit of connection #%d\n",c->ID);
    killConnection(c);
   }else{
    //printf("Read still possible\n");
   } //else we can still get some message, so nothing is needed
  }else{
   //In theory we have something to send; but can we?
   if(!c->canWrite){
    //printf("Some-writes-lost exit of connection #%d\n",c->ID);
    killConnection(c);
   } //else we still can write, so nothing is needed
  }
 }else{
  //printf("Still work to do... Can't stop without it\n");
  if(!c->canWrite){
   //We have work, but we can't write -- connection killed, output orphaned
   ev_io_stop(lp,&c->aWatch);
   killConnection(c);
  } else if(!c->canRead){
   //We just can't read -- probably client just half-closed connection and is still receiving
   ev_io_stop(lp,&c->qWatch);
  }
  //else everything is ok and we just carry on with writing
 }
}

//This function copies its Null-terminated string what argument, so the
// original should be somehow removed manually.
void scheduleWork(const char *what,Connection* c){
 size_t size=strlen(what);
 WorkBuffer* WB=malloc(sizeof(WorkBuffer));
 if(WB){
  WB->c=c;
  WB->working=0;
  WB->orphaned=0;
  WB->buffer=malloc(size+10);
  if(WB->buffer){
   //WB->size=size; REDUND
   //WB->done=0; REDUND
   strcpy(WB->buffer,what);
   pthread_mutex_lock(&gqM);
   //Place on connection
   if(c->tailWork==NULL){
    c->tailWork=c->headWork=WB;
    WB->prv=WB->nxt=NULL;
   }else{
    WB->prv=c->tailWork;
    c->tailWork=WB;
    WB->prv->nxt=WB;
    WB->nxt=NULL;
   }
   //Place on globalQueue
   if(GlobalQueue.tailWork==NULL){
    //Currently this is the only work
    GlobalQueue.tailWork=GlobalQueue.headWork=WB;
    WB->globalPrv=WB->globalNxt=NULL;
   }else{
    WB->globalPrv=GlobalQueue.tailWork;
    GlobalQueue.tailWork=WB;
    WB->globalPrv->globalNxt=WB;
    WB->globalNxt=NULL;
   }   
   //Queues/buffers updated
   pthread_mutex_unlock(&gqM);
     
   //Fire if idling
   pthread_mutex_lock(&idleM);
   if(!working){
    //Signal about the work
    pthread_cond_signal(&idleC);
   }//Else, this work will be fired on finishing currently done work
   pthread_mutex_unlock(&idleM); 
  }//Else scheduling just failed because of OOM
 }//Else ditto
    //TODO: Two above should kill the connection.
}

static void cbRead(struct ev_loop *lp,struct ev_io *this,int revents){
 Connection *c;
 c=this->data;
 tillRNRN(this->fd,&(c->IB));
 if(c->IB.state==1){
  scheduleWork(c->IB.buffer,c);
  freeIB(&c->IB);
  makeIB(&c->IB);
  return;
 }
 if(c->IB.state==2){
  close(this->fd);
 }
 if(c->IB.state>1){
  c->canRead=0;
  ev_io_stop(lp,this);
  pthread_mutex_lock(&gqM);
  tryResolveConnection(c);
  pthread_mutex_unlock(&gqM);
  return;
 }
}

static void cbWrite(struct ev_loop *lp,ev_io *this,int revents){
 Connection* c;
 c=this->data;
 pthread_mutex_lock(&gqM);
 OutBuffer *o=c->headOut;
 pthread_mutex_unlock(&gqM);
 
 if(o==NULL){
  //Queue is emty -- stop this watcher
  ev_io_stop(lp,this);
 }else{
  //Continue streaming outs
  o->streamming=1;
  int written=write(this->fd,o->buffer+o->alrSent,o->size-o->alrSent);
  if(written<0){
   if(errno!=EAGAIN){
    c->canWrite=0;
    pthread_mutex_lock(&gqM);
    tryResolveConnection(c);
    pthread_mutex_unlock(&gqM);
   }
  }else{
   o->alrSent+=written;
   if(o->alrSent==o->size){
    if(o->killAfter) c->canWrite=c->canRead=0;
    pthread_mutex_lock(&gqM);
    killOutputBuffer(o);
    tryResolveConnection(c);
    pthread_mutex_unlock(&gqM);
   }
  }
 } 
}

static void cbAccept(struct ev_loop *lp,ev_io *this,int revents){
 struct sockaddr_in clientAddr;
 socklen_t cliLen=sizeof(clientAddr);
 int conFd;
 conFd=accept(this->fd,(struct sockaddr*)&(clientAddr),&cliLen);
 if(conFd<0) return;
 if(fcntl(conFd,F_SETFL,fcntl(conFd,F_GETFL,0)|O_NONBLOCK)==-1){
  //Something wrong with connection, just skipping request
  close(conFd);
  return;  
 }
 Connection *connection; 
 connection=malloc(sizeof(Connection));
 if(!connection){
  //No memory, just skipping this request
  close(conFd);
  return;
 }
 
 //Put on GlobalQueue
 pthread_mutex_lock(&gqM);
 if(GlobalQueue.tailCon==NULL){
  //Currently this is the only connection
  GlobalQueue.tailCon=GlobalQueue.headCon=connection;
  connection->prv=connection->nxt=NULL;
 }else{
  connection->prv=GlobalQueue.tailCon;
  GlobalQueue.tailCon=connection;
  connection->prv->nxt=connection;
  connection->nxt=NULL;
 }
 connection->ID=GlobalQueue.curCon++;
 
 //Clear local queues
 connection->headOut=connection->tailOut=connection->headWork=connection->tailWork=NULL;
 connection->canRead=1;
 connection->canWrite=1;
 //Make IB for messages
 makeIB(&(connection->IB));
 if(!connection->IB.buffer){
  //Same as above
  free(connection);
  close(conFd);
  return;
 }
 //So we have client accepted; let's hear what it wants to tell us
 ev_io_init(&connection->qWatch,cbRead,conFd,EV_READ);
 connection->qWatch.data=(void*)connection;
 
 ev_io_init(&connection->aWatch,cbWrite,conFd,EV_WRITE);
 connection->aWatch.data=(void*)connection;
 
 ev_io_start(lp,&connection->qWatch); 
 
 clients++;
 curClients++;
 
 //Make global queue acessible again
 pthread_mutex_unlock(&gqM);
} 

