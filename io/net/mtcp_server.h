/*
 * made by delee
 */

#ifndef	IO_NET_TCP_SERVER_H
#define	IO_NET_TCP_SERVER_H

#include <io/socket/socket.h>


class mTCPServer {
	LogHandle log_;
	Socket *socket_;

	mTCPServer(Socket *socket)
	: log_("/tcp/server"),
	  socket_(socket)
	{ }

public:
	~mTCPServer()
	{
		if (socket_ != NULL) {
			delete socket_;
			socket_ = NULL;
		}
	}

	Action *accept(SocketEventCallback *cb)
	{
		return (socket_->accept(cb));
	}

	Action *close(SimpleCallback *cb)
	{
		return (socket_->close(cb));
	}

	std::string getsockname(void) const
	{
		return (socket_->getsockname());
	}

	static mTCPServer *listen(SocketImpl, SocketAddressFamily, const std::string&);
};

#endif /* !IO_NET_TCP_SERVER_H */
