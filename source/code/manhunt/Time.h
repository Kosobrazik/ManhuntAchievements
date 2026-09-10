#pragma once

class CGameTime {
public:
	static int& ms_startRealTime;
	static int& ms_realTimeOffset;
	static int& ms_currGameTime;
	// Despite the inherited name this is not paused time: it advances by the
	// same step as ms_currGameTime under the same condition and is reset with
	// it at scene start, so it reads as elapsed time within the scene.
	static int& ms_currGameTimePaused;
	static int& ms_timeStep;
	// Not a frame counter in the community patched executable: measured across a
	// dozen kills it read 33, then 18, then 126, then 19 - it cycles inside a
	// small range and runs backwards. Nothing here relies on it any more;
	// ms_currGameTime tracks the wall clock to within a couple of milliseconds.
	static int& ms_currFrame;
	static void Update();
};