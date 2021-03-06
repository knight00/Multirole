project('multirole', ['c', 'cpp'], default_options : 'cpp_std=c++17')

is_window = host_machine.system() == 'windows'

atomic_dep  = meson.get_compiler('cpp').find_library('atomic', required : false)
boost_dep   = dependency('boost', modules : ['filesystem', 'system'])
dl_dep      = meson.get_compiler('cpp').find_library('dl', required : false)
fmt_dep     = dependency('fmt')
libgit2_dep = dependency('libgit2')
njson_dep   = dependency('nlohmann_json', required : is_window)
rt_dep      = meson.get_compiler('cpp').find_library('rt', required : false)
spdlog_dep  = dependency('spdlog')
sqlite3_dep = dependency('sqlite3')
thread_dep  = dependency('threads')

multirole_src_files = files([
	'src/DLOpen.cpp',
	'src/Multirole/GitRepo.cpp',
	'src/Multirole/Instance.cpp',
	'src/Multirole/Lobby.cpp',
	'src/Multirole/main.cpp',
	'src/Multirole/STOCMsgFactory.cpp',
	'src/Multirole/Core/DLWrapper.cpp',
	'src/Multirole/Core/HornetWrapper.cpp',
	'src/Multirole/Endpoint/LobbyListing.cpp',
	'src/Multirole/Endpoint/RoomHosting.cpp',
	'src/Multirole/Endpoint/Webhook.cpp',
	'src/Multirole/Room/Client.cpp',
	'src/Multirole/Room/Context.cpp',
	'src/Multirole/Room/Instance.cpp',
	'src/Multirole/Room/TimerAggregator.cpp',
	'src/Multirole/Room/State/ChoosingTurn.cpp',
	'src/Multirole/Room/State/Closing.cpp',
	'src/Multirole/Room/State/Dueling.cpp',
	'src/Multirole/Room/State/Rematching.cpp',
	'src/Multirole/Room/State/RockPaperScissor.cpp',
	'src/Multirole/Room/State/Sidedecking.cpp',
	'src/Multirole/Room/State/Waiting.cpp',
	'src/Multirole/Service/BanlistProvider.cpp',
	'src/Multirole/Service/CoreProvider.cpp',
	'src/Multirole/Service/DataProvider.cpp',
	'src/Multirole/Service/ReplayManager.cpp',
	'src/Multirole/Service/ScriptProvider.cpp',
	'src/Multirole/YGOPro/Banlist.cpp',
	'src/Multirole/YGOPro/CardDatabase.cpp',
	'src/Multirole/YGOPro/CoreUtils.cpp',
	'src/Multirole/YGOPro/Deck.cpp',
	'src/Multirole/YGOPro/Replay.cpp',
	'src/Multirole/YGOPro/StringUtils.cpp',
	'src/Multirole/YGOPro/LZMA/Alloc.c',
	'src/Multirole/YGOPro/LZMA/LzFind.c',
	'src/Multirole/YGOPro/LZMA/LzmaEnc.c'
])

hornet_src_files = files([
	'src/DLOpen.cpp',
	'src/Hornet/main.cpp'
])

executable('multirole', multirole_src_files,
	c_args: [
		'-D_7ZIP_ST',
		'-D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS',
		'-D_WINSOCK_DEPRECATED_NO_WARNINGS',
		'-DNOMINMAX'
	],
	cpp_args: [
		'-DSPDLOG_FMT_EXTERNAL',
		'-DBOOST_DATE_TIME_NO_LIB'
	],
	dependencies: [
		atomic_dep,
		boost_dep,
		dl_dep,
		fmt_dep,
		libgit2_dep,
		njson_dep,
		rt_dep,
		spdlog_dep,
		sqlite3_dep,
		thread_dep
	])

executable('hornet', hornet_src_files,
	cpp_args: [
		'-DBOOST_DATE_TIME_NO_LIB',
		'-DNOMINMAX'
	],
	dependencies: [
		boost_dep,
		dl_dep,
		rt_dep
	])
