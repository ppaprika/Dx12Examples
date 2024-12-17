#pragma once
#include <DirectXMath.h>

#include "Application.h"

using DirectX::XMMATRIX;
using DirectX::XMVECTOR;

class Camera
{
public:
	XMMATRIX GetViewMatrix() const;
	XMMATRIX GetProjectionMatrix() const;

    void OnKeyDown(WPARAM key);
    void OnKeyUp(WPARAM key);

    void Update(float delta, float aspectRatio);

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

	XMVECTOR camera_position_ = {0, 0, -10};
    XMVECTOR camera_up_vector_ = {0, 1, 0};
    XMVECTOR camera_init_forward_vector = { 0, 0, 1 };
    XMVECTOR camera_forward_vector_ = {0, 0, 1};

    float camera_pitch_ = 0;
    float camera_yaw_ = 0;

    float fov_ = 60;
    float aspect_ratio_ = 0;

    // speed per sec
	float camera_move_speed_ = 5;
    float camera_turn_speed_ = 1; // radians

    KeysPressed keys_pressed_ = {};
};

