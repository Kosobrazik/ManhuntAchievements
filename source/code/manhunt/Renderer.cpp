#include "Renderer.h"
#include "core.h"


void CRenderer::DrawQuad2d(float posX, float posY, float scaleX, float scaleY, int red, int green, int blue, int alpha, int pTexture)
{
	Call<0x5F96F0, float, float, float, float, int, int, int, int, int>(posX, posY, scaleX, scaleY, red, green, blue, alpha, pTexture);
}
