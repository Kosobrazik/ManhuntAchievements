#include "Player.h"
#include "core.h"

int CPlayer::GetExecuteStage()
{
	return CallMethodAndReturn<int, 0x46CB80, CPlayer*>(this);
}
