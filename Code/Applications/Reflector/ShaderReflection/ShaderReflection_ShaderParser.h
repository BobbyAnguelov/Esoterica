#pragma once
#include "Base/Types/Arrays.h"

//-------------------------------------------------------------------------

namespace EE::Reflection
{
    class ReflectedShader;

    //-------------------------------------------------------------------------

    class ShaderParser
    {

    public:

        ShaderParser( TVector<ReflectedShader>& shaders );

        bool ParseShaders();

    private:

        bool ParseShader( ReflectedShader& shader ) const;

    private:

        TVector<ReflectedShader>&          m_shaders;
    };
}
