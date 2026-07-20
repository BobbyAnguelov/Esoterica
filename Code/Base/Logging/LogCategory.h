#pragma once
#include "Base/_Module/API.h"
#include <stdint.h>

//-------------------------------------------------------------------------

namespace EE { class StringID; }

//-------------------------------------------------------------------------

namespace EE
{
    enum class LogCategory : int8_t
    {
        Invalid = -1,

        Animation = 0,
        Entity,
        FileSystem,
        Gameplay,
        Input,
        Math,
        Memory,
        Navmesh,
        Network,
        Physics,
        Platform,
        Render,
        Resource,
        Serialization,
        System,
        Threading,
        Tools,
        TypeSystem,
    };

    //-------------------------------------------------------------------------

    EE_BASE_API char const* GetLogCategoryString( LogCategory category );
    EE_BASE_API StringID GetLogCategoryID( LogCategory category );
    inline uint8_t GetNumLogCategories() { return (uint8_t) LogCategory::TypeSystem; }
}