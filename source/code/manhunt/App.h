#pragma once
#include "core.h"

enum eLevels {
	jury_turf, // Born Again
	derelict, // Doorway Into Hell
	der2, // Road To Ruin
	scrap, // White Trash
	scrap2, // Fuelled By Hate
	zoo, // Grounds For Assault
	zoo2, // Strapped for Cash
	mall, // View of Innocence
	church2, // Drunk Driving
	pharm_wks, // Graveyard Shift
	asylum, // Mouth of Madness
	cellblock, // Doing Time
	prison, // Kill The Rabbit
	ramirez, // Divided They Fall
	journo_streets, // Press Coverage
	subway, // Wrong Side of the Tracks
	trainyard, // Trained To Kill
	estate_ext, // Border Patrol
	mansion_int, // Key Personnel
	attic, // Deliverance
	// Order below matches initscripts/LEVELS/levels.txt, where the game lists
	// Bonus3 before Bonus2. Declaring them in name order swapped the two bonus
	// level achievements.
	bonus1, // 20 Hard as Nails
	bonus3, // 21 Brawl Game
	bonus2, // 22 Monkey See, Monkey Die
	weasel, // 23 Time 2 Die
};



class CApp {
public:
	static HWND& ms_window;
	static int&  ms_currLevelNum;
	static int&  ms_mode;

	static void ShutdownLevel();
	static void StartupLevel();
	static void SetLevel(int level);
	static void SetLevel(char* folder);
};