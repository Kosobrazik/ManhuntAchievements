#pragma once

#include <cstdint>

#define MANHUNT_MAIN_SCENE_COUNT 20

enum eAchievementID
{
	ACH_PSYCHOPATH,
	ACH_HE_NEVER_SAW_IT_COMING,
	ACH_5_STAR_KILLER,
	ACH_DRUG_FREE_IS_THE_WAY_TO_BE,
	ACH_BANG_BANG_BOOM,
	ACH_NO_CRANE_NO_GAIN,
	ACH_ARE_YOU_AFRAID_OF_THE_DARK,
	ACH_PINK_MIST,
	ACH_BRAIN_POWER,
	ACH_ENEMY_EFFICIENT,
	ACH_DEATH_FROM_BEHIND,
	ACH_STICK_TO_THE_SHADOWS,
	ACH_OOH_OOH_AAH_AAH,
	ACH_FOLLOW_THE_WHITE_RABBIT,
	ACH_LINE_EM_UP_KNOCK_EM_DOWN,
	ACH_FUN_WITH_FISTICUFFS,
	ACH_APE_ESCAPE,
	ACH_YOURE_GOING_NOWHERE,
	ACH_HUNTER_SEASON,
	ACH_OFF_WITH_THEIR_HEADS,
	ACH_HARD_AS_NAILS,
	ACH_BRAWL_GAME,
	ACH_MONKEY_SEE_MONKEY_DIE,
	ACH_TIME_2_DIE,
	ACH_DANGEROUS,
	ACH_MURDEROUS,
	ACH_4_STAR_FREAK,
	ACH_5_STAR_FIEND,
	ACH_GETTING_YOUR_HANDS_DIRTY,
	ACH_SWINGING_FOR_THE_FENCES,
	ACH_THE_GRIM_REAPER,
	ACH_SUBTLE_SLAUGHTER,
	ACH_A_SPECIAL_GIFT,
	ACH_TOTAL
};

// Compatibility aliases for the unfinished 0.6.1 implementation.
enum eAchivementID
{
	ACHIEVEMENT_HE_NEVER_SAW_IT_COMING = ACH_HE_NEVER_SAW_IT_COMING,
	ACHIEVEMENT_5_STAR_KILLER = ACH_5_STAR_KILLER,
	ACHIEVEMENT_DRUG_FREE_IS_THE_WAY_TO_BE = ACH_DRUG_FREE_IS_THE_WAY_TO_BE,
	ACHIEVEMENT_BANG_BANG_BOOM = ACH_BANG_BANG_BOOM,
	ACHIEVEMENT_NO_CRANE_NO_GAIN = ACH_NO_CRANE_NO_GAIN
};

struct AchievementDefinition
{
	const char* apiName;
	const char* displayName;
	const char* description;
	bool hidden;
};

struct AchievementState
{
	bool unlocked;
	uint32_t unlockTime;
};

struct SceneResult
{
	bool completedFetish;
	bool completedHardcore;
	int bestFetishStars;
	int bestHardcoreStars;
};

struct SceneAchievementState
{
	int sceneID;
	int difficulty;
	int hunterKills;
	// Every hunter death in the scene, the player's kills included. The Brawl
	// Game counts hunters killing each other towards its own on screen counter.
	int hunterDeaths;
	int executions;
	int painkillersUsed;
	int fistKills;
	int baseballBatKills;
	int sickleKills;
	// The game's own millisecond clock, which is what the PS4 release compares.
	int lastShotgunKillGameTime;
	int shotgunKillsThisFrame;
	bool playerDetected;
	bool cheatsUsed;
	bool active;
};

class eAchievements
{
public:
	static bool m_bIsTransitionDone;
	static bool m_bWantsToPlayUnlock;
	static float m_faStartY;
	static float m_fStartY;
	static int m_nAlpha;

	static void Initialize();
	static void LoadProfile();
	static void Shutdown();
	static void InitHooks();

	static bool Unlock(eAchievementID id);
	static bool IsUnlocked(eAchievementID id);
	static const AchievementDefinition* GetDefinition(eAchievementID id);

	static bool Load();
	static bool Save();
	// Writes the profile only when something changed, and only from moments where
	// a flush to disk cannot be felt: loading, the frontend, or shutdown.
	static void FlushProfile();

	static void OnSceneStart(int sceneID, int difficulty);
	// stars is what the finished run itself earned; bestStars is the level's all
	// time best, which the game keeps without separating difficulties.
	static void OnSceneComplete(int sceneID, int difficulty, int stars, int bestStars,
		int levelTimeSeconds);
	static void OnHunterKilled(int weaponID, bool headShot,
		bool killedByExplodingBarrel, bool killedByRefrigerator);
	// Barrel and refrigerator kills never reach the game's player kill counter,
	// so they arrive from the death handler entry instead of OnHunterKilled.
	static void OnHunterKilledByObject(bool explodingBarrel, bool refrigerator);
	static void OnExecution(int weaponID, int executionStage);
	// Any hunter dying, however it happened. Arrives from the death handler entry.
	static void OnHunterDied();
	static void OnPainkillerUsed();
	static void OnHunterChecksHead();
	static void OnPlayerDetected();
	// Every frame while a scene runs. A cheated run need not contain a single
	// kill - invisibility carries a stealth achievement on its own - and a cheat
	// switched off before the level ends would otherwise leave no trace at all.
	static void PollCheats();
	// Dying keeps the scene's tallies: the PS4 release clears them only when a
	// level is opened, never on death.
	static void OnPlayerDeath();

	static void PlaySlider();

private:
	static bool CanUnlockAchievements(eAchievementID id);
	static bool CanTrackSceneProgress();
	static void InvalidateSceneProgressForCheats();
	// Shared by ordinary kills and executions: an execution kills the hunter just
	// as much, but never reaches the game's own player kill counter.
	static void RegisterHunterKill(int weaponID,
		const char* source, bool fromExecution);
	static void ResetSceneState(bool active, int sceneID, int difficulty);
	static void CheckCampaignAchievements();
	static void CheckPsychopath();
	static void QueuePopup(eAchievementID id);
	static void BeginNextPopup();
};
