#pragma once
#include <Windows.h>
#include <cstdlib>
#include <cstring>
#include <string>

// A translation of the gallery for a language the mod does not ship -- a fan
// translation through MHLL, say. One UTF-8 text file per language in
// data\ManhuntAchievements\lang:
//
//   Match = TIPO DE CONTROLE          the game's own "control type" label in
//                                     that language, so the file follows the
//                                     game when its language changes
//   Gallery.Title = CONQUISTAS        the gallery's own words (see kPhraseKeys)
//
//   1. Psicopata                      then the achievements, numbered in the
//   Descrição: Desbloqueie tudo.      gallery's order, which is the PS4 trophy
//                                     list's: a name, and on the next line its
//                                     description, a "Label:" in front of it
//                                     being dropped
//
// So a trophy list copied from a website goes in nearly as it is. Blank lines,
// lines starting with #, ; or [ (the list's own [BRONZE] headings) and anything
// before the first number are ignored. What a file leaves out stays English.
namespace AchievementTranslation
{
	constexpr int kAchievementCount = 33;

	enum Phrase
	{
		Title, TitleCount, Unlocked, KeyboardSelect, KeyboardPages, Select, Pages, Back,
		PhraseCount
	};

	inline const char* const kPhraseKeys[PhraseCount] = {
		"Gallery.Title", "Gallery.TitleCount", "Gallery.Unlocked",
		"Gallery.KeyboardSelect", "Gallery.KeyboardPages",
		"Gallery.Select", "Gallery.Pages", "Gallery.Back",
	};

	struct Translation
	{
		std::wstring match;
		std::wstring phrases[PhraseCount];
		std::wstring names[kAchievementCount];
		std::wstring descriptions[kAchievementCount];
		int count = 0;
	};

	inline std::wstring Widen(const std::string& utf8)
	{
		if (utf8.empty())
			return {};
		const int size = MultiByteToWideChar(CP_UTF8, 0, utf8.data(),
			static_cast<int>(utf8.size()), nullptr, 0);
		std::wstring result(static_cast<size_t>(size), L'\0');
		MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()),
			result.data(), size);
		return result;
	}

	inline std::string Trim(const std::string& text)
	{
		const size_t first = text.find_first_not_of(" \t\r\n");
		if (first == std::string::npos)
			return {};
		const size_t last = text.find_last_not_of(" \t\r\n");
		return text.substr(first, last - first + 1);
	}

	// "Descrição: text" -> "text". Only a single word before the colon counts as
	// a label, so a description that itself contains a colon keeps it.
	inline std::string DropLabel(const std::string& line)
	{
		const size_t colon = line.find(':');
		if (colon == std::string::npos || colon == 0 || colon > 24)
			return line;
		if (line.substr(0, colon).find(' ') != std::string::npos)
			return line;
		return Trim(line.substr(colon + 1));
	}

	inline void Set(std::wstring& slot, const std::string& value, int& count)
	{
		if (value.empty())
			return;
		if (slot.empty())
			++count;
		slot = Widen(value);
	}

	inline Translation Parse(std::string text)
	{
		Translation result;
		if (text.size() >= 3 && static_cast<unsigned char>(text[0]) == 0xEF &&
			static_cast<unsigned char>(text[1]) == 0xBB && static_cast<unsigned char>(text[2]) == 0xBF)
			text.erase(0, 3);

		int current = -1;               // The achievement whose name came last.
		bool expectDescription = false;
		size_t start = 0;
		while (start <= text.size())
		{
			size_t end = text.find('\n', start);
			if (end == std::string::npos)
				end = text.size();
			const std::string line = Trim(text.substr(start, end - start));
			start = end + 1;
			if (line.empty() || line[0] == '#' || line[0] == ';' || line[0] == '[')
				continue;

			// Key = value lines: Match and the gallery's words.
			const size_t equals = line.find('=');
			if (equals != std::string::npos)
			{
				const std::string key = Trim(line.substr(0, equals));
				const std::string value = Trim(line.substr(equals + 1));
				if (_stricmp(key.c_str(), "Match") == 0)
				{
					result.match = Widen(value);
					continue;
				}
				bool phrase = false;
				for (int index = 0; index < PhraseCount; ++index)
				{
					if (_stricmp(key.c_str(), kPhraseKeys[index]) == 0)
					{
						Set(result.phrases[index], value, result.count);
						phrase = true;
						break;
					}
				}
				if (phrase)
					continue;
			}

			// "12. Name": a number, a point or a bracket, then the name.
			size_t digits = 0;
			while (digits < line.size() && line[digits] >= '0' && line[digits] <= '9')
				++digits;
			if (digits > 0 && digits < 4 && digits + 1 < line.size() &&
				(line[digits] == '.' || line[digits] == ')') && line[digits + 1] == ' ')
			{
				const int number = std::atoi(line.substr(0, digits).c_str());
				current = number >= 1 && number <= kAchievementCount ? number - 1 : -1;
				expectDescription = current >= 0;
				if (current >= 0)
					Set(result.names[current], Trim(line.substr(digits + 1)), result.count);
				continue;
			}

			if (expectDescription)
			{
				Set(result.descriptions[current], DropLabel(line), result.count);
				expectDescription = false;
			}
		}
		return result;
	}
}
