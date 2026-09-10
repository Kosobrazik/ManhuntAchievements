#pragma once
#include"Entity.h"

class CEntityManager {
public:
	static int& ms_playerCharacterID;
	static bool& ms_disableHunters;
	static CEntity* FindInstance(char* name);



};