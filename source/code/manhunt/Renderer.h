#pragma once
#include <rwcore.h>
#include <rpworld.h>

class CRenderer {
public:
	static void DrawQuad2d(float posX, float posY, float scaleX, float scaleY, int red, int green, int blue, int alpha, int pTexture);
};