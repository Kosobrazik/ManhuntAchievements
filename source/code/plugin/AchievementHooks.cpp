#include "AchievementHooks.h"
#include "eAchievements.h"
#include "../manhunt/core.h"
#include "../manhunt/Frontend.h"
#include "../manhunt/GameInfo.h"
#include "../manhunt/AmmoWeapon.h"
#include "../manhunt/Inventory.h"
#include "../manhunt/Player.h"
#include "../manhunt/Scene.h"
#include "../manhunt/Time.h"
#include "../manhunt/TypeData.h"
#include "../manhunt/EntityManager.h"
#include "../manhunt/Vector.h"
#include "../manhunt/Shot.h"
#include "../manhunt/Weapon.h"
#include <cctype>
#include <cstring>
#include "HookSites.h"
#include "eLog.h"
#include "PluginCompatibility.h"
namespace
{
    uintptr_t originalShutdown=0x489D50, originalDeath=0x45B8A0;
    uintptr_t originalSceneStart=0x5D4AF0, originalSceneReset=0x5D4AF0;
    uintptr_t originalRender=0x5D7070, originalExecution=0x5B62E0;
    uintptr_t originalKill=0x5B63A0, originalPainkiller=0x5DFB60;
	bool ContainsInsensitive(const char* text, const char* needle)
	{
		if (!text || !needle || !needle[0])
			return false;
		for (const char* start = text; *start; ++start)
		{
			const char* left = start;
			const char* right = needle;
			while (*left && *right &&
				std::tolower(static_cast<unsigned char>(*left)) ==
				std::tolower(static_cast<unsigned char>(*right)))
			{
				++left;
				++right;
			}
			if (!*right)
				return true;
		}
		return false;
	}

	bool EntityMatches(CEntity* entity, const char* token)
	{
		if (!entity)
			return false;
		if (ContainsInsensitive(entity->m_szName, token))
			return true;
		return entity->m_pTypeData &&
			ContainsInsensitive(entity->m_pTypeData->m_szRecordName, token);
	}

	void __cdecl RecordHunterKillFromHook(void* hunter)
	{
		AchievementHooks::RecordHunterKill(hunter);
	}

	void __declspec(naked) HookKillsStatGateway()
	{
		_asm
		{
			pushfd
			pushad
			push ebx
			call RecordHunterKillFromHook
			add esp, 4
			popad
			popfd
			jmp dword ptr [originalKill]
		}
	}
}

namespace
{
	// Entry of the ped death handler. The game's own kill counter sits behind five
	// checks that drop object kills, so barrel and refrigerator deaths are resolved
	// here instead, before any of them run.
	void __cdecl RecordPedDeathFromHook(void* ped)
	{
		AchievementHooks::RecordPedDeath(ped);
	}

	void __declspec(naked) HookPedDeathGateway()
	{
		static const uintptr_t continuation = 0x4ECD19;
		_asm
		{
			pushad
			push ecx
			call RecordPedDeathFromHook
			add esp, 4
			popad
			push ebx
			push ebp
			mov ebx, ecx
			mov ebp, 0x2000000
			jmp continuation
		}
	}
}

namespace
{
	// An explosion damages a hunter without any entity to blame, so the game
	// records no damage source and the death alone says nothing about its cause.
	// The shot does: its weapon is the exploding object itself. Victims caught
	// that way are remembered here until they die, usually the same instant.
	// healthBefore and maxNonHeadDamage answer a question the game never records
	// anywhere we can read: was the head hit. The weapon data says how many head
	// shots it takes to kill (one, for the sniper rifle) and how much a hit
	// elsewhere does - damage times the torso or arm multiplier. If the victim
	// had more health than that and died anyway, nothing but the head can have
	// been hit. Offsets 0xB8 and 0xBC come from the game's own type data parser
	// at 0x4426C8, which stores TORSO_DAMAGE_MULTIPLIER and ARM_DAMAGE_MULTIPLIER
	// there.
	struct DamageRecord { const void* victim; int gameTime; int shotClass; int weaponClass;
		bool hadSource; float healthBefore; float maxNonHeadDamage; };
	DamageRecord g_recentDamage[16] = {};
	int g_nextDamageRecord = 0;

	const DamageRecord* FindRecentDamage(const void* victim)
	{
		// Game clock, not the frame counter: that one cycles inside a small range
		// and runs backwards in this build, so it matched any record still in the
		// ring and a death could be paired with somebody else's blow.
		for (const DamageRecord& hit : g_recentDamage)
		{
			const int since = CGameTime::ms_currGameTime - hit.gameTime;
			if (hit.victim && hit.victim == victim && since >= 0 && since <= 500)
				return &hit;
		}
		return nullptr;
	}

	// One explosion can be lethal to several hunters at once, and the death
	// follows within a frame or two, so a few slots and a short window suffice.
	bool DiedToExplodingObject(const void* victim)
	{
		const DamageRecord* hit = FindRecentDamage(victim);
		return hit && hit->weaponClass == WC_EXPLODER;
	}

	void __cdecl RecordDamageFromHook(void* ped, void* shot, void* source)
	{
		AchievementHooks::RecordDamage(ped, shot, source);
	}

	void __declspec(naked) HookPedDamageGateway()
	{
		static const uintptr_t continuation = 0x4ECE77;
		_asm
		{
			pushad
			mov ebx, [esp+0x2C]
			mov edi, [esp+0x24]
			mov esi, [esp+0x18]
			push ebx
			push edi
			push esi
			call RecordDamageFromHook
			add esp, 12
			popad
			push ebx
			push esi
			push ebp
			mov eax, [esp+0x18]
			jmp continuation
		}
	}
}

namespace
{
	// The crane kills through the level script rather than through damage, so the
	// death carries no cause at all. What it does carry is a refrigerator lying
	// on top of the victim, which is the only thing left to recognise it by.
	float DistanceToNearestRefrigerator(CEntity* victim)
	{
		static const char* const names[] = { "CJ_FRIDGE", "CJ_FRIDGE01", "CJ_FRIDGE02" };
		const CVector* at = victim ? victim->GetLocation() : nullptr;
		if (!at)
			return -1.0f;
		float nearest = -1.0f;
		for (const char* name : names)
		{
			CEntity* fridge = CEntityManager::FindInstance(const_cast<char*>(name));
			const CVector* on = fridge ? fridge->GetLocation() : nullptr;
			if (!on)
				continue;
			const float dx = on->x - at->x, dy = on->y - at->y, dz = on->z - at->z;
			const float distance = sqrtf(dx * dx + dy * dy + dz * dz);
			if (nearest < 0.0f || distance < nearest)
				nearest = distance;
		}
		return nearest;
	}
}

namespace
{
	// Every spawned entity hangs off one list. The head sits here, each node
	// carries its entity first and the next node eight bytes on; this is the
	// same walk CEntityManager::FindInstance makes to look a name up.
	struct EntityNode { CEntity* entity; void* unused; EntityNode* next; };
	EntityNode** const kEntityList = reinterpret_cast<EntityNode**>(0x69BBE4);
	const int kEntityWalkLimit = 4096;

	// What the game asks of an object before it lets it kill: the body kind at
	// 0x104 and the weight behind it at 0x98, both read off the physics record
	// the crushing branch itself uses, which is not CEntity::m_pTypeData.
	const int kCrushingBodyKind = 0x26;
	const int kCrushingWeight = 0x222;

	const char* NameOf(const CEntity* entity)
	{
		return entity && entity->m_szName ? entity->m_szName : "<unnamed>";
	}

	const char* RecordOf(const CEntity* entity)
	{
		return entity && entity->m_pTypeData && entity->m_pTypeData->m_szRecordName
			? entity->m_pTypeData->m_szRecordName : "<none>";
	}

	// What was lying on the victim when it died. The crane kills through a path
	// that leaves no cause behind, so the surroundings are the only witness.
	void LogNeighbours(CEntity* victim)
	{
		if (!eLog::Detailed())
			return;
		CVector* at = victim ? victim->GetLocation() : nullptr;
		if (!at)
			return;
		struct Near { CEntity* entity; float distance; };
		Near nearest[10] = {};
		int found = 0, walked = 0;
		for (EntityNode* node = kEntityList ? *kEntityList : nullptr;
			node && walked < kEntityWalkLimit; node = node->next, ++walked)
		{
			CEntity* entity = node->entity;
			if (!entity || entity == victim)
				continue;
			CVector* on = entity->GetLocation();
			if (!on)
				continue;
			const float dx = on->x - at->x, dy = on->y - at->y, dz = on->z - at->z;
			const Near candidate = { entity, sqrtf(dx * dx + dy * dy + dz * dz) };
			if (found == 10 && candidate.distance >= nearest[9].distance)
				continue;
			int slot = found < 10 ? found++ : 9;
			nearest[slot] = candidate;
			while (slot > 0 && nearest[slot].distance < nearest[slot - 1].distance)
			{
				const Near swap = nearest[slot - 1];
				nearest[slot - 1] = nearest[slot];
				nearest[slot] = swap;
				--slot;
			}
		}
		for (int i = 0; i < found; ++i)
			eLog::Verbose(__FUNCTION__, "%s: %.2f away is %s (type %s)",
				NameOf(victim), nearest[i].distance, NameOf(nearest[i].entity),
				RecordOf(nearest[i].entity));
	}

	// The death handler runs more than once for the same body: a hunter was seen
	// dying twice, and one of them three times. Counting each of those would let
	// the Brawl Game reach its thirty deaths long before thirty hunters had died.
	//
	// The address alone does not identify a body. The Brawl Game recycles a small
	// pool of fighters, and the log caught the same address dying as Attacker4
	// and, a millisecond later, as Attacker6: the object is reused and renamed.
	// Matching on the address alone threw those away and the counter came up
	// short. The name has to agree as well, and the window stays narrow, since
	// repeats of one death arrive within a frame while a body cannot be recycled,
	// respawned and killed again inside half a second.
	// Measured off the logs: the second notification about one death arrives
	// within a tenth of a second, while a fighter recycled by the Brawl Game
	// script needs at least twelve to reappear and be killed again. Wall clock,
	// not the game's frame counter, which restarts with every scene: records made
	// before the restart then looked newer than the present and swallowed real
	// deaths, which is how the Brawl Game trophy came four kills late.
	const ULONGLONG kSameDeathMs = 250;
	struct DeathRecord { const void* victim; ULONGLONG tick; char name[24]; };
	DeathRecord g_recentDeaths[16] = {};
	int g_nextDeathRecord = 0;

	bool AlreadyCountedThisDeath(const CEntity* victim)
	{
		char name[24] = {};
		if (victim && victim->m_szName)
			strncpy_s(name, victim->m_szName, _TRUNCATE);
		const ULONGLONG now = GetTickCount64();
		for (const DeathRecord& past : g_recentDeaths)
			if (past.victim && past.victim == victim &&
				now - past.tick <= kSameDeathMs &&
				strcmp(past.name, name) == 0)
				return true;
		DeathRecord& record = g_recentDeaths[g_nextDeathRecord];
		record.victim = victim;
		record.tick = now;
		strncpy_s(record.name, name, _TRUNCATE);
		g_nextDeathRecord = (g_nextDeathRecord + 1) % 16;
		return false;
	}

	// Crushed hunters take no damage at all, the script kills them outright, so
	// the refrigerator lying on them is the only witness. Measured across both
	// kinds of death it sat between 1.7 and 3.4 from the ones it crushed and no
	// closer than 8.4 to any other, so six separates them with room to spare.
	// Set on both hunters the refrigerator killed and on none of the hundred odd
	// other deaths in the logs: shot, executed, beaten, or crushed by another
	// hunter. Distance alone never separated them - a hunter shot two metres from
	// a refrigerator came closer than one it had actually flattened - and taking
	// no damage does not either, because an execution leaves none.
	const unsigned int kCrushedState = 0x200;

	bool CrushedByRefrigerator(CEntity* victim, const void* lastBlow)
	{
		if (lastBlow || !victim)
			return false;
		const float distance = DistanceToNearestRefrigerator(victim);
		if (distance < 0.0f || distance > 6.0f)
			return false;
		const unsigned int state =
			*reinterpret_cast<unsigned int*>(reinterpret_cast<unsigned char*>(victim) + 0x338);
		if (state & kCrushedState)
			return true;
		// Everything but that one bit agreed. Logged so a refrigerator kill that
		// stops being recognised says so instead of quietly going missing.
		eLog::Verbose(__FUNCTION__,
			"%s died undamaged %.2f from a refrigerator but state 0x%08X is not a crush",
			NameOf(victim), distance, state);
		return false;
	}
}

namespace
{
	// Who crushed whom, taken while the game still knows. Same shape as the
	// damage ring: one falling object can catch several hunters at once.
	struct CrushRecord { const void* victim; ULONGLONG tick; bool refrigerator; };
	CrushRecord g_recentCrushes[8] = {};
	int g_nextCrushRecord = 0;

	bool CrushedByRefrigeratorEntity(const void* victim)
	{
		for (const CrushRecord& crush : g_recentCrushes)
			if (crush.victim && crush.victim == victim && crush.refrigerator &&
				GetTickCount64() - crush.tick <= 1000)
				return true;
		return false;
	}

	void __cdecl RecordCrushFromHook(void* crushingObject)
	{
		AchievementHooks::RecordCrush(crushingObject);
	}

	void __declspec(naked) HookCrushGateway()
	{
		static const uintptr_t continuation = 0x4B99E6;
		_asm
		{
			pushad
			mov esi, [esp+0x28]
			push esi
			call RecordCrushFromHook
			add esp, 4
			popad
			push ebx
			push ebp
			mov ebp, [esp+0x10]
			jmp continuation
		}
	}
}

void AchievementHooks::RecordCrush(void* crushingObject)
{
	auto* bytes = reinterpret_cast<unsigned char*>(crushingObject);
	if (!bytes)
		return;
	CEntity* victim = *reinterpret_cast<CEntity**>(bytes + 0x35C);
	const unsigned char* typeData = *reinterpret_cast<unsigned char**>(bytes + 0x2F8);
	CEntity* object = reinterpret_cast<CEntity*>(crushingObject);
	if (!victim || !typeData)
	{
		eLog::Verbose(__FUNCTION__, "%s hit nobody or carries no type data",
			NameOf(object));
		return;
	}
	// The same two tests the game makes before letting an object kill: only a
	// certain kind of body, and only when it carries enough weight behind it.
	const int kind = *reinterpret_cast<const int*>(typeData + 0x104);
	const int weight = *reinterpret_cast<const int*>(typeData + 0x98);
	if (kind != kCrushingBodyKind || weight < kCrushingWeight)
	{
		// Logged even so: if the refrigerator never gets past here, this line is
		// what says which of the two tests turned it away.
		eLog::Verbose(__FUNCTION__, "%s hit %s but does not crush: kind=0x%X weight=%d",
			NameOf(object), NameOf(victim), kind, weight);
		return;
	}
	const bool refrigerator = EntityMatches(object, "fridge");
	g_recentCrushes[g_nextCrushRecord] = { victim, GetTickCount64(), refrigerator };
	g_nextCrushRecord = (g_nextCrushRecord + 1) % 8;
	eLog::Message(__FUNCTION__, "%s crushed by %s (fridge=%d)",
		victim->m_szName ? victim->m_szName : "<unnamed>",
		object->m_szName ? object->m_szName : "<unnamed>", refrigerator ? 1 : 0);
}

void AchievementHooks::RecordDamage(void* ped, void* shot, void* source)
{
	const CShot* firedShot = reinterpret_cast<const CShot*>(shot);
	if (!ped || !firedShot)
		return;
	const CEntity* hurt = reinterpret_cast<const CEntity*>(ped);
	if (!hurt->m_pTypeData ||
		(hurt->m_pTypeData->m_ecEntityClass & EC_HUNTER) != EC_HUNTER)
		return;
	// Remembered for every hunter, not only for explosions: the death itself
	// carries no cause, so the last blow it took is the only thing that names one.
	float maxNonHeadDamage = -1.0f;
	if (firedShot->m_TypeData && firedShot->m_pWeapon && firedShot->m_pWeapon->m_TypeData)
	{
		const float damage = firedShot->m_TypeData->m_fDamage;
		const auto* weaponType =
			reinterpret_cast<const unsigned char*>(firedShot->m_pWeapon->m_TypeData);
		const float torso = *reinterpret_cast<const float*>(weaponType + 0xB8);
		const float arm = *reinterpret_cast<const float*>(weaponType + 0xBC);
		const float multiplier = torso > arm ? torso : arm;
		// Refuse absurd numbers rather than trust the read: a wrong pointer must
		// cost the achievement, never hand it out.
		if (damage > 0.0f && damage < 10000.0f && multiplier > 0.0f && multiplier <= 10.0f)
			maxNonHeadDamage = damage * multiplier;
	}
	g_recentDamage[g_nextDamageRecord] = { ped, CGameTime::ms_currGameTime,
		firedShot->m_TypeData ? firedShot->m_TypeData->m_nShotClass : -1,
		firedShot->m_pWeapon && firedShot->m_pWeapon->m_TypeData
			? firedShot->m_pWeapon->m_TypeData->m_eWeaponClass : -1,
		source != nullptr, hurt->m_fHealth, maxNonHeadDamage };
	g_nextDamageRecord = (g_nextDamageRecord + 1) % 16;
	eLog::Verbose(__FUNCTION__,
		"hit on %s: health now %.1f, shot class %d, shot damage %.1f, most a non head hit could do %.1f",
		hurt->m_szName ? hurt->m_szName : "<unnamed>", hurt->m_fHealth,
		firedShot->m_TypeData ? firedShot->m_TypeData->m_nShotClass : -1,
		firedShot->m_TypeData ? firedShot->m_TypeData->m_fDamage : -1.0f, maxNonHeadDamage);
}

void AchievementHooks::RecordPedDeath(void* ped)
{
	CEntity* victim = reinterpret_cast<CEntity*>(ped);
	if (!victim)
		return;
	// The same body is announced dead more than once; only the first counts.
	if (AlreadyCountedThisDeath(victim))
	{
		// Logged, because a death dropped here is invisible everywhere else: the
		// Brawl Game counter once stalled on exactly this and said nothing.
		eLog::Verbose(__FUNCTION__, "repeat notification for %s ignored",
			victim->m_szName ? victim->m_szName : "<unnamed>");
		return;
	}
	CEntity* damageSource =
		*reinterpret_cast<CEntity**>(reinterpret_cast<unsigned char*>(victim) + 0x768);
	const int victimClass = victim->m_pTypeData ? victim->m_pTypeData->m_ecEntityClass : 0;
	const bool explodingBarrel = EntityMatches(damageSource, "explodingBarrel") ||
		DiedToExplodingObject(victim);
	const bool refrigerator = EntityMatches(damageSource, "fridge") ||
		CrushedByRefrigeratorEntity(victim) ||
		CrushedByRefrigerator(victim, FindRecentDamage(victim));

	const DamageRecord* lastBlow = FindRecentDamage(victim);
	eLog::Message(__FUNCTION__,
		"death: victim=%s class=0x%X; source=%s (type %s) class=0x%X; barrel=%d fridge=%d; "
		"state=0x%08X; last blow: shot class=%d weapon class=%d had source=%d; nearest fridge=%.2f; "
		"clocks: game=%d frame=%d tick=%llu",
		victim->m_szName ? victim->m_szName : "<none>", victimClass,
		damageSource && damageSource->m_szName ? damageSource->m_szName : "<none>",
		damageSource && damageSource->m_pTypeData && damageSource->m_pTypeData->m_szRecordName
			? damageSource->m_pTypeData->m_szRecordName : "<none>",
		damageSource && damageSource->m_pTypeData ? damageSource->m_pTypeData->m_ecEntityClass : 0,
		explodingBarrel ? 1 : 0, refrigerator ? 1 : 0,
		*reinterpret_cast<unsigned int*>(reinterpret_cast<unsigned char*>(victim) + 0x338),
		lastBlow ? lastBlow->shotClass : -1, lastBlow ? lastBlow->weaponClass : -1,
		lastBlow && lastBlow->hadSource ? 1 : 0,
		DistanceToNearestRefrigerator(victim),
		CGameTime::ms_currGameTime, CGameTime::ms_currFrame, GetTickCount64());
	LogNeighbours(victim);

	// Only hunters count; the player's own death runs through here as well.
	if ((victimClass & EC_HUNTER) != EC_HUNTER)
		return;
	eAchievements::OnHunterDied();
	if (explodingBarrel || refrigerator)
		eAchievements::OnHunterKilledByObject(explodingBarrel, refrigerator);
}

void AchievementHooks::RecordHunterKill(void* hunter)
{
	CEntity* victim = reinterpret_cast<CEntity*>(hunter);
	CEntity* damageSource = victim
		? *reinterpret_cast<CEntity**>(reinterpret_cast<unsigned char*>(victim) + 0x768)
		: nullptr;
	const bool explodingBarrel = EntityMatches(damageSource, "explodingBarrel") ||
		DiedToExplodingObject(victim);
	const bool refrigerator = EntityMatches(damageSource, "fridge") ||
		CrushedByRefrigeratorEntity(victim) ||
		CrushedByRefrigerator(victim, FindRecentDamage(victim));

	// A hit that could not have killed anywhere but the head, on a victim who
	// then died, is a head shot. Nothing weaker is asserted: an unreadable or
	// implausible reading leaves it false.
	const DamageRecord* fatalBlow = FindRecentDamage(victim);
	const bool headShot = fatalBlow && fatalBlow->maxNonHeadDamage > 0.0f &&
		fatalBlow->healthBefore > fatalBlow->maxNonHeadDamage;

	eLog::Message(__FUNCTION__,
		"victim=%s; damage source=%s (type %s); barrel=%d fridge=%d; item=%d; head shot=%d "
		"(health at the blow %.1f, most a non head hit could do %.1f)",
		victim && victim->m_szName ? victim->m_szName : "<none>",
		damageSource && damageSource->m_szName ? damageSource->m_szName : "<none>",
		damageSource && damageSource->m_pTypeData && damageSource->m_pTypeData->m_szRecordName
			? damageSource->m_pTypeData->m_szRecordName : "<none>",
		explodingBarrel ? 1 : 0, refrigerator ? 1 : 0,
		CGameInventory::GetCurrentItem(), headShot ? 1 : 0,
		fatalBlow ? fatalBlow->healthBefore : -1.0f,
		fatalBlow ? fatalBlow->maxNonHeadDamage : -1.0f);

	eAchievements::OnHunterKilled(CGameInventory::GetCurrentItem(), headShot,
		explodingBarrel, refrigerator);
}

namespace
{
    int StreamFileSize(const char* filename)
    {
        // PluginMH supplied actual sizes instead of the game's TOC/default 12 MiB.
        // Retain this prerequisite for enlarged frontend TXDs after extraction.
        const int nativeSize = CallAndReturn<int, 0x4D61B0, const char*>(filename);
        WIN32_FILE_ATTRIBUTE_DATA attributes{};
        if (filename && GetFileAttributesExA(filename, GetFileExInfoStandard, &attributes) &&
            !(attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            attributes.nFileSizeHigh == 0 && attributes.nFileSizeLow > 0 &&
            attributes.nFileSizeLow <= 0x7FFFFFBF)
        {
            const int actualSize = static_cast<int>(attributes.nFileSizeLow);
            const int previousLimit = nativeSize > 0 ? nativeSize : 0xC00000;
            if (actualSize > previousLimit)
                eLog::Message(__FUNCTION__, "%s: actual=%d bytes, original read limit=%d bytes",
                    filename, actualSize, previousLimit);
            return actualSize;
        }
        return nativeSize;
    }

    void OnShutdown()
    {
        eAchievements::Shutdown();
        reinterpret_cast<void(__cdecl*)()>(originalShutdown)();
    }

    void __fastcall OnPlayerDeath(void* player, void*)
    {
        eAchievements::OnPlayerDeath();
        reinterpret_cast<void(__thiscall*)(void*)>(originalDeath)(player);
    }

    void OnSceneStart()
    {
        reinterpret_cast<void(__cdecl*)()>(originalSceneStart)();
        eAchievements::OnSceneStart(CGameInfo::GetCurrentLevel(), CGameInfo::GetDifficulty());
    }

    void OnSceneReset()
    {
        reinterpret_cast<void(__cdecl*)()>(originalSceneReset)();
        eAchievements::OnSceneStart(CGameInfo::GetCurrentLevel(), CGameInfo::GetDifficulty());
    }

    // Everything the plugin does once the frontend has drawn a frame. Reached
    // either from our own chain on the render call or, when that call already
    // belongs to another plugin, from the trampoline just past it.
    void AfterRender()
    {
        // Asked every frame, not only at kills: see eAchievements::PollCheats.
        eAchievements::PollCheats();
        if (eAchievements::m_bWantsToPlayUnlock)
            eAchievements::PlaySlider();
        // Menus and the pause screen are the moment to put anything unwritten on
        // disk: nothing is happening that a flush could interrupt.
        if (!CFrontend::m_gameIsRunning)
            eAchievements::FlushProfile();
    }

    // Installed at the instruction after the frontend render call when that call
    // is not ours to take. Preserves everything, does the post-render work, then
    // replays the compare the jump overwrote so its flags reach the game's own
    // conditional jump.
    void __declspec(naked) HookRenderTailGateway()
    {
        static const uintptr_t continuation = 0x5F18AB;
        _asm
        {
            pushfd
            pushad
            call AfterRender
            popad
            popfd
            cmp dword ptr ds:0x7D3578, 0
            jmp continuation
        }
    }

    void OnRender()
    {
        static bool first = true;
        const bool trace = first;
        first = false;
        if (trace) eLog::Message(__FUNCTION__, "First frontend render: entering game renderer");
        reinterpret_cast<void(__cdecl*)()>(originalRender)();
        if (trace) eLog::Message(__FUNCTION__, "First frontend render: game renderer returned");
        AfterRender();
    }

    void OnExecution()
    {
        auto* player = reinterpret_cast<CPlayer*>(CScene::FindPlayer());
        eAchievements::OnExecution(CGameInventory::GetCurrentItem(),
            player ? player->GetExecuteStage() : 0);
        reinterpret_cast<void(__cdecl*)()>(originalExecution)();
    }


    void OnPainkiller(int value)
    {
        eAchievements::OnPainkillerUsed();
        reinterpret_cast<void(__cdecl*)(int)>(originalPainkiller)(value);
    }
}

bool AchievementHooks::Validate()
{
    if (reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)) != 0x400000)
    {
        eLog::Message(__FUNCTION__, "Unsupported executable image base");
        return false;
    }
    for (const HookSite& site : kHookSites)
    {
        const auto* address = reinterpret_cast<const unsigned char*>(site.address);
        MEMORY_BASIC_INFORMATION region = {};
        if (!VirtualQuery(address, &region, sizeof(region)) ||
            region.State != MEM_COMMIT ||
            (region.Protect & (PAGE_GUARD | PAGE_NOACCESS)) ||
            site.address + site.size > reinterpret_cast<uintptr_t>(region.BaseAddress) + region.RegionSize ||
            (memcmp(address, site.expected, site.size) != 0 && !PluginCompatibility::AcceptPatch(site)))
        {
            eLog::Message(__FUNCTION__,
                "Unsupported or already patched site: 0x%08X (%s); expected %02X %02X %02X %02X %02X %02X %02X; "
                "found %02X %02X %02X %02X %02X %02X %02X; state=%lu protect=0x%lX",
                site.address, site.name,
                site.expected[0], site.expected[1], site.expected[2], site.expected[3],
                site.expected[4], site.expected[5], site.expected[6],
                address[0], address[1], address[2], address[3],
                address[4], address[5], address[6],
                region.State, region.Protect);
            return false;
        }
    }
    return true;
}

void AchievementHooks::Install()
{
    originalShutdown=PluginCompatibility::CallTarget(0x4D7F70);
    originalDeath=*reinterpret_cast<uintptr_t*>(0x7113FC);
    originalSceneStart=PluginCompatibility::CallTarget(0x474A02);
    originalSceneReset=PluginCompatibility::CallTarget(0x473F53);
    originalRender=PluginCompatibility::CallTarget(0x5F189F);
    originalExecution=PluginCompatibility::CallTarget(0x4811F6);
    originalKill=PluginCompatibility::CallTarget(0x4ECDE3);
    originalPainkiller=PluginCompatibility::CallTarget(0x45E688);
    InjectHook(0x4D50D4, StreamFileSize, PATCH_CALL);
    InjectHook(0x4D7F70, OnShutdown, PATCH_CALL);
    Patch<uintptr_t>(0x7113FC, reinterpret_cast<uintptr_t>(&OnPlayerDeath));
    InjectHook(0x474A02, OnSceneStart, PATCH_CALL);
    InjectHook(0x473F53, OnSceneReset, PATCH_CALL);
    // The frontend render call may already belong to another plugin. A foreign
    // callee is not ours to chain, so nothing is installed over it; the unlock
    // banner moves to the instruction right after the call, which is where our
    // post-render work ran anyway.
    if (!PluginCompatibility::ForeignSite(0x5F189F))
        InjectHook(0x5F189F, OnRender, PATCH_CALL);
    else if (memcmp(reinterpret_cast<const void*>(kRenderTailSite.address),
            kRenderTailSite.expected, kRenderTailSite.size) == 0)
    {
        InjectHook(kRenderTailSite.address, HookRenderTailGateway, PATCH_JUMP);
        eLog::Message(__FUNCTION__,
            "Skipping the conflicting hook on the frontend render call 0x5F189F: it belongs to "
            "another plugin. The unlock banner runs from 0x5F18A4 instead");
    }
    else
        eLog::Message(__FUNCTION__,
            "Skipping the conflicting hook on the frontend render call 0x5F189F: it belongs to "
            "another plugin, and 0x5F18A4 is taken as well. Achievements still unlock and are "
            "saved, but the unlock banner stays hidden");
    InjectHook(0x4811F6, OnExecution, PATCH_CALL);
    InjectHook(0x4ECDE3, HookKillsStatGateway, PATCH_CALL);
    // Only when nobody else holds it: our trampoline replays the game's own
    // prologue, so it must not be installed over a foreign detour.
    if (memcmp(reinterpret_cast<const void*>(kCrushSite.address),
            kCrushSite.expected, kCrushSite.size) == 0)
        InjectHook(0x4B99E0, HookCrushGateway, PATCH_JUMP);
    else
        eLog::Message(__FUNCTION__,
            "Crushing is already hooked; kills by falling objects fall back to proximity");
    if (memcmp(reinterpret_cast<const void*>(kExplosiveDamageSite.address),
            kExplosiveDamageSite.expected, kExplosiveDamageSite.size) == 0)
        InjectHook(0x4ECE70, HookPedDamageGateway, PATCH_JUMP);
    else
        eLog::Message(__FUNCTION__,
            "Damage recording is already hooked; kills by exploding objects stay unavailable");
    if (memcmp(reinterpret_cast<const void*>(kPedDeathSite.address),
            kPedDeathSite.expected, kPedDeathSite.size) == 0)
        InjectHook(0x4ECD10, HookPedDeathGateway, PATCH_JUMP);
    else
        eLog::Message(__FUNCTION__,
            "Ped death entry is already hooked; barrel and refrigerator kills stay unavailable");
    InjectHook(0x45E688, OnPainkiller, PATCH_CALL);
}
