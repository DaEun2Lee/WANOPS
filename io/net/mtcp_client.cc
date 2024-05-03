/*
 * made by delee
 */

#include <event/event_callback.h>
#include <event/event_system.h>

#include <io/socket/socket.h>

#include <io/net/tcp_client.h>

mTCPClient::mTCPClient(SocketImpl impl, SocketAddressFamily family)
: log_("/tcp/client"),
  mtx_("mTCPClient"),
  impl_(impl),
  family_(family),
  socket_(NULL),
  close_complete_(NULL, &mtx_, this, &mTCPClient::close_complete),
  close_action_(NULL),
  connect_complete_(NULL, &mtx_, this, &mTCPClient::connect_complete),
  connect_cancel_(&mtx_, this, &mTCPClient::connect_cancel),
  connect_action_(NULL),
  connect_callback_(NULL)
{ }

mTCPClient::~mTCPClient()
{
	ASSERT_NULL(log_, connect_action_);
	ASSERT_NULL(log_, connect_callback_);
	ASSERT_NULL(log_, close_action_);

	if (socket_ != NULL) {
		delete socket_;
		socket_ = NULL;
	}
}

Action *
mTCPClient::connect(const std::string& iface, const std::string& name, SocketEventCallback *ccb)
{
	ScopedLock _(&mtx_);
	ASSERT_NULL(log_, connect_action_);
	ASSERT_NULL(log_, connect_callback_);
	ASSERT_NULL(log_, socket_);

	socket_ = Socket::create(impl_, family_, SocketTypeStream, "tcp", name);
	if (socket_ == NULL) {
		ccb->param(Event::Error, NULL);
		Action *a = ccb->schedule();

		delete this;

		return (a);
	}

	if (iface != "" && !socket_->bind(iface)) {
		ccb->param(Event::Error, NULL);
		Action *a = ccb->schedule();

		/*
		 * XXX
		 * I think maybe just pass the Socket up to the caller
		 * and make them close it?
		 *
		 * NB:
		 * Not duplicating this to other similar code.
		 */
#if 0
		/*
		 * XXX
		 * Need to schedule close and then delete ourselves.
		 */
		delete this;
#else
		HALT(log_) << "Socket bind failed.";
#endif

		return (a);
	}

	connect_action_ = socket_->connect(name, &connect_complete_);
	connect_callback_ = ccb;

	return (&connect_cancel_);
}

void
TCPClient::connect_cancel(void)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	ASSERT_NULL(log_, close_action_);
	ASSERT_NON_NULL(log_, connect_action_);

	connect_action_->cancel();
	connect_action_ = NULL;

	if (connect_callback_ != NULL) {
		connect_callback_ = NULL;
	} else {
		/* XXX This has a race; caller could cancel after we schedule, but before callback occurs.  */
		/* Caller consumed Socket.  */
		socket_ = NULL;

		EventSystem::instance()->destroy(&mtx_, this);
		return;
	}

	ASSERT_NON_NULL(log_, socket_);
	close_action_ = socket_->close(&close_complete_);
}

void
mTCPClient::connect_complete(Event e)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	connect_action_->cancel();
	connect_action_ = NULL;

	connect_callback_->param(e, socket_);
	connect_action_ = connect_callback_->schedule();
	connect_callback_ = NULL;
}

void
mTCPClient::close_complete(void)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	close_action_->cancel();
	close_action_ = NULL;

	delete this;
}

Action *
mTCPClient::connect(SocketImpl impl, SocketAddressFamily family, const std::string& name, SocketEventCallback *cb)
{
	mTCPClient *tcp = new mTCPClient(impl, family);
	return (tcp->connect("", name, cb));
}

Action *
mTCPClient::connect(SocketImpl impl, SocketAddressFamily family, const std::string& iface, const std::string& name, SocketEventCallback *cb)
{
	mTCPClient *tcp = new mTCPClient(impl, family);
	return (tcp->connect(iface, name, cb));
}
