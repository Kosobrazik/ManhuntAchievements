#include "GameInfo.h"
#include "core.h"

int CGameInfo::GetCurrentLevel()
{
	return *(int*)0x75622C;
}

int CGameInfo::GetDifficulty()
{
	return CallAndReturn<int, 0x5D94D0>();
}

int CGameInfo::GetLevelStars(int level)
{
	// The level's best rating ever earned. The game keeps one per level and does
	// not separate difficulties, so this is not the result of the current run.
	return CallAndReturn<int, 0x5D1E80, int>(level);
}

// Points scored over the level's maximum give a rating through a per-difficulty
// table, plus one star for beating the level's par time. Fetish tops out at 3+1
// and Hardcore at 4+1, which is why five stars need Hardcore. The game runs this
// itself moments later, so calling it early only recomputes the same value.
int CGameInfo::ComputeRunRating(int levelTime)
{
	return CallAndReturn<int, 0x5D2140, int>(levelTime);
}

bool CGameInfo::IsMainScene(int level)
{
	return level >= 0 && level < 20;
}
