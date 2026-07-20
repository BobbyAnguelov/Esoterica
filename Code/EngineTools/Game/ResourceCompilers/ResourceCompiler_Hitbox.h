#pragma  once

#include "EngineTools/Resource/ResourceCompiler.h"
#include "Engine/Hitbox/Hitbox_Definition.h"
#include "Engine/Hitbox/Hitbox_Instance.h"

//-------------------------------------------------------------------------

namespace EE
{
    class HitboxCompiler : public Resource::Compiler
    {
        EE_REFLECT_TYPE( HitboxCompiler );

    public:

        HitboxCompiler();
        virtual Resource::CompilationResult Compile( Resource::CompileContext const& ctx ) const override;

    private:

        template<typename T>
        void AddRuntimeShape( HitboxDefinition& definition, uint32_t& currentShapeMemoryOffset ) const
        {
            uint32_t const alignment = ( uint32_t ) alignof( T );
            uint32_t const size = (uint32_t) sizeof( T );

            definition.m_instanceRequiredAlignment = Math::Max( definition.m_instanceRequiredAlignment, alignment );
            uint32_t const requiredShapePadding = (uint32_t) Memory::CalculatePaddingForAlignment( currentShapeMemoryOffset, alignment );

            // Set current node offset
            definition.m_instanceShapesStartOffsets.emplace_back( currentShapeMemoryOffset + requiredShapePadding );

            // Shift memory offset to take into account the current node size
            currentShapeMemoryOffset += uint32_t( size + requiredShapePadding );
        }
    };
}