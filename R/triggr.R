#    R part of triggr
#
#    Copyright 2010 Miron B. Kursa
#
#    This file is part of triggr R package.
#
#Triggr is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
#Triggr is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#You should have received a copy of the GNU General Public License along with triggr. If not, see http://www.gnu.org/licenses/.


getConID<-function(){
 .Call(getCID);
}

serve<-function(callback,port=7777L){
 stopifnot(is.function(callback));
 stopifnot(length(port)==1);
 as.integer(port)->port; 
 .Call(startTrigger,port,
   function(x){
    try(callback(x))->y;
    if(is.integer(y)) return(y);
    if(!is.character(y) | class(y)=="try-error") return(9L);
    if(class(y)!="end-connection")
     y<-paste(paste(y,collapse="\r\n"),"\r\n\r\n",sep="") 
    else
     y<-c(paste(paste(y,collapse="\r\n"),"\r\n\r\n",sep=""),'');
    return(y);
   },new.env());
}

stopServer<-function() 0L

endConnection<-function(x){
 if(missing(x)) return(9L) else{
  class(x)<-"end-connection";
  return(x);
 }
}
