#include "Scene.h"
#include "core.h"
#include "Anim.h"

bool& CScene::ms_bFreeCam = *(bool*)0x715BB0;
int& CScene::ms_stepMode = *(int*)0x715BA0;
CEntity*& CScene::ms_pPlayer = *(CEntity**)0x715B9C;
CEntity*& CScene::ms_pCamera = *(CEntity**)0x715B94;
int& CScene::ms_pWorld= *(int*)0x715B8C;
CEntity * CScene::FindPlayer()
{
	return *(CEntity**)0x715B9C;
}
