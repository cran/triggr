\name{getConID}
\alias{getConID}
\alias{session}
\title{Get the ID of a current connection}
\description{
At each connection, a conencted client is given an unique ID. This function, while executed inside a callback, returns the ID of a connection from witch the currently processed request came from. 
}
\usage{
getConID()
}

\details{
This function returns the unique ID of a currently connected client. In fact Nth connected client has an ID equal to N, however not all numbers may be seen by the callback because of the clients that disconnected (or were disconnected because of error) before completing a request, which are also being counted.

It can be used to make some kind of session management.
}

\note{
This function actually checks if it was called from callback and returns error if not.
}

\author{
Miron B. Kursa <M.Kursa@icm.edu.pl>
}


