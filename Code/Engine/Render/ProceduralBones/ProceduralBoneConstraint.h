#pragma once
#include "Engine/_Module/API.h"
#include "Engine/Render/RenderMesh.h"
#include "Base/TypeSystem/ReflectedType.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class EE_ENGINE_API ProceduralBoneConstraint
    {
    public:

        class EE_ENGINE_API Settings : public IReflectedType
        {
            EE_REFLECT_TYPE( Settings );

        public:

            virtual ~Settings() = default;

            // Get the memory requirements of the runtime constraint
            virtual void GetMemoryRequirements( size_t& size, size_t& alignment ) const = 0;

            // Create an instance of this constraint at the target memory location
            virtual void CreateConstraint( SkeletalMesh const* pMesh, ProceduralBoneConstraint* pMemory ) const = 0;

        public:

            EE_REFLECT();
            uint8_t m_layer = 0;
        };

        //-------------------------------------------------------------------------

        struct BoneChainElement
        {
            BoneChainElement() = default;

            BoneChainElement( int32_t boneIdx, StringID boneID, Transform const& parentSpaceTransform, Transform const& modelSpaceTransform )
                : m_boneIdx( boneIdx ), m_boneID( boneID ), m_parentSpaceTransform( parentSpaceTransform ), m_modelSpaceTransform( modelSpaceTransform )
            {}

        public:

            int32_t     m_boneIdx = InvalidIndex;
            StringID    m_boneID;
            Transform   m_parentSpaceTransform;
            Transform   m_modelSpaceTransform;
        };

    public:

        virtual ~ProceduralBoneConstraint() = default;
        virtual void Solve( SkeletalMesh const* pMesh, TVector<Transform>& parentSpaceTransforms ) = 0;

    protected:

        TInlineVector<BoneChainElement, 20> CalculateModelSpaceTransformsForChain( SkeletalMesh const* pMesh, TVector<Transform> const& parentSpaceTransforms, int32_t chainEndBoneIdx ) const;
    };

    //-------------------------------------------------------------------------

    #define EE_PROCEDURAL_BONE_SETTINGS( T )\
    virtual void GetMemoryRequirements( size_t& size, size_t& alignment ) const override { size = sizeof( T ); alignment = alignof( T ); }
}