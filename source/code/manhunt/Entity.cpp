#include "Entity.h"
#include "core.h"



void CEntity::Spawn(CVector * pos, float arg)
{
	CallMethod<0x4342E0, int,CVector*, float>((int)this, pos, arg);
}

void CEntity::Spawn(RwMatrix * matrix)
{
	CallMethod<0x4345C0, CEntity*, RwMatrix*>(this, matrix);
}

void CEntity::Destroy()
{
	CallMethod<0x4311C0, CEntity*>(this);
}

void CEntity::Kill()
{
	CallMethod<0x4313E0, CEntity*>(this);
}

CVector * CEntity::GetLocation()
{
	return CallMethodAndReturn<CVector*,0x4317E0, CEntity*>(this);
}
