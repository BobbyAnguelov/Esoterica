#pragma once
#include "Base/Render/PageAllocator.h"
#include "Base/Types/Color.h"
#include "EASTL/atomic.h"

//-------------------------------------------------------------------------

namespace EE
{
    class Transform;
    struct Matrix43;
}

namespace EE::Render
{
    namespace ShaderTypes
    {
        struct Transform;
        struct SkinningTransform;
        struct Mesh;
        struct MeshCluster;
        struct DirectionalLightUpdateCommand;
        struct PointLightUpdateCommand;
        struct SpotLightUpdateCommand;
        struct SkinningTransformUpdateCommand;
        struct MeshInstanceTransformUpdateCommand;
    }

    //-------------------------------------------------------------------------

    using Buffer32ByteBlock = uint32_t[8];

    using MeshHandle = PageAllocator<ShaderTypes::Mesh, uint16_t>::Handle;
    using ClustersHandle = PageAllocator<ShaderTypes::MeshCluster, uint32_t>::Handle;
    using ShaderDataHandle = PageAllocator<Buffer32ByteBlock, uint32_t>::Handle;

    //-------------------------------------------------------------------------

    struct MeshInstanceProxy final
    {
        void WriteRootTransform( Transform const& worldTransform, Float3 worldNonUniformScale );
        void WriteLocalTransforms( TArrayView<Matrix43 const> localTransforms );

        inline bool IsValid() const { return m_instanceHandle.IsValid(); }

        //-------------------------------------------------------------------------

        eastl::atomic<uint32_t>*                                                m_pTransformUpdateCounter = nullptr;
        uint64_t const*                                                         m_pTransformUpdateSequence = nullptr;
        ShaderTypes::MeshInstanceTransformUpdateCommand*                        m_pDstTransformUpdateCommands = nullptr; // TODO: Need a workaround for platforms that don't support virtual memory. Can use PageAllocator<T> handle for that.

        uint64_t                                                                m_dstTransformUpdateSequence = ~0ULL;
        uint32_t                                                                m_dstTransformUpdateIndex = ~0U;
        HandleAllocator<uint32_t>::Handle                                       m_instanceHandle = {};
    };

    //-------------------------------------------------------------------------

    struct LightInstanceProxy final
    {
        void WriteDirectionalLight( Float3 lightDirection, float maxIntensity, Color tintedColor, uint16_t cascadedShadowIndex );
        void WritePointLight( Float3 lightPosition, float maxIntensity, float maxRadius, float falloff, Color tintedColor, uint16_t shadowMapHandle );
        void WriteSpotLight( Float3 lightPosition, Float3 lightDirection, float maxIntensity, float maxRadius, float falloff, Color tintedColor, float innerConeAngle, float outerConeAngle, uint16_t shadowMapHandle );

        inline bool IsValid() const { return m_instanceHandle.IsValid(); }

        //-------------------------------------------------------------------------

        eastl::atomic<uint32_t>*                                                m_pTransformUpdateCounter = nullptr;
        uint64_t const*                                                         m_pTransformUpdateSequence = nullptr;
        void*                                                                   m_pDstUpdateCommands = nullptr;

        uint64_t                                                                m_dstTransformUpdateSequence = ~0ULL;
        uint32_t                                                                m_dstTransformUpdateIndex = ~0U;
        HandleAllocator<uint32_t>::Handle                                       m_instanceHandle = {};
    };

    //-------------------------------------------------------------------------

    struct SkinningProxy final
    {
        void WriteTransforms( TArrayView<Transform const> boneTransforms, TArrayView<Transform const> inverseBindPose );

        inline bool IsValid() const { return m_bonesHandle.IsValid(); }

        //-------------------------------------------------------------------------

        eastl::atomic<uint32_t>*                                                m_pTransformUpdateCounter = nullptr;
        uint64_t const*                                                         m_pTransformUpdateSequence = nullptr;
        ShaderTypes::SkinningTransformUpdateCommand*                            m_pDstTransformUpdateCommands = nullptr; // TODO: Need a workaround for platforms that don't support virtual memory. Can use PageAllocator<T> handle for that.

        uint64_t                                                                m_dstTransformUpdateSequence = ~0ULL;
        uint32_t                                                                m_dstTransformUpdateIndex = ~0U;
        HandleAllocator<uint32_t>::Handle                                       m_bonesHandle = {};
    };
}
