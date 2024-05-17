/*
 * Copyright (c) 2013 Juli Mallett. All rights reserved.
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

#ifndef	IO_SOCKET_SOCKET_H
#define	IO_SOCKET_SOCKET_H

#include <event/event.h>
#include <event/typed_pair_callback.h>

#include <io/channel.h>
#include <io/socket/socket_types.h>

//@delee
#include <mtcp_api.h>

typedef	class TypedPairCallback<Event, m_Socket *> SocketEventCallback;

class m_Socket : public StreamChannel {
protected:
	int domain_;
	int socktype_;
	int protocol_;

	m_Socket(int domain, int socktype, int protocol)
	: domain_(domain),
	  socktype_(socktype),
	  protocol_(protocol)

	  //mtcp_init()
	{ }
public:
	static ~m_Socket(){
		/*
		@delee
		void mtcp_destroy();
		*/
		mtcp_destroy();
	}

	static m_Socket *create(mctx_t mctx, SocketAddressFamily, SocketType, const std::string& = "", const std::string& = "");
	static Action *accept(SocketEventCallback *) = 0;
	static bool bind(const std::string&) = 0;
	static Action *connect(const std::string&, EventCallback *) = 0;
	static bool listen(void) = 0;

	static std::string getpeername(void) const = 0;
	static std::string getsockname(void) const = 0;
};

#endif /* !IO_SOCKET_SOCKET_H */
