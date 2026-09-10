#pragma once
#include "Collectable.h"
#include "SampleIDs.h"

class cAudioManager {
public:
};


class cDMAudio {
public:
	void PlayFrontEndSound(short sample, float unk);
};

extern cAudioManager& AudioManager;

extern cDMAudio& DMAudio;