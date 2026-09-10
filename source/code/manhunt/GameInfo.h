#pragma once

enum eGameDifficulty
{
	DIFFICULTY_FETISH = 0,
	DIFFICULTY_HARDCORE = 1
};

class CGameInfo
{
public:
	static int GetCurrentLevel();
	static int GetDifficulty();
	static int GetLevelStars(int level);
	// Rating the finished run itself earns, before the game keeps only the best.
	static int ComputeRunRating(int levelTime);
	static bool IsMainScene(int level);
};
