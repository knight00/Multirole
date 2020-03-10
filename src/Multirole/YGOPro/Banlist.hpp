#ifndef YGOPRO_BANLIST_HPP
#define YGOPRO_BANLIST_HPP
#include <cstdint>
#include <set>

namespace YGOPro
{

using CodeSet = std::set<uint32_t>;

class Banlist
{
public:
	Banlist(CodeSet whit, CodeSet semi, CodeSet limi, CodeSet forb);

	bool IsWhitelist() const;
	const CodeSet& Whitelist() const;
	const CodeSet& Semilimited() const;
	const CodeSet& Limited() const;
	const CodeSet& Forbidden() const;
private:
	CodeSet whitelist;
	CodeSet semilimited;
	CodeSet limited;
	CodeSet forbidden;
};

} // namespace Multirole

#endif // YGOPRO_BANLIST_HPP