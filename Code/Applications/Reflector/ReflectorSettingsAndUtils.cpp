#include "ReflectorSettingsAndUtils.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Reflection
{
    static char const* g_macroNames[] =
    {
        "EE_REFLECT_MODULE",
        "EE_REFLECT_ENUM",
        "EE_REFLECT_TYPE",
        "EE_REFLECT",
        "EE_RESOURCE",
        "EE_DATA_FILE",
        "EE_ENTITY_COMPONENT",
        "EE_SINGLETON_ENTITY_COMPONENT",
        "EE_ENTITY_SYSTEM",
        "EE_ENTITY_WORLD_SYSTEM"
    };

    //-------------------------------------------------------------------------

    char const* GetReflectionMacroText( ReflectionMacroType macro )
    {
        uint32_t const macroIdx = (uint32_t) macro;
        EE_ASSERT( macroIdx < (uint32_t) ReflectionMacroType::NumMacros );
        return g_macroNames[macroIdx];
    }
}