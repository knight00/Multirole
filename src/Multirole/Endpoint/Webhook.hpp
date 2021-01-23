#ifndef WEBHOOKENDPOINT_HPP
#define WEBHOOKENDPOINT_HPP
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>

namespace Ignis::Multirole::Endpoint
{

class Webhook
{
public:
	Webhook(asio::io_context& ioCtx, unsigned short port);
	void Stop();

	virtual void Callback(std::string_view payload);
protected:
	inline ~Webhook() = default;
private:
	class Connection final : public std::enable_shared_from_this<Connection>
	{
	public:
		Connection(Webhook& webhook, asio::ip::tcp::socket socket);
		void DoReadHeader();
	private:
		Webhook& webhook;
		asio::ip::tcp::socket socket;
		std::array<char, 1U << 8U> incoming;

		void DoWrite();
		void DoReadEnd();
	};

	asio::ip::tcp::acceptor acceptor;

	void DoAccept();
};

} // namespace Ignis::Multirole::Endpoint

#endif // WEBHOOKENDPOINT_HPP
