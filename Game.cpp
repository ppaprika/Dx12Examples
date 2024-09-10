#include "Game.h"

#include <DirectXMath.h>

#include "Helpers.h"
#include "Application.h"
#include "Window.h"

Game::Game(const std::wstring& name, int width, int height, bool vSync)
{
    m_Name = name;
    m_Width = width;
    m_Height = height;
    m_vSync = vSync;
}

bool Game::Initialize()
{
    if(!DirectX::XMVerifyCPUSupport())
    {
        MessageBoxA(nullptr, "Failed to verify DirectX math library support.", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    Application& app = Application::Get();
    m_pWindow = app.CreateRenderWindow(m_Name, m_Width, m_Height, m_vSync);
    m_pWindow->RegisterCallbacks(shared_from_this());
    m_pWindow->Show();

    return true;
}


