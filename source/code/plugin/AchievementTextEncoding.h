#pragma once

// How a text bank stores Russian, told from the menu strings the game returns.
// The 1C localization either returns real Cyrillic or keeps its CP1251 bytes
// widened to wchar_t -- and those sit exactly where Latin-1 keeps its accented
// letters. So a high character proves nothing on its own: a Portuguese fan
// translation's "CONFIGURAÇÖES" has two, and ManhuntGInput writes a button's
// name beside "Back" when its icons are off ("TRIÂNGULO"). Russian stored that
// way is made of such characters, though, so it takes most of the letters
// being high to call the bank Russian.
namespace AchievementTextEncoding
{
	enum class Kind
	{
		Latin,
		UnicodeRussian,
		WidenedCp1251Russian
	};

	struct LetterCounts
	{
		int unicodeCyrillic = 0;
		int widenedCp1251 = 0;
		int latin = 0;
	};

	inline void CountLetters(const wchar_t* text, LetterCounts& counts)
	{
		if (!text)
			return;
		for (; *text; ++text)
		{
			const wchar_t character = *text;
			if (character >= L'Ѐ' && character <= L'ӿ')
				++counts.unicodeCyrillic;
			// А-я and Ё/ё when the original CP1251 byte was simply widened.
			else if ((character >= L'À' && character <= L'ÿ' &&
				character != L'×' && character != L'÷') ||
				character == L'¨' || character == L'¸')
				++counts.widenedCp1251;
			else if ((character >= L'A' && character <= L'Z') ||
				(character >= L'a' && character <= L'z'))
				++counts.latin;
		}
	}

	inline Kind Classify(const LetterCounts& counts)
	{
		if (counts.unicodeCyrillic > 0)
			return Kind::UnicodeRussian;
		if (counts.widenedCp1251 > counts.latin)
			return Kind::WidenedCp1251Russian;
		return Kind::Latin;
	}
}
