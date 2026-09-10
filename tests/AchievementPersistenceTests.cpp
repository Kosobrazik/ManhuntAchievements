#include "../source/code/plugin/eAchievementPersistence.h"

#include <Windows.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "ControllerUiTests.h"
#include "MenuBackInputTests.h"
#include "IniFileTests.h"

namespace fs = std::filesystem;

namespace
{
	void Require(bool condition, const char* message)
	{
		if (!condition)
			throw std::runtime_error(message);
	}

	void CorruptFile(const fs::path& path)
	{
		std::ofstream file(path, std::ios::binary | std::ios::trunc);
		const char bytes[] = { 'M', 'H', 'A' };
		file.write(bytes, sizeof(bytes));
	}
}

int main(int argc, char** argv)
{
	const fs::path testRoot = fs::temp_directory_path() /
		("ManhuntAchievements_PersistenceTests_" + std::to_string(GetCurrentProcessId()));
	const fs::path savePath = testRoot / "achievements.dat";
	std::error_code error;
	fs::remove_all(testRoot, error);
	fs::create_directories(testRoot);

	try
	{
		TestControllerUi();
		TestMenuBackInput();
		TestIniByteOrderMark(testRoot);
		if (argc > 1)
		{
			// A standalone DLL must load outside the game without touching fixed game
			// addresses during CRT initialization. Do not invoke InitializeASI here.
			HMODULE plugin = LoadLibraryA(argv[1]);
			Require(plugin != nullptr, "ASI failed to load outside the game");
			const bool hasEntryPoint = GetProcAddress(plugin, "InitializeASI") != nullptr;
			FreeLibrary(plugin);
			Require(hasEntryPoint, "ASI loader entry point is missing");
			std::cout << "ASI load/export/unload: PASS\n";
		}
		AchievementFileData loaded = {};
		Require(!eAchievementPersistence::Load(savePath.wstring(), loaded),
			"a missing profile must not load as valid");

		AchievementFileData first = {};
		first.achievements[ACH_SUBTLE_SLAUGHTER] = { true, 1000 };
		first.scenes[0] = { true, false, 4, 0 };
		Require(eAchievementPersistence::Save(savePath.wstring(), first), "first save failed");
		Require(eAchievementPersistence::Load(savePath.wstring(), loaded), "existing profile failed to load");
		Require(loaded.achievements[ACH_SUBTLE_SLAUGHTER].unlocked,
			"saved achievement was not restored");
		Require(loaded.achievements[ACH_SUBTLE_SLAUGHTER].unlockTime == 1000,
			"unlock timestamp changed");
		Require(loaded.scenes[0].completedFetish && loaded.scenes[0].bestFetishStars == 4,
			"scene result was not restored");

		AchievementFileData second = loaded;
		second.achievements[ACH_HUNTER_SEASON] = { true, 2000 };
		second.scenes[0] = { true, true, 5, 5 };
		Require(eAchievementPersistence::Save(savePath.wstring(), second), "second save failed");
		CorruptFile(savePath);
		loaded = {};
		Require(eAchievementPersistence::Load(savePath.wstring(), loaded),
			"corrupt primary did not recover from backup");
		Require(loaded.achievements[ACH_SUBTLE_SLAUGHTER].unlocked &&
			loaded.achievements[ACH_HUNTER_SEASON].unlocked,
			"backup recovery relocked an achievement");
		Require(loaded.scenes[0].completedHardcore && loaded.scenes[0].bestHardcoreStars == 5,
			"backup recovery lost scene progress");

		CorruptFile(savePath.wstring() + L".bak");
		loaded = {};
		Require(!eAchievementPersistence::Load(savePath.wstring(), loaded),
			"two truncated files must be rejected safely");

		const fs::path legacy = testRoot / "legacy.dat";
		const fs::path standalone = testRoot / "ManhuntAchievements" / "achievements.dat";
		Require(eAchievementPersistence::Save(legacy.wstring(), second), "legacy fixture failed");
		loaded = {};
		Require(eAchievementPersistence::LoadOrMigrate(standalone.wstring(), std::vector<std::wstring>{ legacy.wstring() }, loaded),
			"legacy migration failed");
		Require(loaded.achievements[ACH_HUNTER_SEASON].unlockTime == 2000 &&
			loaded.scenes[0].bestHardcoreStars == 5, "migration lost progress");
		Require(fs::exists(standalone) && fs::exists(standalone.wstring() + L".bak"),
			"migration did not persist an independent profile and backup");
		AchievementFileData legacyAfter = {};
		Require(eAchievementPersistence::Load(legacy.wstring(), legacyAfter) &&
			legacyAfter.achievements[ACH_HUNTER_SEASON].unlockTime == 2000,
			"migration changed the legacy profile");

		Require(eAchievementPersistence::Save(standalone.wstring(), first), "standalone save failed");
		Require(eAchievementPersistence::LoadOrMigrate(standalone.wstring(), std::vector<std::wstring>{ legacy.wstring() }, loaded) &&
			!loaded.achievements[ACH_HUNTER_SEASON].unlocked,
			"existing standalone profile was overwritten by legacy data");
		CorruptFile(standalone);
		Require(eAchievementPersistence::LoadOrMigrate(standalone.wstring(), std::vector<std::wstring>{ legacy.wstring() }, loaded) &&
			!loaded.achievements[ACH_HUNTER_SEASON].unlocked,
			"standalone backup must take priority over legacy data");
		CorruptFile(standalone.wstring() + L".bak");
		Require(!eAchievementPersistence::LoadOrMigrate(standalone.wstring(), std::vector<std::wstring>{ legacy.wstring() }, loaded),
			"corrupt standalone data must not silently import a different legacy profile");

		const fs::path backupMigration = testRoot / "backup-migration.dat";
		CorruptFile(legacy);
		Require(eAchievementPersistence::LoadOrMigrate(backupMigration.wstring(), std::vector<std::wstring>{ legacy.wstring() }, loaded) &&
			loaded.achievements[ACH_HUNTER_SEASON].unlockTime == 2000,
			"migration must recover a valid legacy backup");
		std::cout << "AchievementPersistenceTests: PASS (including standalone migration)\n";
	}
	catch (const std::exception& exception)
	{
		std::cerr << "AchievementPersistenceTests: FAIL: " << exception.what() << "\n";
		fs::remove_all(testRoot, error);
		return 1;
	}

	fs::remove_all(testRoot, error);
	return 0;
}
