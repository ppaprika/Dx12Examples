#pragma once
#include <DirectXMath.h>

#include "Application.h"


using namespace DirectX;

class Camera
{
public:
	XMMATRIX GetViewMatrix();
	XMMATRIX GetProjectionMatrix();

    void OnKeyDown(WPARAM key);
    void OnKeyUp(WPARAM key);

private:
    void Reset();

    struct KeysPressed
    {
        bool w;
        bool a;
        bool s;
        bool d;

        bool left;
        bool right;
        bool up;
        bool down;
    };

	XMFLOAT3 camera_position_ = {0, 0, -10};
	XMFLOAT3 camera_up_vector_ = {0, 1, 0};
	XMFLOAT3 camera_forward_vector_ = {0, 0, 1};

    // speed per sec
	float camera_move_speed_ = 5;
    float camera_turn_speed_ = 1; // radians

    KeysPressed keys_pressed_ = {};
};

