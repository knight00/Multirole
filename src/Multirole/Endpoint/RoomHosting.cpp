#include "RoomHosting.hpp"

#include <type_traits> // std::remove_extent

#include <asio/read.hpp>
#include <asio/write.hpp>

#include "../Lobby.hpp"
#include "../STOCMsgFactory.hpp"
#include "../Room/Client.hpp"
#include "../Room/Instance.hpp"
#include "../YGOPro/CTOSMsg.hpp"
#include "../YGOPro/StringUtils.hpp"

namespace Ignis::Multirole::Endpoint
{

constexpr YGOPro::ClientVersion SERVER_VERSION =
{
	{
		38, // NOLINT: Client version major
		1,  // NOLINT: Client version minor
	},
	{
		8,  // NOLINT: Core version major
		0   // NOLINT: Core version minor
	}
};
constexpr uint32_t HANDSHAKE = 4043399681U;

constexpr bool operator!=(
	const YGOPro::ClientVersion& v1,
	const YGOPro::ClientVersion& v2)
{
	return std::tie(
		v1.client.major,
		v1.client.minor,
		v1.core.major,
		v1.core.minor)
		!=
		std::tie(
		v2.client.major,
		v2.client.minor,
		v2.core.major,
		v2.core.minor);
}

constexpr YGOPro::DeckLimits LimitsFromFlags(uint16_t flag)
{
	const bool doubleDeck = (flag & YGOPro::EXTRA_RULE_DOUBLE_DECK) != 0;
	const bool limit20 = (flag & YGOPro::EXTRA_RULE_DECK_LIMIT_20) != 0;
	YGOPro::DeckLimits l; // initialized with official values
	if(doubleDeck && limit20)
	{
		// NOTE: main deck boundaries are same as official
		l.extra.max = 10;
		l.side.max = 12;
	}
	else if(doubleDeck)
	{
		l.main.min = l.main.max = 100;
		l.extra.max = 30;
		l.side.max = 30;
	}
	else if(limit20)
	{
		l.main.min = 20; l.main.max = 30;
		l.extra.max = 5;
		l.side.max = 6;
	}
	return l;
}

// Holds information about the client before a proper connection to a Room
// has been established.
struct TmpClient
{
	asio::ip::tcp::socket soc;
	YGOPro::CTOSMsg msg;
	std::string name;

	explicit TmpClient(asio::ip::tcp::socket soc) : soc(std::move(soc))
	{}
};

// public

RoomHosting::RoomHosting(
	asio::io_context& ioCtx,
	unsigned short port,
	CoreProvider& coreProvider,
	BanlistProvider& banlistProvider,
	Lobby& lobby)
	:
	ioCtx(ioCtx),
	acceptor(ioCtx, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
	coreProvider(coreProvider),
	banlistProvider(banlistProvider),
	lobby(lobby)
{
	DoAccept();
}

void RoomHosting::Stop()
{
	acceptor.close();
	std::lock_guard<std::mutex> lock(mTmpClients);
	std::error_code ignore;
	for(auto& c : tmpClients)
	{
		c->soc.shutdown(asio::ip::tcp::socket::shutdown_both, ignore);
		c->soc.close(ignore);
	}
}

// private

void RoomHosting::Add(const std::shared_ptr<TmpClient>& tc)
{
	std::lock_guard<std::mutex> lock(mTmpClients);
	tmpClients.insert(tc);
}

void RoomHosting::Remove(const std::shared_ptr<TmpClient>& tc)
{
	std::lock_guard<std::mutex> lock(mTmpClients);
	tmpClients.erase(tc);
}

void RoomHosting::DoAccept()
{
	acceptor.async_accept(
	[this](const std::error_code& ec, asio::ip::tcp::socket soc)
	{
		if(!acceptor.is_open())
			return;
		if(!ec)
		{
			auto tc = std::make_shared<TmpClient>(std::move(soc));
			Add(tc);
			DoReadHeader(tc);
		}
		DoAccept();
	});
}

void RoomHosting::DoReadHeader(const std::shared_ptr<TmpClient>& tc)
{
	auto buffer = asio::buffer(tc->msg.Data(), YGOPro::CTOSMsg::HEADER_LENGTH);
	asio::async_read(tc->soc, buffer,
	[this, tc](const std::error_code& ec, std::size_t /*unused*/)
	{
		if(!ec && tc->msg.IsHeaderValid())
			DoReadBody(tc);
		else
			Remove(tc);
	});
}

void RoomHosting::DoReadBody(const std::shared_ptr<TmpClient>& tc)
{
	auto buffer = asio::buffer(tc->msg.Body(), tc->msg.GetLength());
	asio::async_read(tc->soc, buffer,
	[this, tc](const std::error_code& ec, std::size_t /*unused*/)
	{
		if(!ec && HandleMsg(tc))
			DoReadHeader(tc);
		else
			Remove(tc);
	});
}

bool RoomHosting::HandleMsg(const std::shared_ptr<TmpClient>& tc)
{
#define UTF16_BUFFER_TO_STR(a) \
	YGOPro::UTF16ToUTF8(YGOPro::BufferToUTF16(a, sizeof(decltype(a))))
	auto SendVersionError = [](asio::ip::tcp::socket& s)
	{
		static const auto m = STOCMsgFactory::MakeVersionError(SERVER_VERSION);
		asio::write(s, asio::buffer(m.Data(), m.Length()));
		return false;
	};
	auto& msg = tc->msg;
	switch(msg.GetType())
	{
	case YGOPro::CTOSMsg::MsgType::PLAYER_INFO:
	{
		auto p = msg.GetPlayerInfo();
		if(!p)
			return false;
		tc->name = UTF16_BUFFER_TO_STR(p->name);
		return true;
	}
	case YGOPro::CTOSMsg::MsgType::CREATE_GAME:
	{
		auto p = msg.GetCreateGame();
		if(!p || p->hostInfo.serverHandshake != HANDSHAKE || p->hostInfo.version != SERVER_VERSION)
			return SendVersionError(tc->soc);
		p->notes[199] = '\0'; // NOLINT: Guarantee null-terminated string
		Room::Instance::CreateInfo info;
		info.hostInfo = p->hostInfo;
		info.limits = LimitsFromFlags(info.hostInfo.extraRules);
		info.cpkg = coreProvider.GetCorePkg();
		info.banlist =
			banlistProvider.GetBanlistByHash(info.hostInfo.banlistHash);
		if(info.banlist == nullptr)
			info.hostInfo.banlistHash = 0;
		info.name = UTF16_BUFFER_TO_STR(p->name);
		info.notes = std::string(p->notes);
		info.pass = UTF16_BUFFER_TO_STR(p->pass);
		auto room = std::make_shared<Room::Instance>(
			std::move(info),
			lobby,
			ioCtx);
		room->RegisterToOwner();
		auto client = std::make_shared<Room::Client>(
			*room,
			std::move(tc->soc),
			std::move(tc->name));
		client->RegisterToOwner();
		client->Start(std::move(room));
		return false;
	}
	case YGOPro::CTOSMsg::MsgType::JOIN_GAME:
	{
		auto p = msg.GetJoinGame();
		if(!p || p->version != SERVER_VERSION)
			return SendVersionError(tc->soc);
		auto room = lobby.GetRoomById(p->id);
		if(room && room->CheckPassword(UTF16_BUFFER_TO_STR(p->pass)))
		{
			auto client = std::make_shared<Room::Client>(
				*room,
				std::move(tc->soc),
				std::move(tc->name));
			client->RegisterToOwner();
			client->Start(std::move(room));
		}
		return false;
	}
	default: return false;
	}
#undef UTF16_BUFFER_TO_STR
}

} // namespace Ignis::Multirole::Endpoint
