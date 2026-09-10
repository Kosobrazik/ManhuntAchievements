#include "eAchievementPersistence.h"

#include <Windows.h>
#include <algorithm>
#include <filesystem>
#include <fstream>

namespace
{
	template <typename T>
	bool ReadValue(std::ifstream& file, T& value)
	{
		file.read(reinterpret_cast<char*>(&value), sizeof(T));
		return file.good();
	}

	template <typename T>
	bool WriteValue(std::ofstream& file, const T& value)
	{
		file.write(reinterpret_cast<const char*>(&value), sizeof(T));
		return file.good();
	}

	bool LoadFile(const std::wstring& path, AchievementFileData& output)
	{
		std::ifstream file(std::filesystem::path(path), std::ios::binary);
		if (!file)
			return false;

		uint32_t magic = 0;
		uint32_t version = 0;
		uint32_t achievementCount = 0;
		uint32_t sceneCount = 0;
		if (!ReadValue(file, magic) || !ReadValue(file, version) ||
			!ReadValue(file, achievementCount) || !ReadValue(file, sceneCount))
			return false;

		if (magic != eAchievementPersistence::MAGIC || version == 0 ||
			version > eAchievementPersistence::VERSION || achievementCount > ACH_TOTAL ||
			sceneCount > MANHUNT_MAIN_SCENE_COUNT)
			return false;

		AchievementFileData loaded = {};
		for (uint32_t i = 0; i < achievementCount; ++i)
		{
			uint8_t unlocked = 0;
			if (!ReadValue(file, unlocked) || !ReadValue(file, loaded.achievements[i].unlockTime))
				return false;
			if (unlocked > 1)
				return false;
			loaded.achievements[i].unlocked = unlocked != 0;
		}

		for (uint32_t i = 0; i < sceneCount; ++i)
		{
			uint8_t completedFetish = 0;
			uint8_t completedHardcore = 0;
			if (!ReadValue(file, completedFetish) || !ReadValue(file, completedHardcore) ||
				!ReadValue(file, loaded.scenes[i].bestFetishStars) ||
				!ReadValue(file, loaded.scenes[i].bestHardcoreStars))
				return false;
			if (completedFetish > 1 || completedHardcore > 1 ||
				loaded.scenes[i].bestFetishStars < 0 || loaded.scenes[i].bestFetishStars > 5 ||
				loaded.scenes[i].bestHardcoreStars < 0 || loaded.scenes[i].bestHardcoreStars > 5)
				return false;
			loaded.scenes[i].completedFetish = completedFetish != 0;
			loaded.scenes[i].completedHardcore = completedHardcore != 0;
		}

		char trailingByte = 0;
		if (file.read(&trailingByte, 1))
			return false;

		output = loaded;
		return true;
	}
}

bool eAchievementPersistence::Load(const std::wstring& path, AchievementFileData& data)
{
	AchievementFileData primary = {};
	AchievementFileData backup = {};
	const bool primaryValid = LoadFile(path, primary);
	const bool backupValid = LoadFile(path + L".bak", backup);
	if (!primaryValid && !backupValid)
		return false;

	data = primaryValid ? primary : backup;
	if (primaryValid && backupValid)
	{
		for (int i = 0; i < ACH_TOTAL; ++i)
		{
			if (!data.achievements[i].unlocked && backup.achievements[i].unlocked)
				data.achievements[i] = backup.achievements[i];
			else if (data.achievements[i].unlocked && backup.achievements[i].unlocked &&
				backup.achievements[i].unlockTime != 0 &&
				(data.achievements[i].unlockTime == 0 || backup.achievements[i].unlockTime < data.achievements[i].unlockTime))
				data.achievements[i].unlockTime = backup.achievements[i].unlockTime;
		}
		for (int i = 0; i < MANHUNT_MAIN_SCENE_COUNT; ++i)
		{
			data.scenes[i].completedFetish |= backup.scenes[i].completedFetish;
			data.scenes[i].completedHardcore |= backup.scenes[i].completedHardcore;
			data.scenes[i].bestFetishStars = (std::max)(data.scenes[i].bestFetishStars, backup.scenes[i].bestFetishStars);
			data.scenes[i].bestHardcoreStars = (std::max)(data.scenes[i].bestHardcoreStars, backup.scenes[i].bestHardcoreStars);
		}
	}
	return true;
}

bool eAchievementPersistence::LoadOrMigrate(const std::wstring& path,
	const std::vector<std::wstring>& legacyPaths, AchievementFileData& data)
{
	if (Load(path, data))
		return true;
	std::error_code error;
	if (std::filesystem::exists(path, error) || error)
		return false;
	if (std::filesystem::exists(path + L".bak", error) || error)
		return false;
	for (const std::wstring& legacy : legacyPaths)
	{
		if (legacy.empty() || !Load(legacy, data))
			continue;
		// The older file is left as it is. Even if this first write fails, the
		// recovered progress stays in memory and ordinary saves will retry.
		Save(path, data);
		return true;
	}
	return false;
}

bool eAchievementPersistence::Save(const std::wstring& path, const AchievementFileData& data)
{
	namespace fs = std::filesystem;
	std::error_code error;
	const fs::path finalPath(path);
	const fs::path temporaryPath(path + L".tmp");
	const fs::path backupPath(path + L".bak");
	const fs::path backupTemporaryPath(path + L".bak.tmp");

	if (!finalPath.parent_path().empty())
		fs::create_directories(finalPath.parent_path(), error);
	if (error)
		return false;

	std::ofstream file(temporaryPath, std::ios::binary | std::ios::trunc);
	if (!file)
		return false;

	const uint32_t achievementCount = ACH_TOTAL;
	const uint32_t sceneCount = MANHUNT_MAIN_SCENE_COUNT;
	if (!WriteValue(file, MAGIC) || !WriteValue(file, VERSION) ||
		!WriteValue(file, achievementCount) || !WriteValue(file, sceneCount))
		return false;

	for (int i = 0; i < ACH_TOTAL; ++i)
	{
		const uint8_t unlocked = data.achievements[i].unlocked ? 1 : 0;
		if (!WriteValue(file, unlocked) || !WriteValue(file, data.achievements[i].unlockTime))
			return false;
	}

	for (int i = 0; i < MANHUNT_MAIN_SCENE_COUNT; ++i)
	{
		const uint8_t completedFetish = data.scenes[i].completedFetish ? 1 : 0;
		const uint8_t completedHardcore = data.scenes[i].completedHardcore ? 1 : 0;
		if (!WriteValue(file, completedFetish) || !WriteValue(file, completedHardcore) ||
			!WriteValue(file, data.scenes[i].bestFetishStars) ||
			!WriteValue(file, data.scenes[i].bestHardcoreStars))
			return false;
	}

	file.flush();
	if (!file.good())
		return false;
	file.close();

	error.clear();
	fs::copy_file(temporaryPath, backupTemporaryPath, fs::copy_options::overwrite_existing, error);
	if (error)
	{
		fs::remove(temporaryPath, error);
		return false;
	}

	// Deliberately without MOVEFILE_WRITE_THROUGH. That flag waits for the data
	// to reach the platter, which guards against losing power, not against the
	// game crashing: writes already handed to the system survive a dead process.
	// The wait it costs is long enough to be felt when an achievement unlocks in
	// the middle of a fight, so the trade is not worth making here.
	if (!MoveFileExW(backupTemporaryPath.c_str(), backupPath.c_str(),
		MOVEFILE_REPLACE_EXISTING))
	{
		fs::remove(temporaryPath, error);
		fs::remove(backupTemporaryPath, error);
		return false;
	}

	if (!MoveFileExW(temporaryPath.c_str(), finalPath.c_str(),
		MOVEFILE_REPLACE_EXISTING))
	{
		fs::remove(temporaryPath, error);
		return false;
	}
	return true;
}
