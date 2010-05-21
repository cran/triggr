\name{endConnection}
\alias{endConnection}
\title{Close the current connection}
\description{
\code{endConnection} function can be called from a callback function to end the connection with the client that sent the currently processed request. It can be done either immediately or just after sending a response.
}
\usage{
endConnection(x)
}
%- maybe also 'usage' for other objects documented here.
\arguments{
  \item{x}{Character vector being a response to the current request. This argument may be provided or not; if it is, the server first sends it to the client and then disconnects.}
}


\details{This function produces a special value that must be returned unchanged from a callback function to disconnect the client that sent the currently processed request. If it is given a character argument, it sends it to client just like simple \code{return} would, however it forces server to disconnect just after this response is sent.
}

\note{
This function simply returns 9L (in fact any non-zero integer can be used) if no argument is provided or its argument with class attribute set to 'end-connection'. 
}



\author{
Miron B. Kursa <M.Kursa@icm.edu.pl>
}


