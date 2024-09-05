#include "Game.h"
#include "Helpers.h"
#include "Application.h"

Game::Game(const std::wstring& name, int width, int height, bool vSync)
{
    m_Name = name;
    m_Width = width;
    m_Height = height;
    m_vSync = vSync;
}

bool Game::Initialize()
{
    Application& app = Application::Get();
    std::shared_ptr<Window> wPtr = app.CreateRenderWindow(m_Name, m_Width, m_Height, m_vSync);



    return false;
}
