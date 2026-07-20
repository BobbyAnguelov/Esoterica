#pragma once

//-------------------------------------------------------------------------

namespace EE::Reflection
{
    enum class ReflectionMacro
    {
        ReflectModule,
        ReflectEnum,
        ReflectType,
        ReflectProperty,
        Resource,
        DataFile,
        EntityComponent,
        TransientEntityComponent,
        SingletonEntityComponent,
        TransientSingletonEntityComponent,
        EntitySystem,
        EntityWorldSystem,

        NumMacros,
        Unknown = NumMacros,
    };

    char const* GetReflectionMacroText( ReflectionMacro macro );
}