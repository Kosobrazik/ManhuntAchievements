#include "AchievementMenu.h"
#include "ControllerUi.h"
#include "PluginCompatibility.h"
#include "HookSites.h"
#include "MenuBackInput.h"
#include "../core/AchievementSettings.h"
#include "../manhunt/Frontend.h"
#include "../manhunt/Text.h"
#include "../manhunt/Input.h"
#include "../manhunt/core.h"
#include "../manhunt/Filenames.h"
#include "../manhunt/Renderer.h"
#include "../manhunt/AudioManager.h"
#include "../manhunt/Misc.h"
#include "../manhunt/Cheats.h"
#include "../../MHWSF.h"
#include "eLog.h"
#include <cstring>
#include <cwchar>
#include <filesystem>
#include <string>
#include <vector>
#define ACHIEVEMENTS_PER_PAGE 6
namespace
{
	uintptr_t originalMenuDispatch=0, originalMenuBackground=0;
	uintptr_t originalMainMenuDraw=0, originalMainMenuInput=0;
	// Continuation for our option-registration detour: the game's own code just
	// past the whole overwritten prologue (four pushes and the stack reservation
	// the jump split in half), or another plugin's entry if it got there first.
	// Only the former needs the prologue replayed.
	uintptr_t originalMenuOption=0x5D55C7;
	bool replayMenuOptionPrologue=true;
	MenuBackInput galleryBack;
	bool waitForMainBackRelease=false;
	bool achievementPressHeld=false;
	bool RawKeyDown(int key)
	{
		// GInput publishes B/Back into the same native queue as keyboard Escape.
		if (!*reinterpret_cast<int*>(0x725710)) return false;
		const auto* keys=reinterpret_cast<const int*>(0x725698);
		for (int i=0;i<10;++i)
			if (keys[i*3]==key && keys[i*3+1]) return true;
		return false;
	}
	bool RawBackDown() { return RawKeyDown(1000); }
	void ResetGalleryBack()
	{
		galleryBack.Reset(RawBackDown(),CFrontend::GetInfoBarInput3()!=0,
			CPad::NewMouseControllerState.lmb!=0);
	}
	// Only used when no other plugin owns the main menu and we draw the whole
	// vanilla list ourselves. Another plugin's own rows are never reproduced here.
	enum MenuAction { Play, SelectScene, LoadGame, Settings, Bonus, Achievements, Quit };
	struct MainRow { const char* key; const wchar_t* english; const wchar_t* russian; MenuAction action; };
	const MainRow mainRows[] = {
		{"PLAY",nullptr,nullptr,Play}, {"SELSCE",nullptr,nullptr,SelectScene},
		{"LOADG",nullptr,nullptr,LoadGame}, {"SETT",nullptr,nullptr,Settings},
		{"BONFEA",nullptr,nullptr,Bonus},
		{nullptr,L"ACHIEVEMENTS",L"ДОСТИЖЕНИЯ",Achievements},
		{"QUITPRG",nullptr,nullptr,Quit}
	};
	constexpr int kMainRowCount = 7;

	bool IsRussianGameLocalization();
	std::wstring ToRussianGameWideText(const wchar_t* text);
	std::wstring AchievementRowLabel()
	{
		return IsRussianGameLocalization() ?
			ToRussianGameWideText(L"ДОСТИЖЕНИЯ") : L"ACHIEVEMENTS";
	}

	// Filled in while another plugin's main menu draw runs, so our appended row
	// inherits whatever layout that plugin chose without us knowing its settings.
	struct MenuOptionArgs { const wchar_t* text; float x, y, scaleX, scaleY; int selected; };
	bool g_countingMenuOptions = false;
	// Set once the option registration and text sites are both ours to take. Only
	// then can rows be moved to open a gap under the bonus features row.
	bool g_reorderMainMenu = false;
	int g_bonusRowIndex = -1;
	float g_achievementRowY = 0.0f;
	// Vertical offset applied to the row being registered. Both halves of the
	// game's AddOption receive the same coordinates, so both must shift alike.
	float g_currentRowShift = 0.0f;
	int g_foreignRowCount = 0;
	int g_foreignRowsLastFrame = 0;
	bool g_appendAchievementRow = false;
	MenuOptionArgs g_lastForeignRow = {};
	constexpr int ACHIEVEMENT_ICON_COUNT = 33;

	struct AchievementGalleryEntry
	{
		eAchievementID id;
		const char* name;
		const char* description;
		const wchar_t* russianName;
		const wchar_t* russianDescription;
	};

	const AchievementGalleryEntry ACHIEVEMENT_GALLERY[ACHIEVEMENT_ICON_COUNT] =
	{
		{ ACH_PSYCHOPATH, "Psychopath", "Unlock all achievements.",
			L"Психопат", L"Получите все достижения." },
		{ ACH_HE_NEVER_SAW_IT_COMING, "He Never Saw it Coming", "Perform a gruesome execution.",
			L"Он ничего не заметил", L"Совершите казнь третьего уровня жестокости." },
		{ ACH_5_STAR_KILLER, "5-Star Killer", "Earn a 5-star rating for any level.",
			L"Пятизвёздочный убийца", L"Получите пять звёзд за любой эпизод." },
		{ ACH_DRUG_FREE_IS_THE_WAY_TO_BE, "Drug Free is the Way to Be", "Complete any scene on Hardcore difficulty without using any painkillers.",
			L"Без таблеток лучше", L"Пройдите любой эпизод на сложности «Хардкор», не используя обезболивающие." },
		{ ACH_BANG_BANG_BOOM, "Bang, Bang, Boom", "Kill a hunter by shooting an explosive tank.",
			L"Бах, бах, бум!", L"Убейте охотника, выстрелив во взрывоопасный баллон." },
		{ ACH_NO_CRANE_NO_GAIN, "No Crane, No Gain", "Crush a hunter with a refrigerator.",
			L"Без крана нет победы", L"Раздавите охотника холодильником." },
		{ ACH_ARE_YOU_AFRAID_OF_THE_DARK, "Are You Afraid of the Dark?", "Hide a victim's body in the shadows.",
			L"Вы боитесь темноты?", L"Спрячьте тело жертвы в тени." },
		{ ACH_PINK_MIST, "Pink Mist", "Headshot a hunter with a sniper rifle.",
			L"Розовый туман", L"Убейте охотника выстрелом в голову из снайперской винтовки." },
		{ ACH_BRAIN_POWER, "Brain Power", "Use a severed head to lure a hunter.",
			L"Сила мозга", L"Используйте отрубленную голову, чтобы приманить охотника." },
		{ ACH_ENEMY_EFFICIENT, "Enemy Efficient", "Kill two hunters with one shotgun shell.",
			L"Экономия боеприпасов", L"Убейте двух охотников одним выстрелом из дробовика." },
		{ ACH_OOH_OOH_AAH_AAH, "Ooh, Ooh, Aah, Aah!", "Play any scene wearing the Monkey skin.",
			L"У-у, а-а!", L"Сыграйте любой эпизод в костюме обезьяны." },
		{ ACH_FOLLOW_THE_WHITE_RABBIT, "Follow the White Rabbit", "Complete any scene wearing the Rabbit skin.",
			L"Следуй за белым кроликом", L"Пройдите любой эпизод в костюме кролика." },
		{ ACH_DEATH_FROM_BEHIND, "Death from Behind", "Complete any scene using only executions.",
			L"Смерть со спины", L"Пройдите любой эпизод, используя только казни." },
		{ ACH_STICK_TO_THE_SHADOWS, "Stick to the Shadows", "Complete any scene going completely undetected.",
			L"Держись в тени", L"Пройдите любой эпизод, оставшись полностью незамеченным." },
		{ ACH_LINE_EM_UP_KNOCK_EM_DOWN, "Line 'Em Up, Knock 'Em Down", "Kill 30 hunters in Hard as Nails.",
			L"Поставь и уложи", L"Убейте 30 охотников в бонусном эпизоде «Прочный как гвозди»." },
		{ ACH_FUN_WITH_FISTICUFFS, "Fun With Fisticuffs", "Survive until 30 hunters have been killed in the Brawl Game.",
			L"Кулачные забавы", L"Доживите до гибели 30 охотников в бонусном эпизоде «Скандальная игра»." },
		{ ACH_APE_ESCAPE, "Ape Escape", "Escape the Zoo alive in Monkey See, Monkey Die!",
			L"Побег обезьяны", L"Выберитесь живым из зоопарка в эпизоде «Обезьянка видит, обезьянка умирает!»." },
		{ ACH_YOURE_GOING_NOWHERE, "You're Going Nowhere!", "Kill all the Hoods in Time 2 Die in seven minutes or less.",
			L"Вам не уйти!", L"Убейте всех Капюшонов в эпизоде «Время умереть» не более чем за семь минут." },
		{ ACH_HUNTER_SEASON, "Hunter Season", "Kill 45 hunters in one scene.",
			L"Сезон охоты", L"Убейте 45 охотников в одном эпизоде." },
		{ ACH_OFF_WITH_THEIR_HEADS, "Off With Their Heads!", "Execute 20 hunters in one scene.",
			L"Головы с плеч!", L"Казните 20 охотников в одном эпизоде." },
		{ ACH_HARD_AS_NAILS, "Hard as Nails", "Earn at least three stars on scenes 1 through 5.",
			L"Прочный как гвозди", L"Получите не менее трёх звёзд в эпизодах с 1-го по 5-й." },
		{ ACH_BRAWL_GAME, "Brawl Game", "Earn at least three stars on scenes 6 through 10.",
			L"Скандальная игра", L"Получите не менее трёх звёзд в эпизодах с 6-го по 10-й." },
		{ ACH_MONKEY_SEE_MONKEY_DIE, "Monkey See, Monkey Die!", "Earn at least three stars on scenes 11 through 15.",
			L"Обезьянка видит, обезьянка умирает!", L"Получите не менее трёх звёзд в эпизодах с 11-го по 15-й." },
		{ ACH_TIME_2_DIE, "Time 2 Die", "Earn at least three stars on scenes 16 through 20.",
			L"Время умереть", L"Получите не менее трёх звёзд в эпизодах с 16-го по 20-й." },
		{ ACH_MURDEROUS, "Murderous", "Complete every scene on Hardcore difficulty.",
			L"Кровожадный", L"Пройдите все эпизоды на сложности «Хардкор»." },
		{ ACH_DANGEROUS, "Dangerous", "Complete every scene on Fetish difficulty.",
			L"Опасный", L"Пройдите все эпизоды на сложности «Фетиш»." },
		{ ACH_4_STAR_FREAK, "4-Star Freak", "Earn a 4-star rating for every scene on Fetish Difficulty.",
			L"Четырёхзвёздочный урод", L"Получите четыре звезды за каждый эпизод на сложности «Фетиш»." },
		{ ACH_5_STAR_FIEND, "5-Star Fiend", "Earn a 5-star rating for every scene on Hardcore Difficulty.",
			L"Пятизвёздочный изверг", L"Получите пять звёзд за каждый эпизод на сложности «Хардкор»." },
		{ ACH_GETTING_YOUR_HANDS_DIRTY, "Getting Your Hands Dirty", "Kill 10 hunters in one scene using only your fists.",
			L"Запачкать руки", L"Убейте 10 охотников в одном эпизоде, используя только кулаки." },
		{ ACH_SWINGING_FOR_THE_FENCES, "Swinging for the Fences", "Kill 10 hunters in one scene using a baseball bat.",
			L"Бей со всей силы", L"Убейте 10 охотников в одном эпизоде бейсбольной битой." },
		{ ACH_THE_GRIM_REAPER, "The Grim Reaper", "Kill 10 hunters in one scene using a sickle.",
			L"Мрачный жнец", L"Убейте 10 охотников в одном эпизоде серпом." },
		{ ACH_SUBTLE_SLAUGHTER, "Subtle Slaughter", "Perform an execution with the chainsaw.",
			L"Тонкая резня", L"Совершите казнь бензопилой." },
		{ ACH_A_SPECIAL_GIFT, "A Special Gift", "Bring the patrolling hunter's head to the Guard Room during \"Mouth of Madness\".",
			L"Особый подарок", L"Принесите голову патрульного охотника в комнату охраны в эпизоде «Пасть безумия»." }
	};

	void PlayPressedAchievementSound(int index)
	{
		if (index < 0 || index >= ACHIEVEMENT_ICON_COUNT)
			return;
		// This is confirmation feedback, not cursor-movement feedback.
		// Sample 1 is SFX_FRONTEND_SPECIAL_MOVE (FEGEN_L/R); sample 2 is
		// the normal frontend click/select sound (FESEL_L/R).
		DMAudio.PlayFrontEndSound(
			eAchievements::IsUnlocked(ACHIEVEMENT_GALLERY[index].id) ? 2 : 1,
			-1.0f);
	}

	enum class AchievementWideTextEncoding
	{
		English,
		UnicodeRussian,
		WidenedCp1251Russian
	};

	AchievementWideTextEncoding GetAchievementWideTextEncoding()
	{
		if (AchievementSettings::iAchievementLanguage == 1)
			return AchievementWideTextEncoding::English;

		// The 1C localization replaces the English text bank. Depending on the
		// version, Cyrillic is returned either as Unicode or as CP1251 bytes
		// widened to wchar_t, so checking only U+0400-U+04FF is not sufficient.
		const char* localizationKeys[] =
		{
			"PLAY", "LOADG", "SETT", "QUITPRG", "MAINM", "IBACK"
		};

		int unicodeCyrillicLetters = 0;
		int widenedCp1251Letters = 0;
		for (const char* key : localizationKeys)
		{
			const wchar_t* text = CText::GetFromKey16(key);
			if (!text)
				continue;

			for (; *text; ++text)
			{
				const wchar_t character = *text;
				if (character >= L'\u0400' && character <= L'\u04FF')
					++unicodeCyrillicLetters;

				// А-я and Ё/ё when the original CP1251 byte was simply widened.
				if ((character >= L'\u00C0' && character <= L'\u00FF' &&
					character != L'\u00D7' && character != L'\u00F7') ||
					character == L'\u00A8' || character == L'\u00B8')
				{
					++widenedCp1251Letters;
				}
			}
		}

		if (unicodeCyrillicLetters > 0)
			return AchievementWideTextEncoding::UnicodeRussian;

		// A few extended Latin characters can occur in other localizations;
		// Russian menu strings contain many CP1251 letters.
		if (widenedCp1251Letters >= 3 ||
			AchievementSettings::iAchievementLanguage == 2)
		{
			return AchievementWideTextEncoding::WidenedCp1251Russian;
		}

		return AchievementWideTextEncoding::English;
	}

	bool IsRussianGameLocalization()
	{
		return GetAchievementWideTextEncoding() !=
			AchievementWideTextEncoding::English;
	}

	// The 1C font is not plain CP1251. Verified against the shipped Russian
	// levels\GLOBAL\PC_TEXT\pc_game.gxt and initscripts\FONTS\font.dat:
	//   * lowercase 'ь' is stored as 0xBE in every game string (81 times) and
	//     0xFC is never used, because the game reserves that code for a button
	//     icon — a literal 0xFC shows up in the menu as an arrow;
	//   * 'ё'/'Ё', the guillemets and the en/em dashes have no glyph at all in
	//     font.dat and are drawn as blank space.
	// Source strings therefore stay in normal Russian typography and are folded
	// onto the subset the font can actually draw right here.
	unsigned char RemapToRussianGameFont(unsigned char character)
	{
		switch (character)
		{
		case 0xFC: return 0xBE; // ь
		case 0xB8: return 0xE5; // ё -> е
		case 0xA8: return 0xC5; // Ё -> Е
		case 0x96:              // – -> -
		case 0x97: return 0x2D; // — -> -
		case 0xAB:              // « -> "
		case 0xBB: return 0x22; // » -> "
		default: return character;
		}
	}

	std::string ToRussianGameText(const wchar_t* text)
	{
		if (!text || !text[0])
			return {};

		const int size = WideCharToMultiByte(1251, 0, text, -1,
			nullptr, 0, nullptr, nullptr);
		if (size <= 1)
			return {};

		std::string result(static_cast<size_t>(size), '\0');
		WideCharToMultiByte(1251, 0, text, -1, result.data(), size,
			nullptr, nullptr);
		result.pop_back();
		for (char& character : result)
		{
			character = static_cast<char>(RemapToRussianGameFont(
				static_cast<unsigned char>(character)));
		}
		return result;
	}

	std::wstring ToRussianGameWideText(const wchar_t* text)
	{
		if (!text || !text[0])
			return {};

		// The 1C font maps Cyrillic glyphs to widened CP1251 byte values
		// (for example, U+0414 is stored as U+00C4). Even when CText returns
		// decoded Unicode on a particular executable build, custom strings must
		// still use the font's original widened-CP1251 character map.
		const std::string cp1251Text = ToRussianGameText(text);
		std::wstring result;
		result.reserve(cp1251Text.size());
		for (const unsigned char character : cp1251Text)
			result.push_back(static_cast<wchar_t>(character));
		return result;
	}

	const AchievementGalleryEntry* FindAchievementGalleryEntry(eAchievementID id)
	{
		for (const AchievementGalleryEntry& achievement : ACHIEVEMENT_GALLERY)
		{
			if (achievement.id == id)
				return &achievement;
		}
		return nullptr;
	}


	RwTexDictionary* g_achievementTextureDictionary = nullptr;
	int g_achievementDetailedTextures[ACHIEVEMENT_ICON_COUNT] = {};
	int g_achievementSmallTextures[ACHIEVEMENT_ICON_COUNT] = {};
	int g_achievementLockedDetailedTexture = 0;
	int g_achievementLockedSmallTexture = 0;
	int g_achievementLockedFrameTexture = 0;
	bool g_achievementTexturesLoadAttempted = false;
	bool g_achievementMouseInitialized = false;
	ControllerUi::PageInput g_controllerPages;
	// Selection sits on the page tabs under the grid rather than on a card, the
	// way the game's own bonus features screen moves focus down onto them.
	bool g_tabsFocused = false;
	bool g_achievementWheelUpLatched = false;
	bool g_achievementWheelDownLatched = false;
	float g_lastAchievementMouseX = 0.0f;
	float g_lastAchievementMouseY = 0.0f;

	void AddUniqueAchievementTexturePath(std::vector<std::filesystem::path>& paths,
		const std::filesystem::path& path)
	{
		if (path.empty())
			return;

		for (const std::filesystem::path& existingPath : paths)
		{
			if (existingPath == path)
				return;
		}
		paths.push_back(path);
	}

	std::filesystem::path GetModuleDirectory()
	{
		HMODULE module = nullptr;
		char modulePath[MAX_PATH] = {};
		if (GetModuleHandleExA(
			GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			reinterpret_cast<LPCSTR>(&GetModuleDirectory), &module) &&
			GetModuleFileNameA(module, modulePath, MAX_PATH))
		{
			return std::filesystem::path(modulePath).parent_path();
		}
		return {};
	}

	std::filesystem::path GetExecutableDirectory()
	{
		char executablePath[MAX_PATH] = {};
		if (GetModuleFileNameA(nullptr, executablePath, MAX_PATH))
			return std::filesystem::path(executablePath).parent_path();
		return {};
	}

	std::vector<std::filesystem::path> GetAchievementTexturePaths()
	{
		std::vector<std::filesystem::path> paths;
		const std::filesystem::path moduleDirectory = GetModuleDirectory();
		const std::filesystem::path executableDirectory = GetExecutableDirectory();

		// Preferred shared texture directory, including ASIs installed in scripts.
		AddUniqueAchievementTexturePath(paths,
			moduleDirectory / "data" / "txd" / "achievements.txd");
		AddUniqueAchievementTexturePath(paths,
			executableDirectory / "data" / "txd" / "achievements.txd");
		AddUniqueAchievementTexturePath(paths,
			executableDirectory / "scripts" / "data" / "txd" / "achievements.txd");
		if (moduleDirectory.empty() && executableDirectory.empty())
			AddUniqueAchievementTexturePath(paths, std::filesystem::path("data") / "txd" / "achievements.txd");

		// Support ASI loaders that load plugins either from the game root or scripts.
		AddUniqueAchievementTexturePath(paths,
			moduleDirectory / "data" / "ManhuntAchievements" / "achievements.txd");
		AddUniqueAchievementTexturePath(paths,
			moduleDirectory / "achievements.txd");
		AddUniqueAchievementTexturePath(paths,
			executableDirectory / "data" / "ManhuntAchievements" / "achievements.txd");
		AddUniqueAchievementTexturePath(paths,
			executableDirectory / "scripts" / "data" / "ManhuntAchievements" / "achievements.txd");
		AddUniqueAchievementTexturePath(paths,
			executableDirectory / "achievements.txd");
		AddUniqueAchievementTexturePath(paths,
			executableDirectory / "scripts" / "achievements.txd");

		// Last-resort relative paths for unusual loaders where module lookup fails.
		AddUniqueAchievementTexturePath(paths,
			std::filesystem::path("data") / "ManhuntAchievements" / "achievements.txd");
		AddUniqueAchievementTexturePath(paths,
			std::filesystem::path("scripts") / "data" / "ManhuntAchievements" / "achievements.txd");
		AddUniqueAchievementTexturePath(paths, "achievements.txd");

		return paths;
	}

	void LoadAchievementTextures()
	{
		if (g_achievementTexturesLoadAttempted)
			return;

		g_achievementTexturesLoadAttempted = true;
		std::filesystem::path loadedTexturePath;
		const std::vector<std::filesystem::path> texturePaths =
			GetAchievementTexturePaths();
		for (const std::filesystem::path& texturePath : texturePaths)
		{
			std::error_code error;
			if (!std::filesystem::is_regular_file(texturePath, error))
				continue;

			g_achievementTextureDictionary = TexDictLoad(texturePath.string().c_str());
			if (g_achievementTextureDictionary)
			{
				loadedTexturePath = texturePath;
				break;
			}
			eLog::Message(__FUNCTION__, "Could not load achievement icons from %s",
				texturePath.string().c_str());
		}

		if (!g_achievementTextureDictionary)
		{
			eLog::Message(__FUNCTION__,
				"Could not find a usable achievements.txd in the game root or scripts folder");
			return;
		}

		for (int index = 0; index < ACHIEVEMENT_ICON_COUNT; ++index)
		{
			char detailedName[32] = {};
			char smallName[32] = {};
			sprintf(detailedName, "trop%03d_l", index);
			sprintf(smallName, "trop%03d_s", index);
			g_achievementDetailedTextures[index] = CFrontend::GetTextureFromTXD(
				reinterpret_cast<int>(g_achievementTextureDictionary), detailedName);
			g_achievementSmallTextures[index] = CFrontend::GetTextureFromTXD(
				reinterpret_cast<int>(g_achievementTextureDictionary), smallName);
		}
		g_achievementLockedDetailedTexture = CFrontend::GetTextureFromTXD(
			reinterpret_cast<int>(g_achievementTextureDictionary), "trophy_locked_l");
		g_achievementLockedSmallTexture = CFrontend::GetTextureFromTXD(
			reinterpret_cast<int>(g_achievementTextureDictionary), "trophy_locked_s");
		g_achievementLockedFrameTexture = CFrontend::GetTextureFromTXD(
			reinterpret_cast<int>(g_achievementTextureDictionary), "frame_locked_lcd");

		eLog::Message(__FUNCTION__, "Loaded achievement icon dictionary from %s",
			loadedTexturePath.string().c_str());
	}

	void PrintCenteredText(const char* text, float centerX, float y, float scale,
		int red, int green, int blue, int alpha)
	{
		const float width = CFrontend::GetTextWidth8(const_cast<char*>(text), scale, FONT_TYPE_DEFAULT);
		const float x = SCREEN_FROM_CENTER(centerX) - width * 0.5f;
		CFrontend::SetDrawRGBA(0, 0, 0, alpha);
		CFrontend::Print8(text, x + SCREEN_SCLX(0.002f), y + 0.003f,
			scale, scale, 0.0f, FONT_TYPE_DEFAULT);
		CFrontend::SetDrawRGBA(red, green, blue, alpha);
		CFrontend::Print8(text, x, y, scale, scale, 0.0f, FONT_TYPE_DEFAULT);
	}

	void PrintCenteredTextFitted(const char* text, float centerX, float y,
		float maximumWidth, float maximumScale, float minimumScale,
		int red, int green, int blue, int alpha)
	{
		float scale = maximumScale;
		const float screenMaximumWidth = SCREEN_SCLX(maximumWidth);
		while (scale > minimumScale &&
			CFrontend::GetTextWidth8(const_cast<char*>(text), scale, FONT_TYPE_DEFAULT) >
			screenMaximumWidth)
		{
			scale -= 0.02f;
		}

		if (scale < minimumScale)
			scale = minimumScale;

		PrintCenteredText(text, centerX, y, scale, red, green, blue, alpha);
	}

	void PrintCenteredWideText(const wchar_t* text, float centerX, float y, float scale,
		int red, int green, int blue, int alpha)
	{
		wchar_t* mutableText = const_cast<wchar_t*>(text);
		const float width = CFrontend::CalculateTextLen(mutableText, scale,
			FONT_TYPE_DEFAULT);
		const float x = SCREEN_FROM_CENTER(centerX) - width * 0.5f;
		CFrontend::SetDrawRGBA(0, 0, 0, alpha);
		CFrontend::Print16(text, x + SCREEN_SCLX(0.002f), y + 0.003f,
			scale, scale, 0.0f, FONT_TYPE_DEFAULT);
		CFrontend::SetDrawRGBA(red, green, blue, alpha);
		CFrontend::Print16(text, x, y, scale, scale, 0.0f, FONT_TYPE_DEFAULT);
	}

	void PrintCenteredWideTextFitted(const wchar_t* text, float centerX, float y,
		float maximumWidth, float maximumScale, float minimumScale,
		int red, int green, int blue, int alpha)
	{
		float scale = maximumScale;
		const float screenMaximumWidth = SCREEN_SCLX(maximumWidth);
		wchar_t* mutableText = const_cast<wchar_t*>(text);
		while (scale > minimumScale &&
			CFrontend::CalculateTextLen(mutableText, scale, FONT_TYPE_DEFAULT) >
			screenMaximumWidth)
		{
			scale -= 0.02f;
		}

		if (scale < minimumScale)
			scale = minimumScale;

		PrintCenteredWideText(text, centerX, y, scale, red, green, blue, alpha);
	}

	bool IsAchievementConcealed(const AchievementGalleryEntry& achievement)
	{
		const AchievementDefinition* definition = eAchievements::GetDefinition(achievement.id);
		return definition && definition->hidden && !eAchievements::IsUnlocked(achievement.id);
	}
}

int AchievementMenu::m_nCurrentAchievementPos = 0;
int AchievementMenu::m_nCurrentAchievementPage = 0;
int AchievementMenu::m_allAchievementPages = 6;
int AchievementMenu::m_achievementReturnMenu = MENU_FRONTEND;
wchar_t AchievementMenu::m_szStatsBuffer[128] = {};

int AchievementMenu::HookPauseMenuProcess()
{
	const int result = CallAndReturn<int, 0x5FFB50>();
	int& pauseSelection = *reinterpret_cast<int*>(0x7C8A68);

	if (CFrontend::ms_currentMenu == MENU_PAUSE && pauseSelection == 4 &&
		CInputManager::FrontendButtonEnter())
	{
		m_nCurrentAchievementPos = 0;
		m_nCurrentAchievementPage = 0;
		m_achievementReturnMenu = MENU_PAUSE;
		ResetGalleryBack();
		achievementPressHeld=RawKeyDown(1045);
		eLog::Message(__FUNCTION__,"Open achievements from pause");
		g_controllerPages.Reset(ControllerUi::Read());
		g_tabsFocused = false;
		g_achievementMouseInitialized = false;
		g_achievementWheelUpLatched = false;
		g_achievementWheelDownLatched = false;
		DMAudio.PlayFrontEndSound(0, -1.0f);
		CFrontend::SetCurrentMenu(MENU_ACHIEVEMENTS);
	}

	return result;
}

void AchievementMenu::HookPauseMenuDraw()
{
	Call<0x5FFD20>();

	// The original menu hides all rows while its transition/confirmation state is active.
	if (*reinterpret_cast<int*>(0x7C8758) != 0)
		return;

	const float scaleX = CFrontend::ms_fTextXScale;
	const float scaleY = CFrontend::ms_fTextYScale;
	const float rowSpacing = 0.048f;
	// Selection 4 wraps before original selection 0, so drawing it one row above
	// the original menu keeps both keyboard and mouse order natural.
	const float y = 0.68f - rowSpacing;
	const std::wstring achievementLabel = IsRussianGameLocalization() ?
		ToRussianGameWideText(L"ДОСТИЖЕНИЯ") : L"ACHIEVEMENTS";
	wchar_t* label = const_cast<wchar_t*>(achievementLabel.c_str());
	const float width = CFrontend::CalculateTextLen(label, scaleX, FONT_FRONTEND);
	const float x = 0.5f - width * 0.5f;
	const int pauseSelection = *reinterpret_cast<int*>(0x7C8A68);

	CFrontend::AddOption(label, x, y, scaleX, scaleY,
		pauseSelection == 4);
}

void AchievementMenu::AchievementsMenu()
{
    static bool first = true;
    if (first) {
        first = false;
        eLog::Message(__FUNCTION__, "First achievement gallery render; resolving textures and optional controller UI");
    }
	LoadAchievementTextures();
	const bool useRussian = IsRussianGameLocalization();
	const auto controller = ControllerUi::Read();
	const bool controllerActive = ControllerUi::Active(controller);

	m_nCurrentAchievementPage = m_nCurrentAchievementPos / ACHIEVEMENTS_PER_PAGE;
	const std::wstring counterFormat = useRussian ?
		ToRussianGameWideText(L"ДОСТИЖЕНИЯ %d/%d") : L"ACHIEVEMENTS %d/%d";
	wsprintfW(m_szStatsBuffer, counterFormat.c_str(),
		m_nCurrentAchievementPage + 1, m_allAchievementPages);
	CFrontend::DrawMenuCameraCounter(m_szStatsBuffer);

	const float panelX = 0.12f;
	const float panelY = 0.17f;
	const float panelWidth = 0.76f;
	const float panelHeight = 0.63f;
	const float firstIconX = 0.175f;
	const float firstIconY = 0.195f;
	const float columnSpacing = 0.25f;
	const float rowSpacing = 0.245f;
	const float iconWidth = 0.15f;
	const float iconHeight = 0.20f;

	CRenderer::DrawQuad2d(SCREEN_FROM_CENTER(panelX), panelY,
		SCREEN_SCLX(panelWidth), panelHeight, 12, 12, 12, 65, 0);

	FEMouse mouse = CInputManager::GetFrontendMouse();
	bool mouseMoved = false;
	if (!g_achievementMouseInitialized)
	{
		g_achievementMouseInitialized = true;
		g_lastAchievementMouseX = mouse.X;
		g_lastAchievementMouseY = mouse.Y;
	}
	else if (mouse.X != g_lastAchievementMouseX || mouse.Y != g_lastAchievementMouseY)
	{
		mouseMoved = true;
		g_lastAchievementMouseX = mouse.X;
		g_lastAchievementMouseY = mouse.Y;
	}

	const int firstAchievement = m_nCurrentAchievementPage * ACHIEVEMENTS_PER_PAGE;
	const int lastAchievement = min(firstAchievement + ACHIEVEMENTS_PER_PAGE,
		ACHIEVEMENT_ICON_COUNT);

	for (int index = firstAchievement; index < lastAchievement; ++index)
	{
		const int slot = index - firstAchievement;
		const int column = slot % 3;
		const int row = slot / 3;
		const float x = firstIconX + column * columnSpacing;
		const float y = firstIconY + row * rowSpacing;
		const float screenIconX = SCREEN_FROM_CENTER(x);
		const float screenWidth = SCREEN_SCLX(iconWidth);

		const bool hoveredAchievement=!controllerActive &&
			mouse.X >= screenIconX && mouse.X < screenIconX + screenWidth &&
			mouse.Y >= y && mouse.Y < y + iconHeight + 0.040f;
		if (hoveredAchievement && mouseMoved)
		{
			g_tabsFocused = false;
			if (m_nCurrentAchievementPos != index)
			{
				m_nCurrentAchievementPos = index;
				DMAudio.PlayFrontEndSound(0, -1.0f);
			}
		}
		if (hoveredAchievement && CPad::NewMouseControllerState.lmb &&
			!CPad::OldMouseControllerState.lmb)
			PlayPressedAchievementSound(index);

		const bool selected = index == m_nCurrentAchievementPos && !g_tabsFocused;
		if (selected)
		{
			CRenderer::DrawQuad2d(SCREEN_FROM_CENTER(x - 0.007f), y - 0.007f,
				SCREEN_SCLX(iconWidth + 0.014f), iconHeight + 0.014f,
				210, 210, 235, 220, 0);
		}
		else
		{
			CRenderer::DrawQuad2d(SCREEN_FROM_CENTER(x - 0.004f), y - 0.004f,
				SCREEN_SCLX(iconWidth + 0.008f), iconHeight + 0.008f,
				55, 55, 55, 180, 0);
		}

		const AchievementGalleryEntry& achievement = ACHIEVEMENT_GALLERY[index];
		const bool unlocked = eAchievements::IsUnlocked(achievement.id);
		const bool concealed = IsAchievementConcealed(achievement);
		const int texture = concealed && g_achievementLockedDetailedTexture ?
			g_achievementLockedDetailedTexture : g_achievementDetailedTextures[index];
		if (texture)
		{
			const int tint = selected ? 255 : 220;
			CRenderer::DrawQuad2d(screenIconX, y, screenWidth, iconHeight,
				tint, tint, tint, 255, texture);
		}
		else
		{
			CRenderer::DrawQuad2d(screenIconX, y, screenWidth, iconHeight,
				70, 70, 70, 255, 0);
		}

		// Slightly dim visible but locked achievements while keeping the artwork readable.
		// The lock frame is drawn afterwards so it stays crisp above the darkening layer.
		if (!unlocked && !concealed)
		{
			CRenderer::DrawQuad2d(screenIconX, y, screenWidth, iconHeight,
				0, 0, 0, selected ? 55 : 75, 0);
		}

		// The lock texture contains only the noisy frame/glyph in its alpha channel.
		// Draw it over the base achievement image so the icon remains visible beneath it.
		if (!unlocked && !concealed && g_achievementLockedFrameTexture)
		{
			CRenderer::DrawQuad2d(screenIconX, y, screenWidth, iconHeight,
				255, 255, 255, 255, g_achievementLockedFrameTexture);
		}

		char label[32] = {};
		sprintf(label, "#%02d", index + 1);
		PrintCenteredText(label, x + iconWidth * 0.5f, y + iconHeight + 0.010f,
			0.48f, selected ? 255 : 160, selected ? 255 : 160,
			selected ? 255 : 160, 255);
	}

	const AchievementGalleryEntry& selectedAchievement =
		ACHIEVEMENT_GALLERY[m_nCurrentAchievementPos];
	const bool selectedAchievementConcealed = IsAchievementConcealed(selectedAchievement);
	const std::wstring selectedName = selectedAchievementConcealed ? L"???" :
		(useRussian ? ToRussianGameWideText(selectedAchievement.russianName) :
			std::wstring(selectedAchievement.name,
				selectedAchievement.name + strlen(selectedAchievement.name)));
	const std::wstring selectedDescription = selectedAchievementConcealed ? L"???" :
		(useRussian ? ToRussianGameWideText(selectedAchievement.russianDescription) :
			std::wstring(selectedAchievement.description,
				selectedAchievement.description + strlen(selectedAchievement.description)));
	PrintCenteredWideTextFitted(selectedName.c_str(), 0.5f, 0.690f,
		0.76f, 0.64f, 0.42f, 255, 255, 255, 255);
	PrintCenteredWideTextFitted(selectedDescription.c_str(), 0.5f, 0.735f,
		0.80f, 0.48f, 0.30f, 175, 175, 175, 255);

	const float firstTabCenterX = 0.2375f;
	const float tabSpacing = 0.105f;
	const float tabY = 0.785f;
	// Sized to match the page tabs on the game's own bonus features screen.
	const float tabScale = 0.60f;
	for (int page = 0; page < m_allAchievementPages; ++page)
	{
		const int rangeStart = page * ACHIEVEMENTS_PER_PAGE + 1;
		const int rangeEnd = min(rangeStart + ACHIEVEMENTS_PER_PAGE - 1,
			ACHIEVEMENT_ICON_COUNT);
		char tabLabel[16] = {};
		sprintf(tabLabel, "%d-%d", rangeStart, rangeEnd);

		const float centerX = firstTabCenterX + page * tabSpacing;
		const float textWidth = CFrontend::GetTextWidth8(tabLabel, tabScale,
			FONT_TYPE_DEFAULT);
		const float textX = SCREEN_FROM_CENTER(centerX) - textWidth * 0.5f;
		const float paddingX = SCREEN_SCLX(0.008f);
		const float tabHeight = CFrontend::GetFontHeight(FONT_TYPE_DEFAULT, tabScale);
		const bool selectedPage = page == m_nCurrentAchievementPage;
		const bool hovered = !controllerActive && mouse.X >= textX - paddingX &&
			mouse.X < textX + textWidth + paddingX &&
			mouse.Y >= tabY - 0.004f && mouse.Y < tabY + tabHeight + 0.004f;

		const bool focusedTab = selectedPage && g_tabsFocused;
		if (selectedPage || hovered)
		{
			const int shade = focusedTab ? 250 : (selectedPage ? 205 : 95);
			CRenderer::DrawQuad2d(textX - paddingX, tabY - 0.004f,
				textWidth + paddingX * 2.0f, tabHeight + 0.008f,
				shade, shade, shade, selectedPage ? 225 : 180, 0);
		}

		CFrontend::SetDrawRGBA(selectedPage ? 20 : 185,
			selectedPage ? 20 : 185, selectedPage ? 20 : 185, 255);
		CFrontend::Print8(tabLabel, textX, tabY, tabScale, tabScale,
			0.0f, FONT_TYPE_DEFAULT);

		if (hovered && CPad::NewMouseControllerState.lmb &&
			!CPad::OldMouseControllerState.lmb)
		{
			m_nCurrentAchievementPage = page;
			m_nCurrentAchievementPos = page * ACHIEVEMENTS_PER_PAGE;
			DMAudio.PlayFrontEndSound(0, -1.0f);
		}
	}

	std::wstring navigationHint = useRussian ?
		ToRussianGameWideText(L"Стрелки / колесо: выбор") :
		L"Arrows / wheel: select";
	std::wstring pageHint = useRussian ?
		ToRussianGameWideText(L"Вкладки: страница") : L"Tabs: page";
	std::wstring backHint;
	if (controllerActive)
	{
		const bool ps = controller.glyphSet == ManhuntGInputUI::PlayStation;
		navigationHint = ControllerUi::Button(controller, controller.horizontal, L"D-pad") + L" " +
			ControllerUi::Button(controller, controller.vertical, L"/ LS") + L": " +
			(useRussian ? ToRussianGameWideText(L"выбор") : L"select");
		pageHint = ControllerUi::Button(controller, controller.previousPage, ps ? L"L1" : L"LB") + L" / " +
			ControllerUi::Button(controller, controller.nextPage, ps ? L"R1" : L"RB") + L": " +
			(useRussian ? ToRussianGameWideText(L"страницы") : L"pages");
		backHint = ControllerUi::Button(controller, controller.back, ps ? L"CIRCLE" : L"B") + L": " +
			(useRussian ? ToRussianGameWideText(L"назад") : L"back");
	}
	CFrontend::PrintInfo(const_cast<wchar_t*>(navigationHint.c_str()),
		const_cast<wchar_t*>(pageHint.c_str()),
		controllerActive ? const_cast<wchar_t*>(backHint.c_str()) : CText::GetFromKey16("IBACK"), L"");
}

std::wstring AchievementMenu::GetLocalizedAchievementName(eAchievementID id)
{
	const AchievementGalleryEntry* achievement = FindAchievementGalleryEntry(id);
	if (!achievement)
		return {};
	if (IsRussianGameLocalization())
		return ToRussianGameWideText(achievement->russianName);
	return std::wstring(achievement->name,
		achievement->name + strlen(achievement->name));
}

std::wstring AchievementMenu::GetLocalizedAchievementDescription(eAchievementID id)
{
	const AchievementGalleryEntry* achievement = FindAchievementGalleryEntry(id);
	if (!achievement)
		return {};
	if (IsRussianGameLocalization())
		return ToRussianGameWideText(achievement->russianDescription);
	return std::wstring(achievement->description,
		achievement->description + strlen(achievement->description));
}

std::wstring AchievementMenu::GetLocalizedAchievementUnlockedText()
{
	return IsRussianGameLocalization() ?
		ToRussianGameWideText(L"ДОСТИЖЕНИЕ ПОЛУЧЕНО") : L"ACHIEVEMENT UNLOCKED";
}

int AchievementMenu::GetAchievementSmallTexture(eAchievementID id)
{
	LoadAchievementTextures();
	for (int index = 0; index < ACHIEVEMENT_ICON_COUNT; ++index)
	{
		if (ACHIEVEMENT_GALLERY[index].id == id)
		{
			return g_achievementSmallTextures[index] ?
				g_achievementSmallTextures[index] : g_achievementDetailedTextures[index];
		}
	}
	return 0;
}

void AchievementMenu::ProcessAchievementsMenu()
{
	const auto controller = ControllerUi::Read();
	const bool rawBack=RawBackDown();
	const bool infoBack=CFrontend::GetInfoBarInput3()!=0;
	if (galleryBack.Consume(rawBack,infoBack,CPad::NewMouseControllerState.lmb!=0))
	{
		waitForMainBackRelease=m_achievementReturnMenu==MENU_FRONTEND;
		// Consume the Back hit produced by our own info bar. Main has no PrintInfo
		// call to clear it, and the native Escape helper plays sound for a stale hit
		// even when it returns false. Do not touch the shared keyboard/GInput queue.
		*reinterpret_cast<int*>(0x7C8F9C)=0;
		DMAudio.PlayFrontEndSound(3, -1.0f);
		CFrontend::SetCurrentMenu(m_achievementReturnMenu);
		eLog::Message(__FUNCTION__,"Close achievements: return menu=%d, current=%d, rawBack=%d, infoBack=%d",
			m_achievementReturnMenu,CFrontend::ms_currentMenu,rawBack,infoBack);
		return;
	}
	const bool pressDown=RawKeyDown(1045); // Enter, or GInput's A/Cross event.
	if (pressDown && !achievementPressHeld)
		PlayPressedAchievementSound(m_nCurrentAchievementPos);
	achievementPressHeld=pressDown;

	auto moveHorizontal = [](int& position, int direction) -> bool
	{
		const int page = position / ACHIEVEMENTS_PER_PAGE;
		const int pageStart = page * ACHIEVEMENTS_PER_PAGE;
		const int pageEnd = min(pageStart + ACHIEVEMENTS_PER_PAGE,
			ACHIEVEMENT_ICON_COUNT);
		const int slot = position - pageStart;
		const int column = slot % 3;
		const int row = slot / 3;

		if (direction < 0)
		{
			if (column > 0)
			{
				--position;
				return true;
			}
			if (page > 0)
			{
				const int previousStart = pageStart - ACHIEVEMENTS_PER_PAGE;
				const int previousEnd = pageStart;
				position = min(previousStart + row * 3 + 2, previousEnd - 1);
				return true;
			}
		}
		else
		{
			if (column < 2 && position + 1 < pageEnd)
			{
				++position;
				return true;
			}
			if (page + 1 < (ACHIEVEMENT_ICON_COUNT + ACHIEVEMENTS_PER_PAGE - 1) /
				ACHIEVEMENTS_PER_PAGE)
			{
				const int nextStart = pageEnd;
				position = min(nextStart + row * 3, ACHIEVEMENT_ICON_COUNT - 1);
				return true;
			}
		}
		return false;
	};

	auto moveVertical = [](int& position, int direction) -> bool
	{
		const int page = position / ACHIEVEMENTS_PER_PAGE;
		const int pageStart = page * ACHIEVEMENTS_PER_PAGE;
		const int pageEnd = min(pageStart + ACHIEVEMENTS_PER_PAGE,
			ACHIEVEMENT_ICON_COUNT);
		const int slot = position - pageStart;
		const int column = slot % 3;
		const int row = slot / 3;

		if (direction < 0)
		{
			if (row > 0)
			{
				position -= 3;
				return true;
			}
			if (page > 0)
			{
				const int previousStart = pageStart - ACHIEVEMENTS_PER_PAGE;
				position = min(previousStart + 3 + column, pageStart - 1);
				return true;
			}
		}
		else
		{
			if (row == 0 && position + 3 < pageEnd)
			{
				position += 3;
				return true;
			}
			if (pageEnd < ACHIEVEMENT_ICON_COUNT)
			{
				position = min(pageEnd + column, ACHIEVEMENT_ICON_COUNT - 1);
				return true;
			}
		}
		return false;
	};

	const int page = m_nCurrentAchievementPos / ACHIEVEMENTS_PER_PAGE;
	const int pageStart = page * ACHIEVEMENTS_PER_PAGE;
	const int pageEnd = min(pageStart + ACHIEVEMENTS_PER_PAGE, ACHIEVEMENT_ICON_COUNT);
	const int column = (m_nCurrentAchievementPos - pageStart) % 3;
	const int gridRow = (m_nCurrentAchievementPos - pageStart) / 3;

	if (g_tabsFocused)
	{
		// On the tabs the sideways keys turn pages instead of moving between
		// cards, and either vertical key hands focus back to the grid.
		int wantedPage = page;
		if (CInputManager::FrontendPressedLeft() && page > 0)
			wantedPage = page - 1;
		if (CInputManager::FrontendPressedRight() && page + 1 < m_allAchievementPages)
			wantedPage = page + 1;
		if (wantedPage != page)
			m_nCurrentAchievementPos =
				min(wantedPage * ACHIEVEMENTS_PER_PAGE, ACHIEVEMENT_ICON_COUNT - 1);
		if (CInputManager::FrontendPressedUp())
		{
			g_tabsFocused = false;
			m_nCurrentAchievementPos = min(pageStart + 3 + column, pageEnd - 1);
		}
		else if (CInputManager::FrontendPressedDown())
		{
			g_tabsFocused = false;
			m_nCurrentAchievementPos = min(pageStart + column, pageEnd - 1);
		}
	}
	else
	{
		if (CInputManager::FrontendPressedLeft())
			moveHorizontal(m_nCurrentAchievementPos, -1);
		if (CInputManager::FrontendPressedRight())
			moveHorizontal(m_nCurrentAchievementPos, 1);
		// Leaving the grid vertically lands on the tabs; pages are turned there.
		if (CInputManager::FrontendPressedUp())
		{
			if (gridRow > 0)
				m_nCurrentAchievementPos -= 3;
			else
				g_tabsFocused = true;
		}
		if (CInputManager::FrontendPressedDown())
		{
			if (gridRow == 0 && m_nCurrentAchievementPos + 3 < pageEnd)
				m_nCurrentAchievementPos += 3;
			else
				g_tabsFocused = true;
		}
	}
	const int pageDirection = g_controllerPages.Consume(controller);
	if (pageDirection)
	{
		const int next = ControllerUi::PageSelection(m_nCurrentAchievementPos,
			pageDirection, ACHIEVEMENT_ICON_COUNT, ACHIEVEMENTS_PER_PAGE);
		if (next != m_nCurrentAchievementPos)
		{
			m_nCurrentAchievementPos = next;
			DMAudio.PlayFrontEndSound(0, -1.0f);
		}
	}

	const bool wheelUp = !ControllerUi::Active(controller) && CPad::NewMouseControllerState.wheelUp != 0;
	const bool wheelDown = !ControllerUi::Active(controller) && CPad::NewMouseControllerState.wheelDown != 0;
	const bool wheelUpPressed = wheelUp && !g_achievementWheelUpLatched;
	const bool wheelDownPressed = wheelDown && !g_achievementWheelDownLatched;
	g_achievementWheelUpLatched = wheelUp;
	g_achievementWheelDownLatched = wheelDown;

	bool movedWithMouseWheel = false;
	if (wheelUpPressed || wheelDownPressed)
		g_tabsFocused = false;
	if (wheelUpPressed)
		movedWithMouseWheel = moveVertical(m_nCurrentAchievementPos, -1);

	if (wheelDownPressed)
		movedWithMouseWheel = moveVertical(m_nCurrentAchievementPos, 1);

	// The game's arrow handlers already play the standard navigation sound.
	// Mouse-wheel navigation bypasses those handlers, so play it explicitly.
	if (movedWithMouseWheel)
		DMAudio.PlayFrontEndSound(0, -1.0f);

	m_nCurrentAchievementPage = m_nCurrentAchievementPos / ACHIEVEMENTS_PER_PAGE;
}

void AchievementMenu::Open(int returnMenu)
{
	ResetGalleryBack();
	achievementPressHeld=RawKeyDown(1045);
	eLog::Message(__FUNCTION__,"Open achievements: return menu=%d",returnMenu);
	g_controllerPages.Reset(ControllerUi::Read());
    m_nCurrentAchievementPos = 0;
    m_nCurrentAchievementPage = 0;
    m_achievementReturnMenu = returnMenu;
    g_tabsFocused = false;
    g_achievementMouseInitialized = false;
    g_achievementWheelUpLatched = false;
    g_achievementWheelDownLatched = false;
    DMAudio.PlayFrontEndSound(0, -1.0f);
    CFrontend::SetCurrentMenu(MENU_ACHIEVEMENTS);
}

namespace
{
    void __cdecl RecordMenuOption(MenuOptionArgs* args)
    {
        g_currentRowShift = 0.0f;
        if (!g_countingMenuOptions || !args)
            return;
        // The same draw hook can also render a foreign plugin's custom level
        // list. Only a list that starts with the game's own PLAY row is the
        // main menu, so only that one gets an achievements row appended.
        if (g_foreignRowCount == 0)
        {
            const wchar_t* play = CText::GetFromKey16("PLAY");
            g_appendAchievementRow = args->text && play && wcscmp(args->text, play) == 0;
            g_bonusRowIndex = -1;
            g_achievementRowY = 0.0f;
        }
        if (g_appendAchievementRow && g_reorderMainMenu)
        {
            // Lift the whole list half a step and push everything below the bonus
            // features row down by one. That is the layout the menu would have
            // had with our row in it all along, and it opens the gap for it.
            const float step = CFrontend::ms_fMenuPositionY;
            g_currentRowShift = g_bonusRowIndex >= 0 ? 0.5f * step : -0.5f * step;
            args->y += g_currentRowShift;
            const wchar_t* bonus = CText::GetFromKey16("BONFEA");
            if (g_bonusRowIndex < 0 && bonus && args->text &&
                wcscmp(args->text, bonus) == 0)
            {
                g_bonusRowIndex = g_foreignRowCount;
                g_achievementRowY = args->y + step;
            }
        }
        ++g_foreignRowCount;
        g_lastForeignRow = *args;
    }

    // The text half of the game's AddOption, which receives the same coordinates
    // and must be moved by the same amount or the label parts company with its box.
    void __cdecl ShiftMenuOptionText(MenuOptionArgs* args)
    {
        if (args && g_currentRowShift != 0.0f)
            args->y += g_currentRowShift;
    }
}

// Entry detour on the text half of the game's option drawing. Replays the two
// instructions and the call the jump overwrote, then rejoins the original.
void __declspec(naked) AchievementMenu::HookMenuOptionText()
{
    static const uintptr_t continuation = 0x5D5B38;
    static const uintptr_t replayedCall = 0x5EA2B0;
    __asm {
        pushad
        lea eax, [esp+0x24]
        push eax
        call ShiftMenuOptionText
        add esp, 4
        popad
        push ebx
        xor ebx, ebx
        call dword ptr [replayedCall]
        jmp continuation
    }
}

// Entry detour on the game's option registration, active only while a foreign
// main menu draw is running. The five byte jump covers the four pushes and the
// first byte of the stack reservation behind them, so the whole prologue is
// replayed here and the original function resumes past it.
void __declspec(naked) AchievementMenu::HookMenuOptionRegistered()
{
    // A function entry, so incoming flags carry nothing and are not preserved.
    __asm {
        pushad
        lea eax, [esp+0x24]
        push eax
        call RecordMenuOption
        add esp, 4
        popad
        cmp byte ptr [replayMenuOptionPrologue], 0
        je chainedOption
        push ebx
        push esi
        push edi
        push ebp
        sub esp, 0x0C
    chainedOption:
        jmp dword ptr [originalMenuOption]
    }
}

void AchievementMenu::InitHooks()
{
    originalMenuDispatch=PluginCompatibility::JumpTarget(0x5D75D7);
    originalMenuBackground=PluginCompatibility::JumpTarget(0x5D70F9);
    // Appending a row means observing option registration. If that site is in
    // a shape we cannot chain safely, skip the row rather than refuse to load:
    // everything else, the pause menu entry included, keeps working.
    if (PluginCompatibility::ForeignMainMenu() &&
        PluginCompatibility::DetourableSite(kMenuOptionSite))
    {
        // Another plugin owns the main menu list. Leave its rows, its layout and
        // its settings alone: run its draw, count what it registered, then append
        // one row of our own. Its input handler keeps every row it knows about.
        originalMainMenuDraw=PluginCompatibility::JumpTarget(0x600C20);
        originalMainMenuInput=PluginCompatibility::JumpTarget(0x600B34);
        // Never drop an existing detour on option registration: if another
        // plugin already owns it, continue into that plugin instead of the
        // game's prologue.
        const uintptr_t foreignOption=PluginCompatibility::JumpTarget(0x5D55C0);
        replayMenuOptionPrologue=foreignOption==0;
        originalMenuOption=foreignOption ? foreignOption : 0x5D55C7;
        InjectHook(0x5D55C0, HookMenuOptionRegistered, PATCH_JUMP);
        // Moving a row means moving both halves of the game's option drawing.
        // Without the text half our row can only be appended at the bottom.
        g_reorderMainMenu = memcmp(reinterpret_cast<const void*>(kMenuTextSite.address),
            kMenuTextSite.expected, kMenuTextSite.size) == 0;
        if (g_reorderMainMenu)
            InjectHook(0x5D5B30, HookMenuOptionText, PATCH_JUMP);
        else
            eLog::Message(__FUNCTION__,
                "Option text drawing is already hooked; our row goes to the bottom of the list");
        InjectHook(0x600C20, AppendAchievementRowToMainMenu, PATCH_JUMP);
        InjectHook(0x600B34, ProcessMainMenu, PATCH_JUMP);
        eLog::Message(__FUNCTION__,"Appending our row to a foreign main menu: draw=0x%08X, input=0x%08X",
            originalMainMenuDraw,originalMainMenuInput);
    }
    else if (PluginCompatibility::ForeignMainMenu())
    {
        eLog::Message(__FUNCTION__,
            "Option registration is hooked in a shape we cannot chain; leaving the foreign "
            "main menu untouched. The gallery stays reachable from the pause menu.");
    }
    else
    {
        InjectHook(0x600C20, MainMenu, PATCH_JUMP);
        InjectHook(0x600B34, ProcessMainMenu, PATCH_JUMP);
        eLog::Message(__FUNCTION__,"Drawing the vanilla main menu ourselves");
    }
    InjectHook(0x5D70F9, HookSelectMenuBackground, PATCH_JUMP);
    InjectHook(0x5D75D7, HookExecuteMenuProcess, PATCH_JUMP);
    Patch<uintptr_t>(0x7D61D0, reinterpret_cast<uintptr_t>(&HookPauseMenuProcess));
    Patch<uintptr_t>(0x7D61D4, reinterpret_cast<uintptr_t>(&HookPauseMenuDraw));
    Patch<unsigned char>(0x5FFB89, 4);
    Patch<unsigned char>(0x5FFBAC, 4);
}

void AchievementMenu::MainMenu()
{
    static bool first = true;
    if (first) {
        first = false;
        eLog::Message(__FUNCTION__, "First main menu render");
    }
    CFrontend::DrawMenuCameraCounter(CText::GetFromKey16("MAINM"));
    const int logo = CFrontend::GetTextureFromTXD(*reinterpret_cast<int*>(0x7C8704), "logo");
    CRenderer::DrawQuad2d(*reinterpret_cast<float*>(0x7D6408),
        *reinterpret_cast<float*>(0x7D6404),
        *reinterpret_cast<float*>(0x7D3458) * *reinterpret_cast<float*>(0x7D63FC),
        *reinterpret_cast<float*>(0x7D6400), 180, 180, 180, 255, logo);

    const float x = CFrontend::ms_fMenuPositionX;
    float y = 0.4f - CFrontend::ms_fMenuPositionY * 2.5f;
    for (int button = 0; button < kMainRowCount; ++button)
    {
        const auto& row=mainRows[button];
        std::wstring custom;
        if (!row.key) custom=row.russian && IsRussianGameLocalization() ? ToRussianGameWideText(row.russian) : row.english;
        wchar_t* label=row.key ? CText::GetFromKey16(row.key) : const_cast<wchar_t*>(custom.c_str());
        CFrontend::AddOption(label, x, y,
            CFrontend::ms_fTextXScale, CFrontend::ms_fTextYScale,
            CFrontend::ms_menuButton == button);
        y += CFrontend::ms_fMenuPositionY;
    }
}

// Runs in place of the foreign plugin's own main menu draw hook. It draws its
// rows exactly as before; we only observe how many it registered and add ours
// underneath, inheriting its x, y step and text scale.
void AchievementMenu::AppendAchievementRowToMainMenu()
{
    // Written once so a crash inside the foreign menu can be told apart from a
    // crash on the way in: the pair of lines only completes if it returned.
    static bool first = true;
    const bool trace = first;
    first = false;
    g_foreignRowCount = 0;
    g_appendAchievementRow = false;
    g_countingMenuOptions = true;
    if (trace)
        eLog::Message(__FUNCTION__, "First foreign main menu draw: entering 0x%08X",
            originalMainMenuDraw);
    if (originalMainMenuDraw)
        reinterpret_cast<void(__cdecl*)()>(originalMainMenuDraw)();
    if (trace)
        eLog::Message(__FUNCTION__, "First foreign main menu draw: returned with %d rows",
            g_foreignRowCount);
    g_countingMenuOptions = false;
    g_foreignRowsLastFrame = g_foreignRowCount;
    if (!g_appendAchievementRow || g_foreignRowCount <= 0)
        return;

    // The gap opened under the bonus features row when one was found, otherwise
    // simply below the last row the foreign menu drew.
    const float y = g_reorderMainMenu && g_bonusRowIndex >= 0
        ? g_achievementRowY
        : g_lastForeignRow.y + CFrontend::ms_fMenuPositionY;
    const std::wstring label = AchievementRowLabel();
    CFrontend::AddOption(const_cast<wchar_t*>(label.c_str()),
        g_lastForeignRow.x, y, g_lastForeignRow.scaleX, g_lastForeignRow.scaleY,
        CFrontend::ms_menuButton == g_foreignRowCount);
}

namespace
{
    void ProcessMainMenuInput()
    {
        // The closing Escape/B or mouse click must not activate the main menu.
        // Only suppress our own input handler; leave the game's shared queue intact.
        if (waitForMainBackRelease)
        {
            if (RawBackDown() || RawKeyDown(1045) || CPad::NewMouseControllerState.lmb)
                return;
            waitForMainBackRelease=false;
        }
        if (CInputManager::FrontendPressedUp() && --CFrontend::ms_menuButton < 0)
            CFrontend::ms_menuButton = kMainRowCount-1;
        if (CInputManager::FrontendPressedDown() && ++CFrontend::ms_menuButton >= kMainRowCount)
            CFrontend::ms_menuButton = 0;
        if (CInputManager::FrontedMouseHovered())
            CFrontend::ms_menuButton = CFrontend::GetHoveredItem();
        CFrontend::ms_menuButton=max(0,min(CFrontend::ms_menuButton,kMainRowCount-1));
        if (CInputManager::FrontendPressedEscape())
            CFrontend::SetCurrentMenu(MENU_QUIT);
        if (CInputManager::FrontendButtonEnter())
        {
            switch (mainRows[CFrontend::ms_menuButton].action)
            {
            case Play:
            {
                const int lastLevel = CFrontend::GetLastPlayedLevel();
                if (lastLevel == -1)
                {
                    Call<0x5D6A60>();
                    CFrontend::m_bNewGame = true;
                    *reinterpret_cast<int*>(0x7C86F4) = 0;
                    CFrontend::SetCurrentMenu(MENU_GAMMA_SETTINGS);
                }
                else
                    CFrontend::ForceAndPlayLevel(lastLevel, 1);
                break;
            }
            case SelectScene:
                *reinterpret_cast<int*>(0x7C89E4) = 7;
                CFrontend::SetCurrentMenu(MENU_LEVEL_SELECT);
                break;
            case LoadGame: CFrontend::SetCurrentMenu(MENU_START_LOAD_GAME); break;
            case Settings: CFrontend::SetCurrentMenu(MENU_SETTINGS); break;
            case Bonus: CFrontend::SetCurrentMenu(MENU_BONUS_FEATURES); break;
            // MENU_19 (20) is a transient "return to last menu" page, not main.
            // On the next frame its background setup replaces last menu with our
            // gallery, so its SetCurrentMenu(0) would reopen achievements forever.
            case Achievements: AchievementMenu::Open(CFrontend::ms_currentMenu); break;
            case Quit: CFrontend::SetCurrentMenu(MENU_QUIT); break;
            }
        }

        // Preserve the game's cheat-status text without installing cheat features.
        static wchar_t* cheatText = reinterpret_cast<wchar_t*>(0x7D6360);
        const char* key = nullptr;
        switch (CCheatHandler::m_lastCheat)
        {
        case CHEAT_RUNNER: key = "C_RUN"; break;
        case CHEAT_SILENCE: key = "C_SILEN"; break;
        case CHEAT_REGENERATION: key = "C_REGEN"; break;
        case CHEAT_EXPLODE: key = "C_HELI"; break;
        case CHEAT_EQUIPPED: key = "C_FULEQ"; break;
        case CHEAT_SUPERPUNCH: key = "C_SUPUN"; break;
        case CHEAT_RABBIT: key = "C_RABBI"; break;
        case CHEAT_MONKEY: key = "C_MONKE"; break;
        case CHEAT_INVIS: key = "C_INVIS"; break;
        case CHEAT_PIGGSY: key = "C_PIGGS"; break;
        case CHEAT_GODMODE: key = "C_GOD"; break;
        }
        if (key)
            cheatText = CText::GetFromKey16(key);
        if (CCheatHandler::m_bCheatsActive)
        {
            auto* empty = reinterpret_cast<wchar_t*>(0x7D6360);
            CFrontend::PrintInfo(cheatText, empty, empty, empty);
        }
    }

    // Position on screen from the menu's own row number, and back. Our appended
    // row is numbered last but shown right under bonus features.
    int MenuIndexForVisualRow(int visual, int rowCount, int bonusRow)
    {
        if (bonusRow < 0 || visual <= bonusRow)
            return visual;
        return visual == bonusRow + 1 ? rowCount : visual - 1;
    }

    int VisualRowForMenuIndex(int index, int rowCount, int bonusRow)
    {
        if (bonusRow < 0 || index <= bonusRow)
            return index;
        return index == rowCount ? bonusRow + 1 : index + 1;
    }

    // Chain mode: the foreign plugin keeps every row it knows about. We only
    // repair the two wrap edges its own bounds cannot reach, and confirm on our
    // appended row. Returning true means we handled the frame and its handler
    // must be skipped, so the input helpers are never asked twice.
    bool ProcessAppendedRowInput()
    {
        const int ourRow = g_foreignRowsLastFrame;
        if (!g_appendAchievementRow || ourRow <= 0)
            return false;

        // The closing Escape/B or mouse click must not reactivate the main menu.
        if (waitForMainBackRelease)
        {
            if (RawBackDown() || RawKeyDown(1045) || CPad::NewMouseControllerState.lmb)
                return true;
            waitForMainBackRelease=false;
        }

        // These helpers play the navigation sound only when they return true, so
        // a false result may safely be read again by the foreign handler.
        const bool up = CInputManager::FrontendPressedUp();
        const bool down = CInputManager::FrontendPressedDown();
        if (up || down)
        {
            // Our row keeps the last number so the foreign menu's own numbering
            // stays intact, but it is drawn in the gap under bonus features.
            // Selection therefore moves through what the player sees.
            const int bonusRow = g_reorderMainMenu ? g_bonusRowIndex : -1;
            int visual = VisualRowForMenuIndex(CFrontend::ms_menuButton, ourRow, bonusRow);
            if (visual < 0 || visual > ourRow)
                visual = 0;
            if (up)
                visual = visual <= 0 ? ourRow : visual - 1;
            if (down)
                visual = visual >= ourRow ? 0 : visual + 1;
            CFrontend::ms_menuButton = MenuIndexForVisualRow(visual, ourRow, bonusRow);
            return true;
        }

        if (CFrontend::ms_menuButton != ourRow)
            return false;

        // Our row is selected, but take the frame only when the player actually
        // confirms on it. Holding onto every frame would stop the foreign
        // handler from drawing its own info bar, making it blink on this row.
        // Hover and Escape it handles correctly by itself, and its dispatch has
        // no case for our row number, so letting it run costs nothing.
        if (CInputManager::FrontendButtonEnter())
        {
            AchievementMenu::Open(CFrontend::ms_currentMenu);
            return true;
        }
        return false;
    }

    // Returns true when the foreign main menu input handler still has to run.
    bool DispatchMainMenuInput()
    {
        if (!originalMainMenuInput)
        {
            ProcessMainMenuInput();
            return false;
        }
        return !ProcessAppendedRowInput();
    }

    void SelectAchievementBackground()
    {
        if (CFrontend::ms_currentMenu == MENU_ACHIEVEMENTS &&
            AchievementMenu::m_achievementReturnMenu != MENU_PAUSE)
            CFrontend::SetMenuBackground(CFileNames::ms_BonusEpPath.str);
    }

    bool ProcessAndDrawAchievements()
    {
        if (CFrontend::ms_currentMenu != MENU_ACHIEVEMENTS)
            return false;
        AchievementMenu::ProcessAchievementsMenu();
        if (CFrontend::ms_currentMenu == MENU_ACHIEVEMENTS)
            AchievementMenu::AchievementsMenu();
        return true;
    }
}

// These detours enter in the middle of game functions. Preserve live registers
// and flags, replay the replaced instruction, then use the original continuation.
bool __declspec(naked) AchievementMenu::ProcessMainMenu()
{
    static const uintptr_t continuation = 0x6025B6;
    __asm {
        pushfd
        pushad
        call DispatchMainMenuInput
        test al, al
        jnz foreignHandler
        popad
        popfd
        jmp continuation
    foreignHandler:
        popad
        popfd
        jmp dword ptr [originalMainMenuInput]
    }
}

void __declspec(naked) AchievementMenu::HookSelectMenuBackground()
{
    static const uintptr_t continuation = 0x5D70FE;
    __asm {
        cmp dword ptr ds:0x7C86F8, 29
        je customBackground
        cmp dword ptr [originalMenuBackground], 0
        jne pluginBackground
    customBackground:
        pushfd
        pushad
        call SelectAchievementBackground
        popad
        popfd
        mov eax, ds:0x7C86F8
        jmp continuation
    pluginBackground:
        jmp dword ptr [originalMenuBackground]
    }
}

void __declspec(naked) AchievementMenu::HookExecuteMenuProcess()
{
    static const uintptr_t continuation = 0x5D75DC;
    static const uintptr_t afterMenuDispatch = 0x5D7780;
    __asm {
        pushfd
        pushad
        call ProcessAndDrawAchievements
        test al, al
        jnz galleryHandled
        popad
        popfd
        cmp dword ptr [originalMenuDispatch], 0
        jne pluginDispatch
        mov eax, ds:0x7C86F8
        jmp continuation
    galleryHandled:
        popad
        popfd
        mov eax, ds:0x7C86F8
        mov [esp+0x68], eax
        // Escape returning to main/pause must not be processed a second time
        // by the vanilla menu in the same frame.
        jmp afterMenuDispatch
    pluginDispatch:
        jmp dword ptr [originalMenuDispatch]
    }
}
