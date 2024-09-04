#include "Game.h"
#include "Helpers.h"

Game::Game(const std::wstring& name, int width, int height, bool vSync)
{
    m_Name = name;
    m_Width = width;
    m_Height = height;
    m_vSync = vSync;
}

bool Game::Initialize()
{
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    return false;
}
