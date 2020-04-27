#include "CoreUtils.hpp"

#include <stdexcept> // std::out_of_range

#include "Msgs.inl"

namespace YGOPro::CoreUtils
{

template<typename T>
inline void Write(uint8_t*& ptr, T value)
{
	std::memcpy(ptr, &value, sizeof(T));
	ptr += sizeof(T);
}

std::vector<Msg> SplitToMsgs(const Buffer& buffer)
{
	using length_t = uint32_t;
	static constexpr std::size_t sizeOfLength = sizeof(length_t);
	std::vector<Msg> msgs;
	if(buffer.empty())
		return msgs;
	const std::size_t bufSize = buffer.size();
	const uint8_t* const bufData = buffer.data();
	for(std::size_t pos = 0; pos != bufSize; )
	{
		// Retrieve length of this message
		if(pos + sizeOfLength > bufSize)
			throw std::out_of_range("SplitMessages: Reading length");
		length_t length;
		std::memcpy(&length, bufData + pos, sizeOfLength);
		pos += sizeOfLength;
		// Copy message data to a new message
		if(pos + length > bufSize)
			throw std::out_of_range("SplitMessages: Reading message data");
		auto& msg = msgs.emplace_back(length);
		std::memcpy(msg.data(), bufData + pos, length);
		pos += length;
	}
	return msgs;
}

uint8_t GetMessageType(const Msg& msg)
{
	return msg.at(0);
}

MsgDistType GetMessageDistributionType(const Msg& msg)
{
	switch(GetMessageType(msg))
	{
	case MSG_SELECT_BATTLECMD:
	case MSG_SELECT_IDLECMD:
	case MSG_SELECT_EFFECTYN:
	case MSG_SELECT_YESNO:
	case MSG_SELECT_OPTION:
	case MSG_SELECT_CARD:
	case MSG_SELECT_CHAIN:
	case MSG_SELECT_PLACE:
	case MSG_SELECT_DISFIELD:
	case MSG_SELECT_POSITION:
	case MSG_SELECT_TRIBUTE:
	case MSG_SORT_CHAIN:
	case MSG_SELECT_COUNTER:
	case MSG_SELECT_SUM:
	case MSG_SELECT_UNSELECT_CARD:
	case MSG_ROCK_PAPER_SCISSORS:
	case MSG_ANNOUNCE_RACE:
	case MSG_ANNOUNCE_ATTRIB:
	case MSG_ANNOUNCE_CARD:
	case MSG_ANNOUNCE_NUMBER:
	case MSG_ANNOUNCE_CARD_FILTER:
		return MsgDistType::MSG_DIST_TYPE_FOR_SPECIFIC_TEAM_DUELIST;
	case MSG_HINT:
	{
		// TODO
		[[fallthrough]];
	}
	default:
		return MsgDistType::MSG_DIST_TYPE_FOR_EVERYONE_WITHOUT_STRIPPING;
	}
}

uint8_t GetMessageReceivingTeam(const Msg& msg)
{
	return msg.at(1);
}

StrippedMsg StripMessageForTeam(uint8_t team, const Msg& msg)
{
	return msg; // TODO
}

const Msg& MsgFromStrippedMsg(const StrippedMsg& sMsg)
{
	if(const auto i = sMsg.index(); i == 0)
		return *std::get<0>(sMsg);
	else
		return std::get<1>(sMsg);
}

Msg MakeStartMsg(const MsgStartCreateInfo& info)
{
	Msg msg(18);
	auto ptr = msg.data();
	Write<uint8_t>(ptr, MSG_START);
	Write<uint8_t>(ptr, 0);
	Write<uint32_t>(ptr, info.lp);
	Write<uint32_t>(ptr, info.lp);
	Write(ptr, static_cast<uint16_t>(info.t0DSz));
	Write(ptr, static_cast<uint16_t>(info.t0EdSz));
	Write(ptr, static_cast<uint16_t>(info.t1DSz));
	Write(ptr, static_cast<uint16_t>(info.t1EdSz));
	return msg;
}

STOCMsg GameMsgFromMsg(const Msg& msg)
{
	return STOCMsg{STOCMsg::MsgType::GAME_MSG, msg};
}

} // namespace YGOPro::CoreUtils