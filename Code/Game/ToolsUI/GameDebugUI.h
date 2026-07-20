#pragma once
#include "Game/_Module/API.h"
#include "Engine/ToolsUI/EngineDebugUI.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    class EE_GAME_API GameDebugUI final : public EngineDebugUI
    {

    public:

        using EngineDebugUI::EngineDebugUI;
    };
}
#endif