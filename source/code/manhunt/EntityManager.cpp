#include "EntityManager.h"
#include "core.h"


int& CEntityManager::ms_playerCharacterID = *(int*)0x6A94C0;
bool& CEntityManager::ms_disableHunters = *(bool*)0x6A94C8;

CEntity * CEntityManager::FindInstance(char * name)
{
	return CallAndReturn<CEntity*, 0x437CA0, const char*>(name);
}
