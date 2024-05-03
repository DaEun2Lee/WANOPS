/*
 * made by delee
 */

#ifndef	IO_NET_TCP_CLIENT_H
#define	IO_NET_TCP_CLIENT_H

#include <common/thread/mutex.h>

#include <event/cancellation.h>

#include <io/socket/socket.h>

class mTCPClient {
	friend class DestroyThread;

	LogHandle log_;
	Mutex mtx_;
	SocketImpl impl_;
	SocketAddressFamily family_;
	Socket *socket_;

	SimpleCallback::Method<mTCPClient> close_complete_;
	Action *close_action_;

	EventCallback::Method<mTCPClient> connect_complete_;
	Cancellation<mTCPClient> connect_cancel_;
	Action *connect_action_;
	SocketEventCallback *connect_callback_;

	mTCPClient(SocketImpl, SocketAddressFamily);
	~mTCPClient();

	Action *connect(const std::string&, const std::string&, SocketEventCallback *);
	void connect_cancel(void);
	void connect_complete(Event);

	void close_complete(void);

public:
	static Action *connect(SocketImpl, SocketAddressFamily, const std::string&, SocketEventCallback *);
	static Action *connect(SocketImpl, SocketAddressFamily, const std::string&, const std::string&, SocketEventCallback *);
};

#endif /* !IO_NET_TCP_CLIENT_H */
