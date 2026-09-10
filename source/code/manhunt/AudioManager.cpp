#include "AudioManager.h"
#include "core.h"

cAudioManager& AudioManager = *(cAudioManager*)0x6B53D8;
cDMAudio& DMAudio = *(cDMAudio*)0x6C5244;

void cDMAudio::PlayFrontEndSound(short sample, float unk)
{
	CallMethod<0x456280,cDMAudio*, short, float>(this, sample, unk);
}
