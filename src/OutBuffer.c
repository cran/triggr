/* Output buffer object implementation

   Copyright (c)2011 Miron Bartosz Kursa
 
   This file is part of triggr R package.

 Triggr is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 Triggr is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 You should have received a copy of the GNU General Public License along with triggr. If not, see http://www.gnu.org/licenses/. */

void killOutputBuffer(OutBuffer *o){
 Connection *c=o->c;
 if(o->buffer) free(o->buffer);
 if(o==c->tailOut) c->tailOut=o->prv;
 if(o==c->headOut) c->headOut=o->nxt;
 if(o->prv!=NULL) o->prv->nxt=o->nxt;
 if(o->nxt!=NULL) o->nxt->prv=o->prv;
 free(o);
}

OutBuffer *makeOutputBuffer(const char *what,Connection *c,int killAfter){
 //gqM is locked outside this function
 if(!c->canWrite) tryResolveConnection(c);
 size_t size=strlen(what);
 OutBuffer* OB=malloc(sizeof(OutBuffer));
 if(OB){
  OB->c=c;
  OB->alrSent=0;
  OB->killAfter=killAfter;
  OB->streamming=0;
  OB->buffer=malloc(size+10);
  if(OB->buffer){
   strcpy(OB->buffer,what);
   //Place on connection
   if(c->tailOut==NULL){
    c->tailOut=c->headOut=OB;
    OB->prv=OB->nxt=NULL;
   }else{
    OB->prv=c->tailOut;
    c->tailOut=OB;
    OB->prv->nxt=OB;
    OB->nxt=NULL;
   }
   OB->size=size;
   ev_io_start(lp,&c->aWatch);
   return(OB);
  }else{
   //OOM
   Rprintf("Out of memory when allocating OutBuffer (A)!\n");
   c->canWrite=0;
   tryResolveConnection(c);
   return(NULL);
  }
 }else{
  //OOM
  Rprintf("Out of memory when allocating OutBuffer (B)!\n");
  c->canWrite=0;
  tryResolveConnection(c);
  return(NULL);
 }
}

