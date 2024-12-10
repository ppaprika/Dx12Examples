#include "Camera.h"


XMMATRIX Camera::GetViewMatrix()
{

}

XMMATRIX Camera::GetProjectionMatrix()
{

}

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

void Camera::Reset()
{

}
