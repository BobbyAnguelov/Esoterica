#pragma once

//-------------------------------------------------------------------------

namespace EE
{
    enum class LoadingStatus
    {
        Unloaded = 0,
        Loading,
        Loaded,
        Unloading,
        Failed,
    };
}