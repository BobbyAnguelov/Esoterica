#pragma once

#include "Base/Types/String.h"
#include "Base/Time/Time.h"
#include "Base/Time/Timers.h"
#include "ImguiStyle.h"

//-------------------------------------------------------------------------
// Toast Notifications
//-------------------------------------------------------------------------

namespace EE::ImGuiX
{
    struct NotificationSystem
    {
        static void Initialize();
        static void Shutdown();
        static void Render();
    };
}