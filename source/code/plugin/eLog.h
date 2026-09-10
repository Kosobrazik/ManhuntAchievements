#pragma once


class eLog {
public:
	static wchar_t path[260];

	static void Initialise();
	static void Message(const char* function, const char* format, ...);
	// Only written when Log=2. Meant for a single troubleshooting run: it names
	// every object a level holds and is far too chatty to leave switched on.
	static bool Detailed();
	static void Verbose(const char* function, const char* format, ...);
};
