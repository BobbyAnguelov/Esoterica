#include "ReflectorSettingsAndUtils.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Reflection
{
    static char const* g_macroNames[] =
    {
        "EE_REFLECTION_IGNORE_HEADER",
        "EE_REGISTER_MODULE",
        "EE_REGISTER_ENUM",
        "EE_REGISTER_TYPE",
        "EE_REGISTER_RESOURCE",
        "EE_REGISTER_TYPE_RESOURCE",
        "EE_REGISTER_VIRTUAL_RESOURCE",
        "EE_REGISTER_ENTITY_COMPONENT",
        "EE_REGISTER_SINGLETON_ENTITY_COMPONENT",
        "EE_REGISTER_ENTITY_SYSTEM",
        "EE_REGISTER_ENTITY_WORLD_SYSTEM",
        "EE_REGISTER",
        "EE_EXPOSE",
    };

    //-------------------------------------------------------------------------

    char const* GetReflectionMacroText( ReflectionMacro macro )
    {
        uint32_t const macroIdx = (uint32_t) macro;
        EE_ASSERT( macroIdx < (uint32_t) ReflectionMacro::NumMacros );
        return g_macroNames[macroIdx];
    }
}