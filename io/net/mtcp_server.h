/*
 * made by delee
 */

#ifndef	IO_NET_TCP_SERVER_H
#define	IO_NET_TCP_SERVER_H

#include <io/socket/mtcp_socket.h>


class mTCPServer { 
	LogHandle log_;
	m_Socket *m_socket_;
	
	/*
	@delee
	Check mctx 
	(mtx)
	*/
	size_t mctx_;
	//pleasae check size_t 
	// @delee 
	// Need to change cpu related multi thread 


	mTCPServer(Socket *m_socket)
	: log_("/mtcp/server"),
	mctx_ = mtcp_create_context(cpu); 
	socket_(m_socket)
	{ }

public:
	~mTCPServer()
	{
		// if (m_socket_ != NULL) {
		// 	delete m_socket_;
		// 	m_socket_ = NULL;
		// }
		mtcp_destroy_context(mctx_t mctx_);
	}

	Action *accept(SocketEventCallback *cb)
	{
		
		return (m_socket_->accept(cb));
	}

	Action *close(SimpleCallback *cb)
	{
		return (m_socket_->close(cb));
	}

	std::string getsockname(void) const
	{
		return (m_socket_->getsockname());
	}

	static mTCPServer *listen(SocketImpl, SocketAddressFamily, const std::string&);
};

#endif /* !IO_NET_TCP_SERVER_H */
