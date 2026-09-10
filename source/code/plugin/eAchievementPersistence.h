#pragma once

#include "eAchievements.h"
#include <string>
#include <vector>

struct AchievementFileData
{
	AchievementState achievements[ACH_TOTAL];
	SceneResult scenes[MANHUNT_MAIN_SCENE_COUNT];
};

class eAchievementPersistence
{
public:
	static constexpr uint32_t MAGIC = 0x4341484D;
	static constexpr uint32_t VERSION = 1;

	// Paths stay wide from end to end. Squeezing them through the system's narrow
	// code page loses any character it cannot represent, which is how a profile
	// goes missing for a player whose user folder is not spelled in Latin.
	static bool Load(const std::wstring& path, AchievementFileData& data);
	// Imports from the first readable older location, and only when neither the
	// current profile nor its backup exists. Older locations are never written to.
	static bool LoadOrMigrate(const std::wstring& path,
		const std::vector<std::wstring>& legacyPaths, AchievementFileData& data);
	static bool Save(const std::wstring& path, const AchievementFileData& data);
};
