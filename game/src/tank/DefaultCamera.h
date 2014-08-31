// DefaultCamera.h

#pragma once

#include <math/MyMath.h>

namespace UI
{
	struct IInput;
}

class DefaultCamera
{
public:
	DefaultCamera();

	void HandleMovement(UI::IInput &input, float worldWidth, float worldHeight, float screenWidth, float screenHeight);
	float GetZoom() const { return _zoom; }
	float GetPosX() const { return _pos.x; }
	float GetPosY() const { return _pos.y; }

private:
	float _zoom;
	float _dt;
	vec2d _pos;
	unsigned int _dwTimeX;
	unsigned int _dwTimeY;
};

// end of file
