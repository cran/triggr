\name{stopServer}
\alias{stopServer}
\title{Stopping the triggr server}
\description{
\code{stopServer} function can be called from callback function to stop the triggr server.
}
\usage{
stopServer()
}
%- maybe also 'usage' for other objects documented here.


\details{This function produces a special value that must be returned unchanged from a callback function to stop the triggr server and return to regular interactive session. This is the only way to cleanly stop triggr. Simple calling \code{stopServer} has no side effects.
}

\note{
This function simply returns \code{0L}, so one may simply write \code{return(0L)} to get the same effect.
}


\author{
Miron B. Kursa <M.Kursa@icm.edu.pl>
}


