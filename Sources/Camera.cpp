#include "Camera.h"


void Camera::OnKeyDown(WPARAM key)
{
    switch (key)
    {
    case 'W':
        keys_pressed_.w = true;
        break;
    case 'A':
        keys_pressed_.a = true;
        break;
    case 'S':
        keys_pressed_.s = true;
        break;
    case 'D':
        keys_pressed_.d = true;
        break;
    case VK_LEFT:
        keys_pressed_.left = true;
        break;
    case VK_RIGHT:
        keys_pressed_.right = true;
        break;
    case VK_UP:
        keys_pressed_.up = true;
        break;
    case VK_DOWN:
        keys_pressed_.down = true;
        break;
    case VK_ESCAPE:
        Reset();
        break;
    }
}

void Camera::OnKeyUp(WPARAM key)
{
    switch (key)
    {
    case 'W':
        keys_pressed_.w = false;
        break;
    case 'A':
        keys_pressed_.a = false;
        break;
    case 'S':
        keys_pressed_.s = false;
        break;
    case 'D':
        keys_pressed_.d = false;
        break;
    case VK_LEFT:
        keys_pressed_.left = false;
        break;
    case VK_RIGHT:
        keys_pressed_.right = false;
        break;
    case VK_UP:
        keys_pressed_.up = false;
        break;
    case VK_DOWN:
        keys_pressed_.down = false;
        break;
    }
}

void Camera::Update(float delta, float aspectRatio)
{
    aspect_ratio_ = aspectRatio;

    XMVECTOR rightVector = XMVector3Cross(camera_up_vector_, camera_forward_vector_);

    if (keys_pressed_.w)
        camera_position_ += camera_forward_vector_ * (camera_move_speed_ * delta);
    if(keys_pressed_.s)
        camera_position_ -= camera_forward_vector_ * (camera_move_speed_ * delta);
    if (keys_pressed_.a)
        camera_position_ -= rightVector * (camera_move_speed_ * delta);
    if(keys_pressed_.d)
        camera_position_ += rightVector * (camera_move_speed_ * delta);

    if (keys_pressed_.up)
        camera_pitch_ += camera_turn_speed_ * delta;
    if (keys_pressed_.down)
        camera_pitch_ -= camera_turn_speed_ * delta;
    if (keys_pressed_.right)
        camera_yaw_ += camera_turn_speed_ * delta;
    if(keys_pressed_.left)
        camera_yaw_ -= camera_turn_speed_ * delta;

    // clamp pitch
    camera_pitch_ = min(camera_pitch_, XM_PIDIV4);
    camera_pitch_ = max(-XM_PIDIV4, camera_pitch_);

    XMMATRIX rotateMatrix = XMMatrixRotationRollPitchYaw(camera_pitch_, camera_yaw_, 0);
    camera_forward_vector_ = XMVector3Transform(camera_init_forward_vector, rotateMatrix);
    camera_forward_vector_ = XMVector3Normalize(camera_forward_vector_);
}

XMMATRIX Camera::GetViewMatrix() const
{
    return XMMatrixLookToLH(camera_position_, camera_forward_vector_, camera_up_vector_);
}

XMMATRIX Camera::GetProjectionMatrix() const
{
    return XMMatrixPerspectiveFovLH(XMConvertToRadians(fov_), aspect_ratio_, 0.1, 100);
}

void Camera::Reset()
{

}
