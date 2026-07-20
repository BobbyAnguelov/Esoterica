#include "LogCategory.h"
#include "Base/Types/StringID.h"

//-------------------------------------------------------------------------

namespace EE
{
    inline constexpr char const * const g_logCategoryStrings[] =
    {
        "Animation",
        "Entity",
        "FileSystem",
        "Gameplay",
        "Input",
        "Math",
        "Memory",
        "Navmesh",
        "Network",
        "Physics",
        "Platform",
        "Rendering",
        "Resource",
        "Serialization",
        "System",
        "Threading",
        "Tools",
        "TypeSystem",
    };

    static const StaticStringID g_categoryIDs[] =
    {
        StaticStringID( "Animation" ),
        StaticStringID( "Entity" ),
        StaticStringID( "FileSystem" ),
        StaticStringID( "Gameplay" ),
        StaticStringID( "Input" ),
        StaticStringID( "Math" ),
        StaticStringID( "Memory" ),
        StaticStringID( "Navmesh" ),
        StaticStringID( "Network" ),
        StaticStringID( "Physics" ),
        StaticStringID( "Platform" ),
        StaticStringID( "Rendering" ),
        StaticStringID( "Resource" ),
        StaticStringID( "Serialization" ),
        StaticStringID( "System" ),
        StaticStringID( "Threading" ),
        StaticStringID( "Tools" ),
        StaticStringID( "TypeSystem" ),
    };

    char const* GetLogCategoryString( LogCategory category )
    {
        EE_ASSERT( category != LogCategory::Invalid );
        return g_logCategoryStrings[ (int) category ];
    }

    StringID GetLogCategoryID( LogCategory category )
    {
        EE_ASSERT( category != LogCategory::Invalid );
        return g_categoryIDs[(int) category];
    }
}