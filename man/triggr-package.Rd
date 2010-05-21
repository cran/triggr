\name{triggr-package}
\alias{triggr-package}
\alias{triggr}
\docType{package}
\title{Triggr}

\description{
Triggr allows to use R by network via simple and light RPC protocol. It queues requests and routes responses, thus supports multiple simulatenous client connections sending multiple requests while still executing jobs sequentially in the same R environment. Unlike Rserve, triggr does not evaluate requests directly, rather executes a user-defined R function on them.

}
\details{
\tabular{ll}{
Package: \tab triggr\cr
Type: \tab Package\cr
Version: \tab 0.1\cr
Date: \tab 2010-05-21\cr
License: \tab LICENCE\cr
LazyLoad: \tab yes\cr
}

Unlike other similar solutions, triggr is not a standalone application spawning isolated R environments but a controller of a R session it was started in. Namely, it blocks the interactive console and and creates a thread (later called `trigger`) that hadles network connections and queue. When the queue is not empty, the trigger releases the original thread and makes it execute the oldest request by calling a user-defined callback R function on it. The result is than sent to the appropriate client, again by the trigger thread. 

Triggr communication protocol is intended to be simple and general enough to provide good performance and mimic other popular protocols, including great subset of HTTP. Namely, triggr expects a stream of plain-text messages separated by blank lines, i.e. \code{\\r\\n\\r\\n} terminators (\code{\\r\\n} is a standard line terminator for network use; it is also used by Windows, unlike standard UNIX \code{\\n} terminator). Each such incomming message is treated as a request and queued; server is not blocking after reading a message, so one should not rely on this for synchronisation. The results of computation are sent in the same manner. The server only guarantees that the responses will be sent in the same order as their requests -- the callback may decide to break the connection at all times. When the connection is broken (regardless wheather it was closed by client, server or system), all its queued requests and responses are wiped out. However, the request being processed when a disconnect occured is not stopped and continues untill callback finishes; yet its result is discarded.

The server can be started using the \code{serve} function.


}
\author{
Miron B. Kursa <M.Kursa@icm.edu.pl>
}

\keyword{package}
