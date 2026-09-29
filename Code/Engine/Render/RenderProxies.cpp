
#include "RenderProxies.h"
#include "Engine/Render/Device/DeviceResourceState.h"
#include "Engine/Render/Device/DeviceRenderView.h"

#include "Base/Math/Vector.h"
#include "Base/Math/Transform.h"
#include "Base/Math/Matrix43.h"

#include "Engine/Render/Shaders/Renderer/WorldUpdate.esh"

namespace EE::Render
{
    static void WriteMatrix4x3( Matrix const& transformMatrix, float4x3 dst )
    {
        // Validate that this matrix is a valid 4x3 matrix with implicit (0,0,0,1) column
        EE_ASSERT( Math::IsNearEqual( transformMatrix.GetRow( 0 ).GetW(), 0.0F ) );
        EE_ASSERT( Math::IsNearEqual( transformMatrix.GetRow( 1 ).GetW(), 0.0F ) );
        EE_ASSERT( Math::IsNearEqual( transformMatrix.GetRow( 2 ).GetW(), 0.0F ) );
        EE_ASSERT( Math::IsNearEqual( transformMatrix.GetRow( 3 ).GetW(), 1.0F ) );

        transformMatrix.GetRow( 0 ).StoreFloat3( dst + 0 );
        transformMatrix.GetRow( 1 ).StoreFloat3( dst + 3 );
        transformMatrix.GetRow( 2 ).StoreFloat3( dst + 6 );
        transformMatrix.GetRow( 3 ).StoreFloat3( dst + 9 );
    }

    //-------------------------------------------------------------------------

    void MeshInstanceRootProxy::WriteRootTransform( Transform const& worldTransform, Float3 worldNonUniformScale, Float3 worldAABBCenter, Float3 worldAABBHalfExtents )
    {
        EE_ASSERT( m_pTransformUpdateCounter != nullptr );
        EE_ASSERT( IsValid() );

        Vector scale = worldTransform.GetScaleVector() * worldNonUniformScale;
        Matrix transformMatrix = Matrix( worldTransform.GetRotation(), worldTransform.GetTranslation().GetWithW1(), scale );

        ShaderTypes::MeshInstanceRootUpdateCommand updateCommand = {};
        updateCommand.m_instanceID = uint32_t( m_instanceHandle.m_offset );

        WriteMatrix4x3( transformMatrix, updateCommand.m_transform );

        updateCommand.EncodeWorldAABB( worldAABBCenter, worldAABBHalfExtents, worldTransform.GetTranslation().ToFloat3() );

        uint64_t transformUpdateSequence = *m_pTransformUpdateSequence;
        if ( m_dstTransformUpdateIndex == ~0 || m_dstTransformUpdateSequence != transformUpdateSequence )
        {
            m_dstTransformUpdateIndex = m_pTransformUpdateCounter->fetch_add( 1 );
            m_dstTransformUpdateSequence = transformUpdateSequence;
        }

        *( m_pDstUpdateCommands + m_dstTransformUpdateIndex ) = updateCommand;
    }

    void MeshInstanceProxy::StartLocalTransformWrite()
    {
        EE_ASSERT( m_pTransformUpdateCounter != nullptr );
        EE_ASSERT( IsValid() );

        uint64_t transformUpdateSequence = *m_pTransformUpdateSequence;
        if ( m_dstTransformUpdateIndex == ~0 || m_dstTransformUpdateSequence != transformUpdateSequence ) // TODO: Do we need to handle overflow collision here?
        {
            m_dstTransformUpdateIndex = m_pTransformUpdateCounter->fetch_add( m_instanceHandle.m_size );
            m_dstTransformUpdateSequence = transformUpdateSequence;
        }

        m_numWrittenLocalTransforms = 0;
    }

    void MeshInstanceProxy::WriteLocalTransform( Matrix43 const& localTransform )
    {
        EE_ASSERT( IsValid() );
        EE_ASSERT( m_numWrittenLocalTransforms < m_instanceHandle.m_size );

        ShaderTypes::MeshInstanceTransformUpdateCommand instanceUpdateCommand = {};
        instanceUpdateCommand.m_instanceID = uint32_t( m_instanceHandle.m_offset + m_numWrittenLocalTransforms );
        instanceUpdateCommand.m_shaderIndex = m_shaderIndex;

        memcpy( &instanceUpdateCommand.m_transform, &localTransform, sizeof( Matrix43 ) );

        m_pDstTransformUpdateCommands[m_dstTransformUpdateIndex + m_numWrittenLocalTransforms] = instanceUpdateCommand; // TODO: We want to keep instance transforms intact and instead upload one shared transform.
        m_numWrittenLocalTransforms++;
    }

    void MeshInstanceProxy::SubmitLocalTransformWrite() const
    {
        EE_ASSERT( m_numWrittenLocalTransforms == m_instanceHandle.m_size );
    }

    //-------------------------------------------------------------------------

    void LightInstanceProxy::WriteDirectionalLight( ShaderTypes::LightInstance_DirectionalLight const& light )
    {
        EE_ASSERT( m_pTransformUpdateCounter != nullptr );
        EE_ASSERT( IsValid() );

        uint64_t transformUpdateSequence = *m_pTransformUpdateSequence;
        if ( m_dstTransformUpdateIndex == ~0 || m_dstTransformUpdateSequence != transformUpdateSequence )
        {
            m_dstTransformUpdateIndex = m_pTransformUpdateCounter->fetch_add( 1 );
            m_dstTransformUpdateSequence = transformUpdateSequence;
        }

        ShaderTypes::DirectionalLightUpdateCommand* pDstLightUpdateCommand = static_cast<ShaderTypes::DirectionalLightUpdateCommand*>( m_pDstUpdateCommands );

        ShaderTypes::DirectionalLightUpdateCommand lightUpdateCommand = {};
        lightUpdateCommand.m_instanceID = uint32_t( m_instanceHandle.m_offset );
        lightUpdateCommand.m_maxIntensity = light.m_maxIntensity;
        memcpy( lightUpdateCommand.m_lightDirection, light.m_lightDirection, sizeof( light.m_lightDirection ) );
        lightUpdateCommand.m_packedTintedColor = light.m_packedTintedColor;
        lightUpdateCommand.m_shadowCascades = light.m_shadowCascades;
        memcpy( lightUpdateCommand.m_shadowMatrix, light.m_shadowMatrix, sizeof( light.m_shadowMatrix ) );
        memcpy( lightUpdateCommand.m_cascadeOffsets, light.m_cascadeOffsets, sizeof( light.m_cascadeOffsets ) );
        memcpy( lightUpdateCommand.m_cascadeScales, light.m_cascadeScales, sizeof( light.m_cascadeScales ) );
        memcpy( lightUpdateCommand.m_cascadeSize, light.m_cascadeSize, sizeof( light.m_cascadeSize ) );

        *( pDstLightUpdateCommand + m_dstTransformUpdateIndex ) = lightUpdateCommand;
    }

    void LightInstanceProxy::WritePointLight( Float3 lightPosition, float maxIntensity, float maxRadius, float falloff, Color tintedColor )
    {
        EE_ASSERT( m_pTransformUpdateCounter != nullptr );
        EE_ASSERT( IsValid() );

        uint64_t transformUpdateSequence = *m_pTransformUpdateSequence;
        if ( m_dstTransformUpdateIndex == ~0 || m_dstTransformUpdateSequence != transformUpdateSequence )
        {
            m_dstTransformUpdateIndex = m_pTransformUpdateCounter->fetch_add( 1 );
            m_dstTransformUpdateSequence = transformUpdateSequence;
        }

        ShaderTypes::PointLightTransformUpdateCommand* pDstLightUpdateCommand = static_cast<ShaderTypes::PointLightTransformUpdateCommand*>( m_pDstUpdateCommands );

        ShaderTypes::PointLightTransformUpdateCommand lightUpdateCommand = {};
        lightUpdateCommand.m_instanceID = uint32_t( m_instanceHandle.m_offset );
        lightUpdateCommand.m_maxIntensity = maxIntensity;
        lightUpdateCommand.m_maxRadius = maxRadius;
        lightUpdateCommand.m_falloff = falloff;
        lightUpdateCommand.m_packedTintedColor = tintedColor.ToUInt32();
        std::memcpy( &lightUpdateCommand.m_lightPosition, &lightPosition, sizeof( Float3 ) );

        *( pDstLightUpdateCommand + m_dstTransformUpdateIndex ) = lightUpdateCommand;
    }

    void LightInstanceProxy::WriteSpotLight( Float3 lightPosition, Float3 lightDirection, float maxIntensity, float maxRadius, float falloff, Color tintedColor, float innerConeAngle, float outerConeAngle, Matrix const& shadowViewProjectionMatrix )
    {
        EE_ASSERT( m_pTransformUpdateCounter != nullptr );
        EE_ASSERT( IsValid() );

        uint64_t transformUpdateSequence = *m_pTransformUpdateSequence;
        if ( m_dstTransformUpdateIndex == ~0 || m_dstTransformUpdateSequence != transformUpdateSequence )
        {
            m_dstTransformUpdateIndex = m_pTransformUpdateCounter->fetch_add( 1 );
            m_dstTransformUpdateSequence = transformUpdateSequence;
        }

        ShaderTypes::SpotLightTransformUpdateCommand* pDstLightUpdateCommand = static_cast<ShaderTypes::SpotLightTransformUpdateCommand*>( m_pDstUpdateCommands );

        ShaderTypes::SpotLightTransformUpdateCommand lightUpdateCommand = {};
        lightUpdateCommand.m_instanceID = uint32_t( m_instanceHandle.m_offset );
        lightUpdateCommand.m_maxIntensity = maxIntensity;
        lightUpdateCommand.m_maxRadius = maxRadius;
        lightUpdateCommand.m_falloff = falloff;
        lightUpdateCommand.m_packedTintedColor = tintedColor.ToUInt32();
        lightUpdateCommand.m_innerConeAngle = innerConeAngle;
        lightUpdateCommand.m_outerConeAngle = outerConeAngle;
        std::memcpy( &lightUpdateCommand.m_lightPosition, &lightPosition, sizeof( Float3 ) );
        std::memcpy( &lightUpdateCommand.m_lightDirection, &lightDirection, sizeof( Float3 ) );
        std::memcpy( lightUpdateCommand.m_shadowMatrix, shadowViewProjectionMatrix.m_rows, sizeof( lightUpdateCommand.m_shadowMatrix ) );

        *( pDstLightUpdateCommand + m_dstTransformUpdateIndex ) = lightUpdateCommand;
    }

    //-------------------------------------------------------------------------

    static_assert( sizeof( Transform ) == sizeof( ShaderTypes::SkinningTransform ) );

    void SkinningProxy::WriteTransforms( TArrayView<Transform const> boneTransforms, TArrayView<Transform const> inverseBindPose )
    {
        EE_ASSERT( m_pTransformUpdateCounter != nullptr );
        EE_ASSERT( IsValid() );
        EE_ASSERT( boneTransforms.size() == inverseBindPose.size() );
        EE_ASSERT( m_bonesHandle.m_size == uint32_t( boneTransforms.size() ) );

        uint64_t transformUpdateSequence = *m_pTransformUpdateSequence;
        if ( m_dstTransformUpdateIndex == ~0 || m_dstTransformUpdateSequence != transformUpdateSequence ) // TODO: Do we need to handle overflow collision here?
        {
            m_dstTransformUpdateIndex = m_pTransformUpdateCounter->fetch_add( uint32_t( boneTransforms.size() ) );
            m_dstTransformUpdateSequence = transformUpdateSequence;
        }

        for ( size_t boneIndex = 0; boneIndex < boneTransforms.size(); ++boneIndex )
        {
            Transform const skinningTransform = inverseBindPose[boneIndex] * boneTransforms[boneIndex];

            uint32_t instanceID = uint32_t( m_bonesHandle.m_offset + boneIndex );

            ShaderTypes::SkinningTransformUpdateCommand instanceUpdateCommand = {};
            instanceUpdateCommand.EncodeSkinningTransform( instanceID, skinningTransform );

            *( m_pDstTransformUpdateCommands + m_dstTransformUpdateIndex + boneIndex ) = instanceUpdateCommand;
        }
    }

    //-------------------------------------------------------------------------

    void RenderViewProxy::StartRenderViewWrite()
    {
        EE_ASSERT( m_pUpdateCounter != nullptr );
        EE_ASSERT( IsValid() );

        uint64_t updateSequence = *m_pUpdateSequence;
        if ( m_dstUpdateIndex == ~0 || m_dstUpdateSequence != updateSequence ) // TODO: Do we need to handle overflow collision here?
        {
            m_dstUpdateIndex = m_pUpdateCounter->fetch_add( m_renderViewHandle.m_handle.m_size );
            m_dstUpdateSequence = updateSequence;
        }

        m_numWrittenRenderViews = 0;
    }

    void RenderViewProxy::WriteRenderView( uint32_t viewIndex, ShaderTypes::RenderView const& renderView )
    {
        EE_ASSERT( IsValid() );
        EE_ASSERT( viewIndex < m_renderViewHandle.m_handle.m_size );
        EE_ASSERT( viewIndex == m_numWrittenRenderViews );

        ShaderTypes::RenderViewUpdateCommand updateCommand = {};
        updateCommand.m_renderView = renderView;
        updateCommand.m_deviceRenderViewIndex = GetBaseRenderViewIndex() + viewIndex;

        *( m_pDstUpdateCommands + m_dstUpdateIndex + m_numWrittenRenderViews ) = updateCommand;
        m_numWrittenRenderViews++;
    }

    void RenderViewProxy::SubmitRenderViewWrite() const
    {
        EE_ASSERT( m_numWrittenRenderViews == m_renderViewHandle.m_handle.m_size );
    }

    DeviceRenderView& RenderViewProxy::GetRenderView( uint32_t viewIndex )
    {
        EE_ASSERT( IsValid() );
        EE_ASSERT( viewIndex < GetNumRenderViews() );
        return m_renderViewHandle.m_data[viewIndex];
    }

    DeviceRenderView const& RenderViewProxy::GetRenderView( uint32_t viewIndex ) const
    {
        EE_ASSERT( IsValid() );
        EE_ASSERT( viewIndex < GetNumRenderViews() );
        return m_renderViewHandle.m_data[viewIndex];
    }

    //-------------------------------------------------------------------------

    static inline void StoreMatrix( float4x4& dst, Matrix const& src )
    {
        static_assert( sizeof( float4x4 ) == sizeof( Matrix ) );
        std::memcpy( dst, src.m_rows, sizeof( float4x4 ) );
    }

    static ShaderTypes::RenderView CreateCommonRenderView
    (
        Matrix const&   viewMatrix,
        Matrix const&   projectionMatrix,
        Matrix const&   viewProjectionMatrix,
        Float4 const&   renderTargetSize,
        float           znear,
        uint32_t        renderViewFlags,
        uint32_t        renderViewLayerFlags
    )
    {
        ShaderTypes::RenderView renderView = {};

        StoreMatrix( renderView.m_viewMatrix, viewMatrix );
        StoreMatrix( renderView.m_viewProjectionMatrix, viewProjectionMatrix );
        StoreMatrix( renderView.m_inverseViewMatrix, viewMatrix.GetInverse() );
        StoreMatrix( renderView.m_inverseProjectionMatrix, projectionMatrix.GetInverse() );
        StoreMatrix( renderView.m_inverseViewProjectionMatrix, viewProjectionMatrix.GetInverse() );

        renderView.m_renderTargetSize[0] = renderTargetSize.m_x;
        renderView.m_renderTargetSize[1] = renderTargetSize.m_y;
        renderView.m_renderTargetSize[2] = renderTargetSize.m_z;
        renderView.m_renderTargetSize[3] = renderTargetSize.m_w;

        renderView.m_renderViewFlags = renderViewFlags;
        renderView.m_renderViewLayerFlags = renderViewLayerFlags;

        renderView.m_projectionP00 = projectionMatrix.m_values[0][0];
        renderView.m_projectionP11 = projectionMatrix.m_values[1][1];
        renderView.m_znear = znear;

        return renderView;
    }

    static inline Float4 PackRenderTargetSize( uint32_t resolution )
    {
        float const inverseResolution = ( resolution > 0 ) ? ( 1.0F / float( resolution ) ) : 0.0F;
        return Float4( float( resolution ), float( resolution ), inverseResolution, inverseResolution );
    }

    //-------------------------------------------------------------------------

    void RenderViewProxy::WriteRenderView( uint32_t viewIndex, Math::ViewVolume const& viewVolume, Float2 renderTargetSize, uint32_t renderViewFlags )
    {
        EE_ASSERT( !renderTargetSize.IsNearZero() );

        Matrix const& viewMatrix = viewVolume.GetViewMatrix();
        Matrix const projectionMatrix = viewVolume.GetProjectionMatrix() * Matrix::ReverseZ;
        Matrix const viewProjectionMatrix = viewMatrix * projectionMatrix;

        Float4 const renderTargetSize4 = Float4( renderTargetSize.m_x, renderTargetSize.m_y, 1.0F / renderTargetSize.m_x, 1.0F / renderTargetSize.m_y );

        ShaderTypes::RenderView renderView = CreateCommonRenderView
        (
            viewMatrix,
            projectionMatrix,
            viewProjectionMatrix,
            renderTargetSize4,
            viewVolume.GetDepthRange().m_begin,
            renderViewFlags,
            ShaderTypes::RENDER_VIEW_LAYER_FLAG_FORWARD_SHADING
        );

        WriteRenderView( viewIndex, renderView );
    }

    void RenderViewProxy::WritePointLightShadowRenderView( uint32_t viewIndex, Vector const& viewPosition, float maxRadius, uint32_t resolution )
    {
        Float3 const lightPosition = viewPosition.ToFloat3();

        ShaderTypes::RenderView renderView = {};
        StoreMatrix( renderView.m_viewMatrix, Matrix::Identity );
        StoreMatrix( renderView.m_viewProjectionMatrix, Matrix::Identity );
        StoreMatrix( renderView.m_inverseViewMatrix, Matrix::Identity );
        StoreMatrix( renderView.m_inverseProjectionMatrix, Matrix::Identity );
        StoreMatrix( renderView.m_inverseViewProjectionMatrix, Matrix::Identity );

        Float4 const renderTargetSize = PackRenderTargetSize( resolution );
        renderView.m_renderTargetSize[0] = renderTargetSize.m_x;
        renderView.m_renderTargetSize[1] = renderTargetSize.m_y;
        renderView.m_renderTargetSize[2] = renderTargetSize.m_z;
        renderView.m_renderTargetSize[3] = renderTargetSize.m_w;

        renderView.m_renderViewFlags = ShaderTypes::RENDER_VIEW_FLAG_DEPTH_ONLY | ShaderTypes::RENDER_VIEW_FLAG_SHADOW_FACE;
        renderView.m_renderViewLayerFlags = ShaderTypes::RENDER_VIEW_LAYER_FLAG_SHADOW_MAP;

        renderView.m_znear = ShaderTypes::PunctualShadowNearPlane( maxRadius );

        renderView.m_shadowLightSphere[0] = lightPosition.m_x;
        renderView.m_shadowLightSphere[1] = lightPosition.m_y;
        renderView.m_shadowLightSphere[2] = lightPosition.m_z;
        renderView.m_shadowLightSphere[3] = maxRadius;

        WriteRenderView( viewIndex, renderView );
    }

    Matrix RenderViewProxy::WriteSpotLightShadowRenderView( uint32_t viewIndex, Vector const& viewPosition, Vector const& beamDirection, float halfAngleRadians, float maxRadius, uint32_t resolution )
    {
        float const nearPlane = ShaderTypes::PunctualShadowNearPlane( maxRadius );
        float const farPlane = ShaderTypes::PunctualShadowFarPlane( maxRadius );

        Matrix const projectionMatrix = Math::CreatePerspectiveProjectionMatrix( halfAngleRadians * 2.0F, 1.0F, nearPlane, farPlane ) * Matrix::ReverseZ;

        Vector viewUpDirection = Vector::WorldUp;
        if ( Math::Abs( beamDirection.GetDot3( viewUpDirection ) ) > 0.99F )
        {
            viewUpDirection = Vector::WorldRight;
        }

        Matrix const viewMatrix = Math::CreateLookAtMatrix( viewPosition, viewPosition + beamDirection, viewUpDirection );
        Matrix const viewProjectionMatrix = viewMatrix * projectionMatrix;

        ShaderTypes::RenderView renderView = CreateCommonRenderView
        (
            viewMatrix,
            projectionMatrix,
            viewProjectionMatrix,
            PackRenderTargetSize( resolution ),
            nearPlane,
            ShaderTypes::RENDER_VIEW_FLAG_DEPTH_ONLY,
            ShaderTypes::RENDER_VIEW_LAYER_FLAG_SHADOW_MAP
        );

        WriteRenderView( viewIndex, renderView );

        return viewProjectionMatrix;
    }

    void RenderViewProxy::WriteCascadedShadowRenderView( uint32_t viewIndex, Matrix const& viewMatrix, Matrix const& projectionMatrix, float znear, uint32_t resolution )
    {
        Matrix const viewProjectionMatrix = viewMatrix * projectionMatrix;

        ShaderTypes::RenderView renderView = CreateCommonRenderView
        (
            viewMatrix,
            projectionMatrix,
            viewProjectionMatrix,
            PackRenderTargetSize( resolution ),
            znear,
            ShaderTypes::RENDER_VIEW_FLAG_DEPTH_ONLY,
            ShaderTypes::RENDER_VIEW_LAYER_FLAG_SHADOW_MAP
        );

        WriteRenderView( viewIndex, renderView );
    }

    void RenderViewProxy::WriteGlobalEnvironmentMapRenderView( uint32_t viewIndex, Vector const& viewPosition, float znear, float zfar, uint32_t resolution )
    {
        EE_ASSERT( viewIndex < g_NumGlobalEnvironmentMapViews );

        static Vector const faceDirections[g_NumGlobalEnvironmentMapViews] =
        {
            Vector::UnitX, -Vector::UnitX,
            Vector::UnitY, -Vector::UnitY,
            Vector::UnitZ, -Vector::UnitZ
        };

        static Vector const faceUpDirections[g_NumGlobalEnvironmentMapViews] =
        {
            Vector::UnitY, Vector::UnitY,
            Vector::UnitZ, Vector::UnitZ,
            Vector::UnitY, Vector::UnitY
        };

        static Vector const faceMirrorScales[g_NumGlobalEnvironmentMapViews] =
        {
            Vector( -1.0F, 1.0F, 1.0F, 1.0F ),
            Vector( -1.0F, 1.0F, 1.0F, 1.0F ),
            Vector( 1.0F, -1.0F, 1.0F, 1.0F ),
            Vector( -1.0F, 1.0F, 1.0F, 1.0F ),
            Vector( -1.0F, 1.0F, 1.0F, 1.0F ),
            Vector( -1.0F, 1.0F, 1.0F, 1.0F )
        };

        Matrix mirrorMatrix = Matrix::Identity;
        mirrorMatrix.SetScale( faceMirrorScales[viewIndex] );

        Matrix const viewMatrix = Math::CreateLookAtMatrix( viewPosition, viewPosition + faceDirections[viewIndex], faceUpDirections[viewIndex] );
        Matrix const projectionMatrix = Math::CreatePerspectiveProjectionMatrix( Math::DegreesToRadians * 90.0F, 1.0F, znear, zfar ) * Matrix::ReverseZ;
        Matrix const viewProjectionMatrix = viewMatrix * projectionMatrix * mirrorMatrix;

        ShaderTypes::RenderView renderView = CreateCommonRenderView
        (
            viewMatrix,
            projectionMatrix,
            viewProjectionMatrix,
            PackRenderTargetSize( resolution ),
            znear,
            ShaderTypes::RENDER_VIEW_FLAG_NONE,
            ShaderTypes::RENDER_VIEW_LAYER_FLAG_GLOBAL_ENVIRONMENT_MAP
        );

        WriteRenderView( viewIndex, renderView );
    }
}
