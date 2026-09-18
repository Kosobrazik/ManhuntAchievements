#include "eAchievements.h"
#include "eAchievementPersistence.h"
#include "eLog.h"
#include "HookSites.h"
#include "AchievementMenu.h"
#include "../core/AchievementSettings.h"
#include "../manhunt/Cheats.h"
#include "../manhunt/Collectable.h"
#include "../manhunt/EntityManager.h"
#include "../manhunt/Filenames.h"
#include "../manhunt/Frontend.h"
#include "../manhunt/GameInfo.h"
#include "../manhunt/Renderer.h"
#include "../manhunt/Scene.h"
#include "../manhunt/Time.h"
#include "../manhunt/App.h" // Native scene IDs (bonus1, asylum, scrap2, ...).
#include "../manhunt/AI.h"
#include "../manhunt/Player.h"
#include "../manhunt/TypeData.h"
#include "../manhunt/core.h"
#include "../../MHWSF.h"
#include "../../resource.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <deque>
#include <filesystem>
#include <string>
#include <vector>
#include <mmsystem.h>

namespace
{
	const AchievementDefinition g_definitions[ACH_TOTAL] =
	{
		{ "MH_PSYCHOPATH", "Psychopath", "Unlock all achievements.", false },
		{ "MH_HE_NEVER_SAW_IT_COMING", "He Never Saw it Coming", "Perform a gruesome execution.", false },
		{ "MH_5_STAR_KILLER", "5-Star Killer", "Earn a 5-star rating for any level.", false },
		{ "MH_DRUG_FREE_IS_THE_WAY_TO_BE", "Drug Free is the Way to Be", "Complete any scene on Hardcore difficulty without using any painkillers.", false },
		{ "MH_BANG_BANG_BOOM", "Bang, Bang, Boom", "Kill a hunter by shooting an explosive tank.", false },
		{ "MH_NO_CRANE_NO_GAIN", "No Crane, No Gain", "Crush a hunter with a refrigerator.", false },
		{ "MH_ARE_YOU_AFRAID_OF_THE_DARK", "Are You Afraid of the Dark?", "Hide a victim's body in the shadows.", false },
		{ "MH_PINK_MIST", "Pink Mist", "Headshot a hunter with a sniper rifle.", false },
		{ "MH_BRAIN_POWER", "Brain Power", "Use a severed head to lure a hunter.", false },
		{ "MH_ENEMY_EFFICIENT", "Enemy Efficient", "Kill two hunters with one shotgun shell.", false },
		{ "MH_DEATH_FROM_BEHIND", "Death from Behind", "Complete any scene using only executions.", false },
		{ "MH_STICK_TO_THE_SHADOWS", "Stick to the Shadows", "Complete any scene going completely undetected.", false },
		{ "MH_OOH_OOH_AAH_AAH", "Ooh, Ooh, Aah, Aah!", "Play any scene wearing the Monkey skin.", true },
		{ "MH_FOLLOW_THE_WHITE_RABBIT", "Follow the White Rabbit", "Complete any scene wearing the Rabbit skin.", true },
		{ "MH_LINE_EM_UP_KNOCK_EM_DOWN", "Line 'Em Up, Knock 'Em Down", "Kill 30 hunters in Hard as Nails.", true },
		{ "MH_FUN_WITH_FISTICUFFS", "Fun With Fisticuffs", "Survive until 30 hunters have been killed in the Brawl Game.", true },
		{ "MH_APE_ESCAPE", "Ape Escape", "Escape the Zoo alive in Monkey See, Monkey Die!", true },
		{ "MH_YOURE_GOING_NOWHERE", "You're Going Nowhere!", "Kill all the Hoods in Time 2 Die in seven minutes or less.", true },
		{ "MH_HUNTER_SEASON", "Hunter Season", "Kill 45 hunters in one scene.", false },
		{ "MH_OFF_WITH_THEIR_HEADS", "Off With Their Heads!", "Execute 20 hunters in one scene.", false },
		{ "MH_HARD_AS_NAILS", "Hard as Nails", "Earn at least three stars on scenes 1 through 5.", true },
		{ "MH_BRAWL_GAME", "Brawl Game", "Earn at least three stars on scenes 6 through 10.", true },
		{ "MH_MONKEY_SEE_MONKEY_DIE", "Monkey See, Monkey Die!", "Earn at least three stars on scenes 11 through 15.", true },
		{ "MH_TIME_2_DIE", "Time 2 Die", "Earn at least three stars on scenes 16 through 20.", true },
		{ "MH_DANGEROUS", "Dangerous", "Complete every scene on Fetish difficulty.", false },
		{ "MH_MURDEROUS", "Murderous", "Complete every scene on Hardcore difficulty.", false },
		{ "MH_4_STAR_FREAK", "4-Star Freak", "Earn a 4-star rating for every scene on Fetish difficulty.", false },
		{ "MH_5_STAR_FIEND", "5-Star Fiend", "Earn a 5-star rating for every scene on Hardcore difficulty.", false },
		{ "MH_GETTING_YOUR_HANDS_DIRTY", "Getting Your Hands Dirty", "Kill 10 hunters in one scene using only your fists.", false },
		{ "MH_SWINGING_FOR_THE_FENCES", "Swinging for the Fences", "Kill 10 hunters in one scene using a baseball bat.", false },
		{ "MH_THE_GRIM_REAPER", "The Grim Reaper", "Kill 10 hunters in one scene using a sickle.", false },
		{ "MH_SUBTLE_SLAUGHTER", "Subtle Slaughter", "Perform an execution with the chainsaw.", true },
		{ "MH_A_SPECIAL_GIFT", "A Special Gift", "Bring the patrolling hunter's head to the Guard Room during Mouth of Madness.", true }
	};

	AchievementState g_states[ACH_TOTAL] = {};
	SceneResult g_sceneResults[MANHUNT_MAIN_SCENE_COUNT] = {};
	SceneAchievementState g_sceneState =
		{ -1, -1, 0, 0, 0, 0, 0, 0, 0, -1000, 0, false, false, false };
	// Set whenever the profile changed and cleared once it reached disk. Writing
	// happens immediately, and the mark survives a failed write so that loading,
	// the frontend or shutdown can try again.
	bool g_profileDirty = false;
	std::deque<eAchievementID> g_popupQueue;
	eAchievementID g_currentPopup = ACH_TOTAL;
	bool g_popupActive = false;
	uint64_t g_popupElapsedMs = 0;
	ULONGLONG g_popupLastTick = 0;

	void PlayAchievementUnlockSound()
	{
		if (!AchievementSettings::bEnableAchievementSound)
			return;

		HMODULE module = nullptr;
		if (!GetModuleHandleExW(
			GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
			GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			reinterpret_cast<LPCWSTR>(&PlayAchievementUnlockSound), &module))
		{
			return;
		}

		// SND_ASYNC plays the embedded WAV once; deliberately omit SND_LOOP.
		PlaySoundW(MAKEINTRESOURCEW(IDR_ACHIEVEMENT_UNLOCK_SOUND), module,
			SND_RESOURCE | SND_ASYNC | SND_NODEFAULT);
	}

	std::filesystem::path GetUserDirectory()
	{
		const char* userDirectory = CFileNames::GetMyDocumentsDirectory();
		if (!userDirectory || !userDirectory[0])
			return AchievementSettings::ModuleDirectory();
		return std::filesystem::path(userDirectory);
	}

	std::wstring GetAchievementFilePath()
	{
		return (GetUserDirectory() / L"Achievements" / L"achievements.dat").wstring();
	}

	void HookSceneFinalize(int time, int totalKills, int executions, int totalAttempts, int cleanKills)
	{
		// Ask the game what this run itself scored before it folds the result into
		// the level's all time best, which is all GetLevelStars can report later.
		const int level = CGameInfo::GetCurrentLevel();
		const int earnedStars = CGameInfo::ComputeRunRating(time);
		Call<0x5D2320, int, int, int, int, int>(time, totalKills, executions, totalAttempts, cleanKills);
		eAchievements::OnSceneComplete(level, CGameInfo::GetDifficulty(),
			earnedStars, CGameInfo::GetLevelStars(level), time);
	}

	// The hunt objective's sub-goal: 6 is "Check Head" in the game's own table
	// of names (None, KillEnemy, Search, Investigate, Guard Boundary, Check Body,
	// Check Head, ...), kept at this offset of cAI_ObjectiveHuntEnemy.
	constexpr int kHuntSubgoalOffset = 0xB4;
	constexpr int kCheckHeadSubgoal = 6;

	// A hunter turning to look at a severed head it has noticed. The PS4 trophy
	// fires from the same point, the objective's head investigation, and not from
	// the throw: a head is found by sight, and none of the thrown-object noises
	// the game registers ever names one.
	void __fastcall HookStartCheckHead(void* objective, void*, int headID)
	{
		CallMethod<0x50DA30, void*, int>(objective, headID);
		// The transition gives up without a change when the ID no longer names
		// a head, so only a hunter that actually took the sub-goal counts.
		const int subgoal = *reinterpret_cast<const int*>(
			static_cast<const char*>(objective) + kHuntSubgoalOffset);
		if (eLog::Detailed())
			eLog::Verbose(__FUNCTION__, "hunt objective turned to head %d: sub-goal %d",
				headID, subgoal);
		if (subgoal == kCheckHeadSubgoal)
			eAchievements::OnHunterChecksHead();
	}

	int __fastcall HookGetSightingLevel(void* vision, void*, CEntity* target)
	{
		const int sightingLevel =
			CallMethodAndReturn<int, 0x42B170, void*, CEntity*>(vision, target);

		// The game dispatches OnHighSighting/OnVeryHighSighting for levels 3 and 4.
		// Level 9 means that the target is not visible at all.
		if (target == CScene::FindPlayer() && sightingLevel >= 3 && sightingLevel <= 4)
			eAchievements::OnPlayerDetected();

		return sightingLevel;
	}

	// A Special Gift asks for the patrolling hunter's severed head in the Guard
	// Room. The scene can be finished by carrying his body there instead, which
	// opens the door just as well, so completing it proves nothing on its own.
	//
	// The level script settles it for us. Mouth of Madness tests the ending with
	// GetIndexFromInventoryItemName(GetPlayer,'Hunter1_Tower_h') and only then
	// plays Idle_Player_Hold_Up_Head, so carrying that one head is the game's own
	// criterion. Hunter1_Tower is the patrolling Smiley the trophy means; any
	// other severed head does not count.
	bool PlayerCarriesPatrollingHunterHead()
	{
		auto* player = reinterpret_cast<CCharacter*>(CScene::FindPlayer());
		if (!player || !player->m_pInventory)
		{
			eLog::Message(__FUNCTION__, "no player inventory to inspect");
			return false;
		}
		const CInventory* inventory = player->m_pInventory;
		bool carried = false;
		for (int slot = 0; slot < inventory->m_numSlots; ++slot)
		{
			const CCollectable* item = inventory->m_inventory[slot];
			if (!item || !item->m_szName)
				continue;
			eLog::Message(__FUNCTION__, "inventory slot %d holds %s", slot, item->m_szName);
			if (_stricmp(item->m_szName, "Hunter1_Tower_h") == 0)
				carried = true;
		}
		eLog::Message(__FUNCTION__, "patrolling hunter's head carried at the end: %d",
			carried ? 1 : 0);
		return carried;
	}

	bool IsPositionInShadow(CVector* position)
	{
		if (!position)
			return false;

		// This is the same darkness query and threshold used by the game's AI
		// when it decides whether a navigation target is inside a shadow.
		const float sampleRadius = *reinterpret_cast<float*>(0x7983A0);
		const float darkness =
			CallAndReturn<float, 0x472F70, CVector*, float>(position, sampleRadius);
		return darkness <= *reinterpret_cast<float*>(0x69A5B4);
	}

	// The field inherited as m_pDeadBody is really whatever the player is
	// carrying, and the game puts objects down through the same call: a jerry can
	// left in a shadow unlocked this as readily as a corpse did. Only a character
	// body is a victim's body. Carried objects come through as EC_BASIC or
	// EC_COLLECTABLE and a severed head as EC_PEDHEAD, none of which carry the
	// ped bits, so the entity class settles it.
	//
	// This is stricter than the original. The PS4 script hooks the same function
	// and looks at one flag on the player - whether he stands in a hiding zone -
	// without asking what was put down or where it landed, so a can counts there
	// too. We follow the trophy's wording instead of its code, as with the
	// refrigerator.
	bool IsCharacterBody(CEntity* entity)
	{
		return entity && entity->m_pTypeData &&
			(entity->m_pTypeData->m_ecEntityClass & EC_PED) == EC_PED;
	}

	void __fastcall HookFinalizePutDownBody(CPlayer* player, void*)
	{
		CallMethod<0x4A39B0, CPlayer*>(player);

		// CPlayer::PutDownBody clears m_pDeadBody immediately after this call,
		// so the body is already at its final location but is still available here.
		CEntity* carried = player ? player->m_pDeadBody : nullptr;
		if (!carried)
			return;
		const bool body = IsCharacterBody(carried);
		const bool shadow = IsPositionInShadow(carried->GetLocation());
		eLog::Message(__FUNCTION__, "put down %s: class=0x%X, body=%d, in shadow=%d",
			carried->m_szName ? carried->m_szName : "<unnamed>",
			carried->m_pTypeData ? carried->m_pTypeData->m_ecEntityClass : 0,
			body ? 1 : 0, shadow ? 1 : 0);
		if (body && shadow)
			eAchievements::Unlock(ACH_ARE_YOU_AFRAID_OF_THE_DARK);
	}

	float GetFittedPopupScale(const wchar_t* text, float maximumWidth,
		float preferredScale, float minimumScale)
	{
		float scale = preferredScale;
		while (scale > minimumScale &&
			CFrontend::CalculateTextLen(const_cast<wchar_t*>(text), scale,
				FONT_TYPE_DEFAULT) >
			maximumWidth)
		{
			scale -= 0.02f;
		}
		return (std::max)(scale, minimumScale);
	}

	std::wstring TruncatePopupText(const wchar_t* text, float maximumWidth, float scale)
	{
		std::wstring result = text ? text : L"";
		if (CFrontend::CalculateTextLen(const_cast<wchar_t*>(result.c_str()), scale,
			FONT_TYPE_DEFAULT) <= maximumWidth)
		{
			return result;
		}

		const std::wstring ellipsis = L"...";
		while (!result.empty())
		{
			result.pop_back();
			std::wstring candidate = result + ellipsis;
			if (CFrontend::CalculateTextLen(const_cast<wchar_t*>(candidate.c_str()), scale,
				FONT_TYPE_DEFAULT) <= maximumWidth)
			{
				const size_t lastSpace = result.find_last_of(L' ');
				if (lastSpace != std::wstring::npos && result.size() - lastSpace < 12)
					result.erase(lastSpace);
				return result + ellipsis;
			}
		}
		return ellipsis;
	}

	void PrintPopupText(const wchar_t* text, float x, float y, float scale,
		int red, int green, int blue, int alpha)
	{
		CFrontend::SetDrawRGBA(0, 0, 0, alpha);
		CFrontend::Print16(text, x + SCREEN_SCLX(0.002f), y + 0.003f,
			scale, scale, 0.0f, FONT_TYPE_DEFAULT);
		CFrontend::SetDrawRGBA(red, green, blue, alpha);
		CFrontend::Print16(text, x, y, scale, scale, 0.0f, FONT_TYPE_DEFAULT);
	}
}

static_assert(sizeof(g_definitions) / sizeof(g_definitions[0]) == ACH_TOTAL,
	"Every achievement must have exactly one metadata definition.");

bool eAchievements::m_bIsTransitionDone = false;
float eAchievements::m_faStartY = -0.03f;
float eAchievements::m_fStartY = 0.0f;
int eAchievements::m_nAlpha = 255;
bool eAchievements::m_bWantsToPlayUnlock = false;

void eAchievements::Initialize()
{
	memset(g_states, 0, sizeof(g_states));
	memset(g_sceneResults, 0, sizeof(g_sceneResults));
	ResetSceneState(false, -1, -1);
	g_popupQueue.clear();
	g_currentPopup = ACH_TOTAL;
	g_popupActive = false;
	m_bWantsToPlayUnlock = false;

}

void eAchievements::LoadProfile()
{
	// Deliberately not part of Initialize. The profile lives in the folder the
	// game reports, and the game only learns it during its own start-up, which
	// runs after our hooks are already in place.
	if (AchievementSettings::bEnableAchievements)
		Load();
}

void eAchievements::Shutdown()
{
	if (AchievementSettings::bEnableAchievements)
		FlushProfile();
}

void eAchievements::InitHooks()
{
	if (AchievementSettings::bEnableAchievements)
	{
		InjectHook(0x47414B, HookSceneFinalize, PATCH_CALL);
		if (memcmp(reinterpret_cast<const void*>(kCheckHeadSite.address),
				kCheckHeadSite.expected, kCheckHeadSite.size) == 0)
			InjectHook(kCheckHeadSite.address, HookStartCheckHead, PATCH_CALL);
		else
			eLog::Message(__FUNCTION__,
				"Head investigation is already hooked; Brain Power stays unavailable");
		InjectHook(0x519357, HookGetSightingLevel, PATCH_CALL);
		InjectHook(0x5199D8, HookGetSightingLevel, PATCH_CALL);
		InjectHook(0x466742, HookFinalizePutDownBody, PATCH_CALL);
	}
}

namespace
{
	// The game's own "cheats are on" flag is not enough: it is cleared when a
	// scene loads while the individual cheats stay switched on, so a run played
	// under Evil Eyes looked perfectly clean and handed out a stealth trophy for
	// an open brawl. Ask every cheat directly instead. Skin cheats are not here:
	// two achievements are meant to be earned wearing one.
	// Deliverance ends with scripted deaths the game refuses to credit: on that
	// scene alone it takes one kill and two executions back off its own tallies
	// (0x5D20F0), and the PS4 trophies are judged on the reduced numbers. We count
	// the events themselves, so the same amount has to come off here.
	int CreditedKills()
	{
		const int kills = g_sceneState.hunterKills;
		return g_sceneState.sceneID == attic ? kills - 3 : kills;
	}

	int CreditedExecutions()
	{
		const int executions = g_sceneState.executions;
		return g_sceneState.sceneID == attic ? executions - 2 : executions;
	}

	// The rabbit and the monkey can also be worn through another plugin's skin
	// selector, which loads the game's own models - levels/global/charpak/bun_pc
	// and hel_pc - without touching the character id the cheats set. Then the
	// model's root name is what names the skin.
	//
	// Credited only on the game's own terms. It unlocks each skin cheat for five
	// stars on both scenes of its pair and refuses the code otherwise (0x5D43CB):
	// the rabbit for scenes 13 and 14, the monkey for 15 and 16. Wearing the same
	// model by another route asks for the same thing, so the achievement stays as
	// hard to reach as the cheat it belongs to.
	bool WearsStockSkinModel(const char* rootName, int firstSceneOfPair)
	{
		// A cheat skin is worn: that one is judged by its own id, not by a name
		// left behind by whatever was selected before.
		if (CEntityManager::ms_playerCharacterID != 0)
			return false;
		const char* root = reinterpret_cast<const char*>(0x6A94A0);
		if (!root || _stricmp(root, rootName) != 0)
			return false;
		const bool unlocked = CGameInfo::GetLevelStars(firstSceneOfPair) >= 5 &&
			CGameInfo::GetLevelStars(firstSceneOfPair + 1) >= 5;
		eLog::Message(__FUNCTION__, "%s worn through a skin selector; cheat unlocked=%d",
			rootName, unlocked ? 1 : 0);
		return unlocked;
	}

	bool AnyCheatActive()
	{
		return CCheatHandler::m_bCheatsActive ||
			CCheatHandler::m_runner || CCheatHandler::m_silence ||
			CCheatHandler::m_regenerate || CCheatHandler::m_heliumHunters ||
			CCheatHandler::m_fullyarmed || CCheatHandler::m_superPunch ||
			CCheatHandler::m_invisibility || CCheatHandler::m_godMode;
	}
}

bool eAchievements::CanUnlockAchievements(eAchievementID id)
{
	if (!AchievementSettings::bEnableAchievements)
		return false;

	if (!AchievementSettings::bAllowAchievementsWithCheats)
	{
		if (g_sceneState.active && AnyCheatActive())
			InvalidateSceneProgressForCheats();

		const bool cheatsDisqualifyUnlock = AnyCheatActive() ||
			(g_sceneState.active && g_sceneState.cheatsUsed);
		const bool skinOrMetaException = id == ACH_OOH_OOH_AAH_AAH ||
			id == ACH_FOLLOW_THE_WHITE_RABBIT || id == ACH_PSYCHOPATH;
		if (cheatsDisqualifyUnlock && !skinOrMetaException)
			return false;
	}
	return true;
}

bool eAchievements::CanTrackSceneProgress()
{
	if (!AchievementSettings::bEnableAchievements)
		return false;
	if (AchievementSettings::bAllowAchievementsWithCheats)
		return true;
	if (AnyCheatActive())
		InvalidateSceneProgressForCheats();
	return !g_sceneState.cheatsUsed;
}

void eAchievements::PollCheats()
{
	// Only worth asking when cheats are meant to disqualify: the default follows
	// the PS4 script, which never looks at them. Skin cheats are exempt either
	// way - two achievements are earned wearing one.
	if (AchievementSettings::bAllowAchievementsWithCheats || !g_sceneState.active)
		return;
	if (AnyCheatActive())
		InvalidateSceneProgressForCheats();
}

void eAchievements::InvalidateSceneProgressForCheats()
{
	if (!g_sceneState.active || g_sceneState.cheatsUsed)
		return;

	// Name the culprit: the scene is written off from here on, and "cheats were
	// on" alone leaves no way to tell a deliberate cheat from a misread flag.
	eLog::Message(__FUNCTION__,
		"scene %d disqualified by a cheat (flag=%d runner=%d silence=%d regen=%d "
		"helium=%d armed=%d punch=%d invisible=%d god=%d)",
		g_sceneState.sceneID, CCheatHandler::m_bCheatsActive ? 1 : 0,
		CCheatHandler::m_runner, CCheatHandler::m_silence, CCheatHandler::m_regenerate,
		CCheatHandler::m_heliumHunters, CCheatHandler::m_fullyarmed,
		CCheatHandler::m_superPunch, CCheatHandler::m_invisibility,
		CCheatHandler::m_godMode);
	g_sceneState.cheatsUsed = true;
	g_sceneState.hunterKills = 0;
	g_sceneState.hunterDeaths = 0;
	g_sceneState.executions = 0;
	g_sceneState.painkillersUsed = 0;
	g_sceneState.fistKills = 0;
	g_sceneState.baseballBatKills = 0;
	g_sceneState.sickleKills = 0;
	g_sceneState.lastShotgunKillGameTime = -1000;
	g_sceneState.shotgunKillsThisFrame = 0;
}

bool eAchievements::Unlock(eAchievementID id)
{
	if (id < 0 || id >= ACH_TOTAL || !CanUnlockAchievements(id) || g_states[id].unlocked)
		return false;

	g_states[id].unlocked = true;
	const time_t now = time(nullptr);
	g_states[id].unlockTime = now > 0 ? static_cast<uint32_t>(now) : 0;
	QueuePopup(id);
	// Written at once: without the forced platter flush this costs microseconds,
	// so there is no reason to make the player wait for a safe moment.
	g_profileDirty = true;
	FlushProfile();
	eLog::Message(__FUNCTION__, "Unlocked achievement: %s", g_definitions[id].apiName);

	if (id != ACH_PSYCHOPATH)
		CheckPsychopath();
	return true;
}

bool eAchievements::IsUnlocked(eAchievementID id)
{
	return id >= 0 && id < ACH_TOTAL && g_states[id].unlocked;
}

const AchievementDefinition* eAchievements::GetDefinition(eAchievementID id)
{
	return id >= 0 && id < ACH_TOTAL ? &g_definitions[id] : nullptr;
}

bool eAchievements::Load()
{
	AchievementFileData data = {};
	// Older places the profile has lived, newest first. Read once, never written.
	const std::vector<std::wstring> previousLocations = {
		(GetUserDirectory() / L"ManhuntAchievements" / L"achievements.dat").wstring(),
		(GetUserDirectory() / L"achievements.dat").wstring()
	};
	if (!eAchievementPersistence::LoadOrMigrate(GetAchievementFilePath(),
		previousLocations, data))
	{
		eLog::Message(__FUNCTION__, "No valid achievements file found; using a fresh profile");
		return false;
	}

	memcpy(g_states, data.achievements, sizeof(g_states));
	memcpy(g_sceneResults, data.scenes, sizeof(g_sceneResults));
	eLog::Message(__FUNCTION__, "Loaded achievements from file");
	return true;
}

void eAchievements::FlushProfile()
{
	if (!g_profileDirty)
		return;
	// Keep the mark on failure so the next safe moment tries again.
	if (Save())
		g_profileDirty = false;
}

bool eAchievements::Save()
{
	AchievementFileData data = {};
	memcpy(data.achievements, g_states, sizeof(g_states));
	memcpy(data.scenes, g_sceneResults, sizeof(g_sceneResults));
	const bool saved = eAchievementPersistence::Save(GetAchievementFilePath(), data);
	if (!saved)
		eLog::Message(__FUNCTION__, "Failed to save achievements file");
	return saved;
}

void eAchievements::ResetSceneState(bool active, int sceneID, int difficulty)
{
	g_sceneState.sceneID = sceneID;
	g_sceneState.difficulty = difficulty;
	g_sceneState.hunterKills = 0;
	g_sceneState.hunterDeaths = 0;
	g_sceneState.executions = 0;
	g_sceneState.painkillersUsed = 0;
	g_sceneState.fistKills = 0;
	g_sceneState.baseballBatKills = 0;
	g_sceneState.sickleKills = 0;
	g_sceneState.lastShotgunKillGameTime = -1000;
	g_sceneState.shotgunKillsThisFrame = 0;
	g_sceneState.playerDetected = false;
	g_sceneState.cheatsUsed = !AchievementSettings::bAllowAchievementsWithCheats &&
		AnyCheatActive();
	g_sceneState.active = active;
}

void eAchievements::OnSceneStart(int sceneID, int difficulty)
{
	if (!AchievementSettings::bEnableAchievements)
		return;
	// Loading a scene is a safe moment for anything still unwritten.
	FlushProfile();
	ResetSceneState(true, sceneID, difficulty);
	eLog::Message(__FUNCTION__,
		"scene %d started: difficulty=%d, player skin=%d, cheats active=%d "
		"(flag=%d runner=%d silence=%d regen=%d helium=%d armed=%d punch=%d "
		"invisible=%d god=%d)",
		sceneID, difficulty, CEntityManager::ms_playerCharacterID,
		AnyCheatActive() ? 1 : 0, CCheatHandler::m_bCheatsActive ? 1 : 0,
		CCheatHandler::m_runner, CCheatHandler::m_silence, CCheatHandler::m_regenerate,
		CCheatHandler::m_heliumHunters, CCheatHandler::m_fullyarmed,
		CCheatHandler::m_superPunch, CCheatHandler::m_invisibility,
		CCheatHandler::m_godMode);
	if (CEntityManager::ms_playerCharacterID == 2 ||
		WearsStockSkinModel("Monkey_Player", 14))
		Unlock(ACH_OOH_OOH_AAH_AAH);
}

void eAchievements::OnSceneComplete(int sceneID, int difficulty, int stars, int bestStars,
	int levelTimeSeconds)
{
	eLog::Message(__FUNCTION__,
		"scene %d finished in %d s: difficulty=%d, earned this run=%d, level best=%d, "
		"player skin=%d, tracked scene=%d, active=%d, kills=%d, deaths=%d, executions=%d",
		sceneID, levelTimeSeconds, difficulty, stars, bestStars,
		CEntityManager::ms_playerCharacterID,
		g_sceneState.sceneID, g_sceneState.active ? 1 : 0,
		g_sceneState.hunterKills, g_sceneState.hunterDeaths, g_sceneState.executions);
	if (!g_sceneState.active || g_sceneState.sceneID != sceneID)
		return;

	// These two achievements intentionally require cheat-unlocked character skins,
	// worn either by the cheat itself or by a skin selector - see WearsStockSkinModel.
	if (CEntityManager::ms_playerCharacterID == 1 ||
		WearsStockSkinModel("Bun_Bod_P", 12))
		Unlock(ACH_FOLLOW_THE_WHITE_RABBIT);
	// Wearing the Monkey skin is also checked here, not only at scene start: the
	// skin can be applied after the level has loaded, which the start check misses.
	if (CEntityManager::ms_playerCharacterID == 2 ||
		WearsStockSkinModel("Monkey_Player", 14))
		Unlock(ACH_OOH_OOH_AAH_AAH);

	// A cheated attempt must not contribute kills, ratings or campaign completion.
	if (!CanTrackSceneProgress())
	{
		g_sceneState.active = false;
		return;
	}

	if (stars >= 5)
		Unlock(ACH_5_STAR_KILLER);
	// Fetish caps at four stars. A higher value here would mean the rating did
	// not come from this run after all, so refuse to record it as one.
	if (difficulty == DIFFICULTY_FETISH && stars > 4)
	{
		eLog::Message(__FUNCTION__, "implausible Fetish rating %d; not recorded", stars);
		stars = -1;
	}
	if (difficulty == DIFFICULTY_HARDCORE && g_sceneState.painkillersUsed == 0)
		Unlock(ACH_DRUG_FREE_IS_THE_WAY_TO_BE);
	if (g_sceneState.hunterKills > 0 && g_sceneState.executions == g_sceneState.hunterKills)
		Unlock(ACH_DEATH_FROM_BEHIND);
	if (!g_sceneState.playerDetected)
		Unlock(ACH_STICK_TO_THE_SHADOWS);
	if (sceneID == bonus1 && g_sceneState.hunterDeaths >= 30)
		Unlock(ACH_LINE_EM_UP_KNOCK_EM_DOWN);
	else if (sceneID == bonus2)
		Unlock(ACH_APE_ESCAPE);
	else if (sceneID == bonus3 && g_sceneState.hunterDeaths >= 30)
		Unlock(ACH_FUN_WITH_FISTICUFFS);
	// Level time in seconds, the same figure the game measures its own par times
	// against. The seven minutes are counted off the mission timer, which starts
	// twenty three seconds into the level; the PS4 release adds exactly that back.
	else if (sceneID == weasel && levelTimeSeconds <= 7 * 60 + 23)
		Unlock(ACH_YOURE_GOING_NOWHERE);

	if (sceneID == asylum && PlayerCarriesPatrollingHunterHead())
		Unlock(ACH_A_SPECIAL_GIFT);

	if (CGameInfo::IsMainScene(sceneID))
	{
		SceneResult& result = g_sceneResults[sceneID];
		if (difficulty == DIFFICULTY_HARDCORE)
		{
			result.completedHardcore = true;
			if (stars >= 0)
				result.bestHardcoreStars = (std::max)(result.bestHardcoreStars, stars);
		}
		else if (difficulty == DIFFICULTY_FETISH)
		{
			result.completedFetish = true;
			if (stars >= 0)
				result.bestFetishStars = (std::max)(result.bestFetishStars, stars);
		}
		CheckCampaignAchievements();
	}

	g_sceneState.active = false;
	g_profileDirty = true;
	FlushProfile();
}

void eAchievements::OnHunterKilled(int weaponID, bool headShot,
	bool killedByExplodingBarrel, bool killedByRefrigerator)
{
	if (!g_sceneState.active || !CanTrackSceneProgress())
	{
		eLog::Message(__FUNCTION__,
			"kill ignored: scene active=%d, cheats active=%d, scene disqualified=%d",
			g_sceneState.active ? 1 : 0, AnyCheatActive() ? 1 : 0,
			g_sceneState.cheatsUsed ? 1 : 0);
		return;
	}
	if (killedByExplodingBarrel)
		Unlock(ACH_BANG_BANG_BOOM);
	if (killedByRefrigerator)
		Unlock(ACH_NO_CRANE_NO_GAIN);

	// An object doing the killing was not wielded, so whatever happens to be
	// selected in the inventory must not be credited for it. Otherwise one barrel
	// taking out two hunters counts as two felled by the shotgun held at the time,
	// and a crane drop counts as a swing of the bat.
	const bool killedByObject = killedByExplodingBarrel || killedByRefrigerator;
	// The head shot is deduced from the game's own numbers rather than from any
	// flag: a hunter has 100 hit points, the rifle's shot does 66.7 with a torso
	// or arm multiplier of 1.0, and its weapon data asks for a single head shot
	// to kill. A hunter who dies from one hit while holding more health than a
	// body hit can take away was therefore hit in the head.
	if (!killedByObject && headShot &&
		(weaponID == CT_SNIPER_RIFLE || weaponID == CT_SNIPER_RIFLE_SILENCED))
		Unlock(ACH_PINK_MIST);

	RegisterHunterKill(killedByObject ? -1 : weaponID,
		killedByObject ? "object kill" : "kill", false);
}

void eAchievements::RegisterHunterKill(int weaponID,
	const char* source, bool fromExecution)
{
	++g_sceneState.hunterKills;

	switch (weaponID)
	{
	case CT_FISTS:
		// No clean streak is asked for: the PS4 release simply counts bare handed
		// kills within the scene, whatever else was used in between. Executions do
		// not count towards them there, only towards the bat and the sickle.
		if (fromExecution)
			break;
		if (++g_sceneState.fistKills >= 10)
			Unlock(ACH_GETTING_YOUR_HANDS_DIRTY);
		break;
	// The small bat is not one of the three the PS4 release accepts.
	case CT_BASEBALL_BAT:
	case CT_W_BASEBALL_BAT:
	case CT_BASEBALL_BAT_BLADES:
		if (++g_sceneState.baseballBatKills >= 10)
			Unlock(ACH_SWINGING_FOR_THE_FENCES);
		break;
	case CT_SICKLE:
		if (++g_sceneState.sickleKills >= 10)
			Unlock(ACH_THE_GRIM_REAPER);
		break;
	case CT_SHOTGUN:
	case CT_SHOTGUN_TORCH:
	case CT_SAWNOFF:
		// The PS4 release asks for both kills to carry the same timestamp off the
		// game's clock, which amounts to the same frame. A console frame is a
		// thirtieth of a second, so allow that much: on a PC running far faster the
		// two deaths can land a frame or two apart and still come from one shell.
		const int now = CGameTime::ms_currGameTime;
		const int since = now - g_sceneState.lastShotgunKillGameTime;
		if (since >= 0 && since <= 33)
			++g_sceneState.shotgunKillsThisFrame;
		else
		{
			g_sceneState.lastShotgunKillGameTime = now;
			g_sceneState.shotgunKillsThisFrame = 1;
		}
		if (g_sceneState.shotgunKillsThisFrame >= 2)
			Unlock(ACH_ENEMY_EFFICIENT);
		break;
	}

	// Written after the weapon totals move, so the line shows what this kill made
	// them rather than what they were before it.
	eLog::Message(__FUNCTION__,
		"%s %d counted: weapon=%d scene=%d (fists=%d bat=%d sickle=%d); "
		"clocks: game=%d frame=%d tick=%llu",
		source, g_sceneState.hunterKills, weaponID,
		g_sceneState.sceneID, g_sceneState.fistKills,
		g_sceneState.baseballBatKills, g_sceneState.sickleKills,
		CGameTime::ms_currGameTime, CGameTime::ms_currFrame, GetTickCount64());

	if (CreditedKills() >= 45)
		Unlock(ACH_HUNTER_SEASON);
}

void eAchievements::OnHunterKilledByObject(bool explodingBarrel, bool refrigerator)
{
	if (!g_sceneState.active || !CanTrackSceneProgress())
	{
		eLog::Message(__FUNCTION__, "object kill ignored: scene active=%d, disqualified=%d",
			g_sceneState.active ? 1 : 0, g_sceneState.cheatsUsed ? 1 : 0);
		return;
	}
	eLog::Message(__FUNCTION__, "object kill in scene %d: barrel=%d fridge=%d",
		g_sceneState.sceneID, explodingBarrel ? 1 : 0, refrigerator ? 1 : 0);
	// Killing with an object is not killing with fists either.
	g_sceneState.fistKills = 0;
	if (explodingBarrel)
		Unlock(ACH_BANG_BANG_BOOM);
	// The trophy names no level, and refrigerators stand in several of them.
	if (refrigerator)
		Unlock(ACH_NO_CRANE_NO_GAIN);
}

void eAchievements::OnExecution(int weaponID, int executionStage)
{
	if (g_sceneState.active && !CanTrackSceneProgress())
		return;

	if (executionStage >= 2)
		Unlock(ACH_HE_NEVER_SAW_IT_COMING);

	// Deliverance gives Cash CT_CHAINSAW_PLAYER after Piggsy; custom maps and
	// the developer menu can still provide the base CT_CHAINSAW collectable.
	if (weaponID == CT_CHAINSAW || weaponID == CT_CHAINSAW_PLAYER)
		Unlock(ACH_SUBTLE_SLAUGHTER);

	if (!g_sceneState.active)
		return;
	++g_sceneState.executions;
	if (CreditedExecutions() >= 20)
		Unlock(ACH_OFF_WITH_THEIR_HEADS);
	// An execution kills the hunter with the equipped weapon, but the game's own
	// player kill counter never sees it, so the weapon totals are recorded here.
	// Without this a scene cleared entirely by executions counts zero kills, which
	// also made "using only executions" impossible to satisfy.
	RegisterHunterKill(weaponID, "execution", true);
}

void eAchievements::OnHunterDied()
{
	if (!g_sceneState.active || !CanTrackSceneProgress())
		return;
	++g_sceneState.hunterDeaths;
	// Both bonus scenes ask for thirty hunters dead, not for thirty killed by the
	// player and not for finishing the scene: the PS4 release reads the level
	// script's own counter for either, and hunters finishing each other off feed
	// that counter just the same.
	if (g_sceneState.hunterDeaths >= 30)
	{
		if (g_sceneState.sceneID == bonus1)
			Unlock(ACH_LINE_EM_UP_KNOCK_EM_DOWN);
		else if (g_sceneState.sceneID == bonus3)
			Unlock(ACH_FUN_WITH_FISTICUFFS);
	}
}

void eAchievements::OnPainkillerUsed()
{
	if (g_sceneState.active && CanTrackSceneProgress())
		++g_sceneState.painkillersUsed;
}

void eAchievements::OnHunterChecksHead()
{
	if (g_sceneState.active && CanTrackSceneProgress())
	{
		Unlock(ACH_BRAIN_POWER);
		return;
	}
	eLog::Message(__FUNCTION__, "head investigation ignored: scene active=%d, cheats active=%d, "
		"scene disqualified=%d", g_sceneState.active ? 1 : 0, AnyCheatActive() ? 1 : 0,
		g_sceneState.cheatsUsed ? 1 : 0);
}

void eAchievements::OnPlayerDetected()
{
	if (g_sceneState.active && CanTrackSceneProgress())
		g_sceneState.playerDetected = true;
}

void eAchievements::CheckCampaignAchievements()
{
	const eAchievementID groupAchievements[4] =
	{
		ACH_HARD_AS_NAILS,
		ACH_BRAWL_GAME,
		ACH_MONKEY_SEE_MONKEY_DIE,
		ACH_TIME_2_DIE
	};

	for (int group = 0; group < 4; ++group)
	{
		bool earned = true;
		for (int scene = group * 5; scene < group * 5 + 5; ++scene)
		{
			// The game keeps its own rating per level and unlocks the bonus
			// scenes from exactly this comparison, so read it rather than our
			// own record: ratings earned before the plugin was installed count.
			if (CGameInfo::GetLevelStars(scene) < 3)
			{
				earned = false;
				break;
			}
		}
		if (earned)
			Unlock(groupAchievements[group]);
	}

	bool allFetish = true;
	bool allHardcore = true;
	bool allFetishFourStars = true;
	bool allHardcoreFiveStars = true;
	for (int scene = 0; scene < MANHUNT_MAIN_SCENE_COUNT; ++scene)
	{
		const SceneResult& result = g_sceneResults[scene];
		allFetish &= result.completedFetish;
		allHardcore &= result.completedHardcore;
		allFetishFourStars &= result.bestFetishStars >= 4;
		allHardcoreFiveStars &= result.bestHardcoreStars >= 5;
	}
	if (allFetish)
		Unlock(ACH_DANGEROUS);
	if (allHardcore)
		Unlock(ACH_MURDEROUS);
	if (allFetishFourStars)
		Unlock(ACH_4_STAR_FREAK);
	if (allHardcoreFiveStars)
		Unlock(ACH_5_STAR_FIEND);
}

void eAchievements::OnPlayerDeath()
{
	if (!g_sceneState.active)
		return;
	// Dying loads the last checkpoint, so the scene goes back to the progress it
	// had there: kills made before it still stand, kills made after it are lost.
	// Without a checkpoint the copy is the state the scene started with.
	// Dying drops into a loading screen, another safe moment to write.
	// Dying drops into a loading screen, a safe moment to write. Nothing else
	// happens: the PS4 release clears the scene's tallies only when a level is
	// opened, never on death, which is what lets the known method for "Getting
	// Your Hands Dirty" work at all - kill one with bare hands, die, repeat.
	FlushProfile();
	eLog::Message(__FUNCTION__,
		"death in scene %d: progress kept (kills=%d executions=%d, fists=%d bat=%d "
		"sickle=%d); disqualified by: seen=%d painkillers=%d cheats=%d",
		g_sceneState.sceneID, g_sceneState.hunterKills, g_sceneState.executions,
		g_sceneState.fistKills, g_sceneState.baseballBatKills, g_sceneState.sickleKills,
		g_sceneState.playerDetected ? 1 : 0, g_sceneState.painkillersUsed,
		g_sceneState.cheatsUsed ? 1 : 0);
}

void eAchievements::CheckPsychopath()
{
	for (int i = 1; i < ACH_TOTAL; ++i)
	{
		if (!g_states[i].unlocked)
			return;
	}
	Unlock(ACH_PSYCHOPATH);
}

void eAchievements::QueuePopup(eAchievementID id)
{
	g_popupQueue.push_back(id);
	m_bWantsToPlayUnlock = true;
}

void eAchievements::BeginNextPopup()
{
	if (g_popupQueue.empty())
	{
		g_popupActive = false;
		g_currentPopup = ACH_TOTAL;
		m_bWantsToPlayUnlock = false;
		return;
	}

	g_currentPopup = g_popupQueue.front();
	g_popupQueue.pop_front();
	g_popupActive = true;
	g_popupElapsedMs = 0;
	g_popupLastTick = GetTickCount64();
	m_fStartY = 0.0f;
	m_faStartY = -0.17f;
	m_nAlpha = 255;
	m_bIsTransitionDone = false;
	m_bWantsToPlayUnlock = true;
	PlayAchievementUnlockSound();
}

void eAchievements::PlaySlider()
{
	if (!CFrontend::m_gameIsRunning)
	{
		g_popupLastTick = GetTickCount64();
		return;
	}
	if (!g_popupActive)
		BeginNextPopup();
	if (!g_popupActive || g_currentPopup < 0 || g_currentPopup >= ACH_TOTAL)
		return;

	const ULONGLONG now = GetTickCount64();
	if (g_popupLastTick != 0)
		g_popupElapsedMs += now - g_popupLastTick;
	g_popupLastTick = now;

	const uint64_t slideDuration = 450;
	const uint64_t holdUntil = 5000;
	const uint64_t finishAt = 5900;
	const float slide = (std::min)(1.0f, static_cast<float>(g_popupElapsedMs) / slideDuration);
	const float smoothSlide = slide * slide * (3.0f - 2.0f * slide);
	m_faStartY = -0.17f + (0.20f * smoothSlide);
	m_fStartY = m_faStartY;
	m_bIsTransitionDone = slide >= 1.0f;
	if (g_popupElapsedMs > holdUntil)
	{
		const uint64_t fadeElapsed = (std::min)(finishAt - holdUntil, g_popupElapsedMs - holdUntil);
		m_nAlpha = 255 - static_cast<int>((255 * fadeElapsed) / (finishAt - holdUntil));
	}
	else
		m_nAlpha = 255;

	const std::wstring achievementName =
		AchievementMenu::GetLocalizedAchievementName(g_currentPopup);
	const std::wstring achievementDescription =
		AchievementMenu::GetLocalizedAchievementDescription(g_currentPopup);
	const std::wstring achievementUnlocked =
		AchievementMenu::GetLocalizedAchievementUnlockedText();
	const float panelX = SCREEN_FROM_CENTER(0.18f);
	const float panelY = m_faStartY;
	const float panelWidth = SCREEN_SCLX(0.64f);
	const float panelHeight = 0.145f;
	const float iconX = panelX + SCREEN_SCLX(0.012f);
	const float iconY = panelY + 0.012f;
	// 0.091 x 0.121 is square in pixels in the game's 4:3 reference space.
	const float iconWidth = SCREEN_SCLX(0.091f);
	const float iconHeight = 0.121f;
	const float textX = panelX + SCREEN_SCLX(0.135f);
	const float textMaximumWidth = SCREEN_SCLX(0.485f);
	const int shadowAlpha = (150 * m_nAlpha) / 255;

	// Dark game-style notification body with a restrained PSN-like highlight.
	CRenderer::DrawQuad2d(panelX + SCREEN_SCLX(0.006f), panelY + 0.008f,
		panelWidth, panelHeight, 0, 0, 0, shadowAlpha, 0);
	CRenderer::DrawQuad2d(panelX, panelY, panelWidth, panelHeight,
		12, 12, 14, (220 * m_nAlpha) / 255, 0);
	CRenderer::DrawQuad2d(panelX, panelY, SCREEN_SCLX(0.006f), panelHeight,
		125, 135, 175, m_nAlpha, 0);
	CRenderer::DrawQuad2d(panelX, panelY, panelWidth, 0.003f,
		135, 135, 145, (180 * m_nAlpha) / 255, 0);
	CRenderer::DrawQuad2d(panelX, panelY + panelHeight - 0.003f, panelWidth, 0.003f,
		55, 55, 60, (180 * m_nAlpha) / 255, 0);
	CRenderer::DrawQuad2d(iconX - SCREEN_SCLX(0.003f), iconY - 0.003f,
		iconWidth + SCREEN_SCLX(0.006f), iconHeight + 0.006f,
		65, 65, 70, (210 * m_nAlpha) / 255, 0);

	const int iconTexture = AchievementMenu::GetAchievementSmallTexture(g_currentPopup);
	if (iconTexture)
	{
		CRenderer::DrawQuad2d(iconX, iconY, iconWidth, iconHeight,
			255, 255, 255, m_nAlpha, iconTexture);
	}

	PrintPopupText(achievementUnlocked.c_str(), textX, panelY + 0.012f, 0.30f,
		150, 160, 205, m_nAlpha);

	const float nameScale = GetFittedPopupScale(achievementName.c_str(),
		textMaximumWidth, 0.58f, 0.42f);
	PrintPopupText(achievementName.c_str(), textX, panelY + 0.044f, nameScale,
		245, 245, 245, m_nAlpha);

	// The description must stay visibly smaller than the name, so its ceiling
	// follows whatever scale the name settled on rather than being fixed.
	const float descriptionScale = GetFittedPopupScale(achievementDescription.c_str(),
		textMaximumWidth, (std::min)(0.40f, nameScale - 0.10f), 0.28f);
	const std::wstring description = TruncatePopupText(achievementDescription.c_str(),
		textMaximumWidth, descriptionScale);
	PrintPopupText(description.c_str(), textX, panelY + 0.100f, descriptionScale,
		170, 170, 175, m_nAlpha);

	if (g_popupElapsedMs >= finishAt)
	{
		g_popupActive = false;
		BeginNextPopup();
	}
}

