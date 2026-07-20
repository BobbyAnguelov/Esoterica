#pragma once
#include "Base/TypeSystem/ReflectedType.h"

//-------------------------------------------------------------------------

namespace EE
{
    class GameState : public IReflectedType
    {
        EE_REFLECT_TYPE( GameState );

    public:

        virtual void Update( float deltaTime ) {}
    };
}