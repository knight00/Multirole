#include "Instance.hpp"


#include "../IRoomManager.hpp"

namespace Ignis::Multirole::Room
{

Instance::Instance(
	CreateInfo&& info,
	IRoomManager& owner,
	asio::io_context& ioCtx)
	:
	owner(owner),
	strand(ioCtx),
	ctx(std::move(info.hostInfo),
		std::move(info.limits),
		std::move(info.cpkg),
		info.banlist),
	state(State::Waiting{nullptr}),
	name(std::move(info.name)),
	notes(std::move(info.notes)),
	pass(std::move(info.pass))
{}

void Instance::RegisterToOwner()
{
	ctx.SetId(id = owner.Add(shared_from_this()));
}

bool Instance::Started() const
{
	return !std::holds_alternative<State::Waiting>(state);
}

bool Instance::CheckPassword(std::string_view str) const
{
	return pass.empty() || pass == str;
}

Instance::Properties Instance::GetProperties()
{
	return Properties
	{
		ctx.HostInfo(),
		notes,
		!pass.empty(),
		Started(),
		id,
		ctx.GetDuelistsNames()
	};
}

void Instance::TryClose()
{
	if(Started())
		return;
	for(auto& c : clients)
		c->Disconnect();
}

void Instance::Add(std::shared_ptr<Client> client)
{
	std::lock_guard<std::mutex> lock(mClients);
	clients.insert(client);
}

void Instance::Remove(std::shared_ptr<Client> client)
{
	std::lock_guard<std::mutex> lock(mClients);
	clients.erase(client);
	if(clients.empty())
	{
		asio::post(strand,
		[this, self = shared_from_this()]()
		{
			owner.Remove(id);
			// NOTE: Destructor of this Room instance is called here
		});
	}
}

asio::io_context::strand& Instance::Strand()
{
	return strand;
}

void Instance::Dispatch(EventVariant e)
{
	StateOpt newState = std::visit(ctx, state, e);
	if(newState)
	{
		state = *std::move(newState);
		std::visit(ctx, state);
	}
}

} // namespace Ignis::Multirole::Room