#pragma once
#include "Base/Render/PageAllocator.h"
#include "Base/Types/Arrays.h"
#include "Base/Types/Color.h"
#include "Base/Math/Matrix.h"
#include "Base/Math/ViewVolume.h"
#include "EASTL/atomic.h"

//-------------------------------------------------------------------------

namespace EE
{
    class Transform;
    struct Matrix43;
}

namespace EE::Render
{
    class Material;
    struct DeviceRenderView;

    namespace ShaderTypes
    {
        struct Transform;
        struct SkinningTransform;
        struct LightInstance_DirectionalLight;
        struct DirectionalLightUpdateCommand;
        struct PointLightInitializeCommand;
        struct PointLightTransformUpdateCommand;
        struct SpotLightInitializeCommand;
        struct SpotLightTransformUpdateCommand;
        struct SkinningTransformUpdateCommand;
        struct MeshInstanceTransformUpdateCommand;
        struct MeshInstanceRootUpdateCommand;
        struct RenderView;
        struct RenderViewUpdateCommand;
    }

    //-------------------------------------------------------------------------

    using Buffer32ByteBlock = uint32_t[8];

    using ShaderDataHandle = PageAllocator<Buffer32ByteBlock, uint32_t>::Handle;

    //-------------------------------------------------------------------------

    struct MeshInstanceRootProxy final
    {
        void WriteRootTransform( Transform const& worldTransform, Float3 worldNonUniformScale, Float3 worldAABBCenter, Float3 worldAABBHalfExtents );

        inline bool IsValid() const { return m_instanceHandle.IsValid(); }

        //-------------------------------------------------------------------------

        eastl::atomic<uint32_t>*                                                m_pTransformUpdateCounter = nullptr;
        uint64_t const*                                                         m_pTransformUpdateSequence = nullptr;
        ShaderTypes::MeshInstanceRootUpdateCommand*                             m_pDstUpdateCommands = nullptr; // TODO: Need a workaround for platforms that don't support virtual memory. Can use PageAllocator<T> handle for that.

        uint64_t                                                                m_dstTransformUpdateSequence = ~0ULL;
        uint32_t                                                                m_dstTransformUpdateIndex = ~0U;
        HandleAllocator<uint32_t>::Handle                                       m_instanceHandle = {};
    };

    //-------------------------------------------------------------------------

    struct MeshInstanceProxy final
    {
        void StartLocalTransformWrite();
        void WriteLocalTransform( Matrix43 const& localTransform );
        void SubmitLocalTransformWrite() const;

        inline bool IsValid() const { return m_instanceHandle.IsValid(); }

        //-------------------------------------------------------------------------

        eastl::atomic<uint32_t>*                                                m_pTransformUpdateCounter = nullptr;
        uint64_t const*                                                         m_pTransformUpdateSequence = nullptr;
        ShaderTypes::MeshInstanceTransformUpdateCommand*                        m_pDstTransformUpdateCommands = nullptr; // TODO: Need a workaround for platforms that don't support virtual memory. Can use PageAllocator<T> handle for that.

        uint64_t                                                                m_dstTransformUpdateSequence = ~0ULL;
        uint32_t                                                                m_dstTransformUpdateIndex = ~0U;
        uint32_t                                                                m_numWrittenLocalTransforms = 0; // Write cursor for the reserved local transform range
        HandleAllocator<uint32_t>::Handle                                       m_instanceHandle = {};

        uint32_t                                                                m_shaderIndex = ~0U;
        HandleAllocator<uint32_t>::Handle                                       m_clusterHandle = {};
    };

    //-------------------------------------------------------------------------

    struct LightInstanceProxy final
    {
        void WriteDirectionalLight( ShaderTypes::LightInstance_DirectionalLight const& light );
        void WritePointLight( Float3 lightPosition, float maxIntensity, float maxRadius, float falloff, Color tintedColor );
        void WriteSpotLight( Float3 lightPosition, Float3 lightDirection, float maxIntensity, float maxRadius, float falloff, Color tintedColor, float innerConeAngle, float outerConeAngle, Matrix const& shadowViewProjectionMatrix );

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

    //-------------------------------------------------------------------------

    struct RenderViewProxy final
    {
        void StartRenderViewWrite();
        void WriteRenderView( uint32_t viewIndex, Math::ViewVolume const& viewVolume, Float2 renderTargetSize, uint32_t renderViewFlags );
        void WritePointLightShadowRenderView( uint32_t viewIndex, Vector const& viewPosition, float maxRadius, uint32_t resolution );
        Matrix WriteSpotLightShadowRenderView( uint32_t viewIndex, Vector const& viewPosition, Vector const& beamDirection, float halfAngleRadians, float maxRadius, uint32_t resolution );
        void WriteCascadedShadowRenderView( uint32_t viewIndex, Matrix const& viewMatrix, Matrix const& projectionMatrix, float znear, uint32_t resolution );
        void WriteGlobalEnvironmentMapRenderView( uint32_t viewIndex, Vector const& viewPosition, float znear, float zfar, uint32_t resolution );
        void SubmitRenderViewWrite() const;

        inline bool IsValid() const { return m_renderViewHandle.m_handle.IsValid(); }

        inline uint32_t GetNumRenderViews() const { return m_renderViewHandle.m_handle.m_size; }
        inline uint32_t GetBaseRenderViewIndex() const { return m_renderViewHandle.m_handle.m_offset; }

        DeviceRenderView& GetRenderView( uint32_t viewIndex );
        DeviceRenderView const& GetRenderView( uint32_t viewIndex ) const;

        //-------------------------------------------------------------------------

        eastl::atomic<uint32_t>*                                                m_pUpdateCounter = nullptr;
        uint64_t const*                                                         m_pUpdateSequence = nullptr;
        ShaderTypes::RenderViewUpdateCommand*                                   m_pDstUpdateCommands = nullptr; // TODO: Need a workaround for platforms that don't support virtual memory. Can use PageAllocator<T> handle for that.

        uint64_t                                                                m_dstUpdateSequence = ~0ULL;
        uint32_t                                                                m_dstUpdateIndex = ~0U;
        uint32_t                                                                m_numWrittenRenderViews = 0;
        PageAllocator<DeviceRenderView, uint16_t>::Handle                       m_renderViewHandle = {};

    private:

        void WriteRenderView( uint32_t viewIndex, ShaderTypes::RenderView const& renderView );
    };
}
