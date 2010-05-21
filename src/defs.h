/* Object definitions and global variables

   Copyright (c)2011 Miron Bartosz Kursa
 
   This file is part of triggr R package.

 Triggr is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 Triggr is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 You should have received a copy of the GNU General Public License along with triggr. If not, see http://www.gnu.org/licenses/. */

struct _Connection;
typedef struct _Connection Connection;

struct _OutBuffer;
typedef struct _OutBuffer OutBuffer;

struct _WorkBuffer;
typedef struct _WorkBuffer WorkBuffer;

struct _InBuffer;
typedef struct _InBuffer InBuffer;

struct _OutBuffer{
 char* buffer;
 size_t size;
 size_t alrSent;
 int streamming;
 int killAfter;
 OutBuffer* nxt;
 OutBuffer* prv;
 Connection* c;
};


struct _InBuffer{
 char *buffer;
 size_t actualSize;
 size_t truSize;
 int state; //0-uncompleted, 1-finished, 2-error, 3-eof
};

struct _WorkBuffer{
 char* buffer;
 int working;
 Connection* c;
 int orphaned;
 WorkBuffer* nxt;
 WorkBuffer* prv;
 WorkBuffer* globalNxt;
 WorkBuffer* globalPrv;
};

struct _Connection{
 struct ev_io qWatch;
 struct ev_io aWatch;
 Connection* prv;
 Connection* nxt;
 OutBuffer* tailOut;
 OutBuffer* headOut;
 WorkBuffer* tailWork;
 WorkBuffer* headWork;
 int canRead;
 int canWrite;
 InBuffer IB;
 int ID;
}; 


int working=0;
int active=0;
int port;

int count=0;
int clients=0;
int curClients=0;
int curConID;
int killConnectionAftrSend=0;

int acceptFd;

int processedJobs;

struct ev_loop *lp;
struct ev_async idleAgain;

//Block the main thread during nothing-to-be-processed
pthread_cond_t idleC=PTHREAD_COND_INITIALIZER;
pthread_mutex_t idleM=PTHREAD_MUTEX_INITIALIZER;

//Block the main thread until async event will not convert the result into OutBuffer
pthread_cond_t outSchedC=PTHREAD_COND_INITIALIZER;
pthread_mutex_t outSchedM=PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t gqM=PTHREAD_MUTEX_INITIALIZER;

Connection *lastDoneConnection;
char *lastResult;
int lastOrphaned;
int termCon;

//Global state object
struct{
 WorkBuffer* tailWork;
 WorkBuffer* headWork;
 Connection* tailCon;
 Connection* headCon;
 int curCon;
} GlobalQueue;

void tryResolveConnection(Connection* c);


