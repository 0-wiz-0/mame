defines {
	"UNICODE",
	"_UNICODE",
	"OSD_WINDOWS",
	"USE_SDL=0",
	"main=utf8_main",
	"_WIN32_WINNT=0x0501",
}

if not _OPTIONS["DONT_USE_NETWORK"] then
	defines {
		"SDLMAME_NET_PCAP",
	}
end
