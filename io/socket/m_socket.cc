/*
 * Copyright (c) 2013 Patrick Kelsey. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <common/thread/mutex.h>

#include <event/event_callback.h>

#include <io/socket/socket_handle.h>

//@delee
#include <mtcp_api.h>

// #if defined(HAVE_UINET)
// #include <io/socket/socket_uinet.h>
// #endif

m_Socket *
m_Socket::create(mctx_t mctx, SocketAddressFamily family, SocketType type, const std::string& protocol, const std::string& hint)
{
	Socket *newSocket;

	switch (impl) {
	case SocketImplOS:
		newSocket = SocketHandle::create(family, type, protocol, hint);
		break;

#if defined(HAVE_UINET)
	case SocketImplUinet:
		newSocket = SocketUinet::create(family, type, protocol, hint);
		break;
#endif

	default:
		ERROR("/socket") << "Unsupported socket type.";
		return (NULL);
	}

	return (newSocket);
}


bool
m_Socket::m_bind(const std::string& name){
	socket_address addr;

	/*
		int mtcp_bind(mctx_t mctx, int sockid, const struct sockaddr *addr, socklen_t addrlen);

		- return_value
			- 0 : success
			- 1 : failure 
	*/
	
	result = mtcp_bind(mctx_t mctx, int sockid, const struct sockaddr *addr, socklen_t addrlen);

	if (result < 0){
		//mtcp_bind failure
		//errono has what kind of errors

		ERROR(log_) << "Could not bind: " << strerror(uinet_errno_to_os(error));

		switch(strerror(errno)){
		case EBADF:
			// sockid is not a valid socket descriptor for binding to an address.
			ERROR(log_) << "EBADF - Invalid socket descriptor for bind: " << name;
			break;
		
		case EINVAL:
			// The addr argument is NULL. This may also mean that an address is already bound to the current sockid descriptor. 
			ERROR(log_) << "EINVAL - address is NULL or already bound for bind: " << name;
			break;
		case ENOTSOCK:
			// The socket referred to by sockid has an invalid type.
			ERROR(log_) << "ENOTSOCK - invalid sockid for bind: " << name;
			break;
		}
	}
	return (true);
}