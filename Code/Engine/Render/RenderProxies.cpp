
#include "RenderProxies.h"
#include "Engine/Render/Device/DeviceResourceState.h"

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

    //-------------------------------------------------------------------------------------------------------

    void MeshInstanceProxy::WriteRootTransform( Transform const& worldTransform, Float3 worldNonUniformScale )
    {
        EE_ASSERT( m_pTransformUpdateCounter != nullptr );
        EE_ASSERT( IsValid() );

        Vector scale = worldTransform.GetScaleVector() * worldNonUniformScale;
        Matrix transformMatrix = Matrix( worldTransform.GetRotation(), worldTransform.GetTranslation().GetWithW1(), scale );

        ShaderTypes::MeshInstanceTransformUpdateCommand updateCommand = {};
        updateCommand.m_instanceID = uint32_t( m_instanceHandle.m_offset );

        WriteMatrix4x3( transformMatrix, updateCommand.m_transform );

        uint64_t transformUpdateSequence = *m_pTransformUpdateSequence;
        if ( m_dstTransformUpdateIndex == ~0 || m_dstTransformUpdateSequence != transformUpdateSequence )
        {
            m_dstTransformUpdateIndex = m_pTransformUpdateCounter->fetch_add( 1 );
            m_dstTransformUpdateSequence = transformUpdateSequence;
        }

        *( m_pDstTransformUpdateCommands + m_dstTransformUpdateIndex ) = updateCommand;
    }

    void MeshInstanceProxy::WriteLocalTransforms( TArrayView<Matrix43 const> localTransforms )
    {
        EE_ASSERT( m_pTransformUpdateCounter != nullptr );
        EE_ASSERT( IsValid() );

        uint64_t transformUpdateSequence = *m_pTransformUpdateSequence;
        if ( m_dstTransformUpdateIndex == ~0 || m_dstTransformUpdateSequence != transformUpdateSequence ) // TODO: Do we need to handle overflow collision here?
        {
            m_dstTransformUpdateIndex = m_pTransformUpdateCounter->fetch_add( m_instanceHandle.m_size );
            m_dstTransformUpdateSequence = transformUpdateSequence;
        }

        for ( uint32_t instanceIndex = 0; instanceIndex < m_instanceHandle.m_size; ++instanceIndex )
        {
            ShaderTypes::MeshInstanceTransformUpdateCommand instanceUpdateCommand = {};
            instanceUpdateCommand.m_instanceID = uint32_t( m_instanceHandle.m_offset + instanceIndex );

            memcpy( &instanceUpdateCommand.m_transform, &localTransforms[instanceIndex], sizeof( Matrix43 ) );

            *( m_pDstTransformUpdateCommands + m_dstTransformUpdateIndex + instanceIndex ) = instanceUpdateCommand; // TODO: We want to keep instance transforms intact and instead upload one shared transform.
        }
    }

    //-------------------------------------------------------------------------------------------------------

    void LightInstanceProxy::WriteDirectionalLight( Float3 lightDirection, float maxIntensity, Color tintedColor, uint16_t cascadedShadowIndex )
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
        lightUpdateCommand.m_maxIntensity = maxIntensity;
        lightUpdateCommand.m_packedTintedColor = tintedColor.ToUInt32();
        lightUpdateCommand.m_cascadedShadowIndex = cascadedShadowIndex;
        std::memcpy( &lightUpdateCommand.m_lightDirection, &lightDirection, sizeof( Float3 ) );

        *( pDstLightUpdateCommand + m_dstTransformUpdateIndex ) = lightUpdateCommand;
    }

    void LightInstanceProxy::WritePointLight( Float3 lightPosition, float maxIntensity, float maxRadius, float falloff, Color tintedColor, uint16_t shadowMapHandle )
    {
        EE_ASSERT( m_pTransformUpdateCounter != nullptr );
        EE_ASSERT( IsValid() );

        uint64_t transformUpdateSequence = *m_pTransformUpdateSequence;
        if ( m_dstTransformUpdateIndex == ~0 || m_dstTransformUpdateSequence != transformUpdateSequence )
        {
            m_dstTransformUpdateIndex = m_pTransformUpdateCounter->fetch_add( 1 );
            m_dstTransformUpdateSequence = transformUpdateSequence;
        }

        ShaderTypes::PointLightUpdateCommand* pDstLightUpdateCommand = static_cast<ShaderTypes::PointLightUpdateCommand*>( m_pDstUpdateCommands );

        ShaderTypes::PointLightUpdateCommand lightUpdateCommand = {};
        lightUpdateCommand.m_instanceID = uint32_t( m_instanceHandle.m_offset );
        lightUpdateCommand.m_maxIntensity = maxIntensity;
        lightUpdateCommand.m_maxRadius = maxRadius;
        lightUpdateCommand.m_falloff = falloff;
        lightUpdateCommand.m_packedTintedColor = tintedColor.ToUInt32();
        lightUpdateCommand.m_shadowMapHandle = shadowMapHandle;
        std::memcpy( &lightUpdateCommand.m_lightPosition, &lightPosition, sizeof( Float3 ) );

        *( pDstLightUpdateCommand + m_dstTransformUpdateIndex ) = lightUpdateCommand;
    }

    void LightInstanceProxy::WriteSpotLight( Float3 lightPosition, Float3 lightDirection, float maxIntensity, float maxRadius, float falloff, Color tintedColor, float innerConeAngle, float outerConeAngle, uint16_t shadowMapHandle )
    {
        EE_ASSERT( m_pTransformUpdateCounter != nullptr );
        EE_ASSERT( IsValid() );

        uint64_t transformUpdateSequence = *m_pTransformUpdateSequence;
        if ( m_dstTransformUpdateIndex == ~0 || m_dstTransformUpdateSequence != transformUpdateSequence )
        {
            m_dstTransformUpdateIndex = m_pTransformUpdateCounter->fetch_add( 1 );
            m_dstTransformUpdateSequence = transformUpdateSequence;
        }

        ShaderTypes::SpotLightUpdateCommand* pDstLightUpdateCommand = static_cast<ShaderTypes::SpotLightUpdateCommand*>( m_pDstUpdateCommands );

        ShaderTypes::SpotLightUpdateCommand lightUpdateCommand = {};
        lightUpdateCommand.m_instanceID = uint32_t( m_instanceHandle.m_offset );
        lightUpdateCommand.m_maxIntensity = maxIntensity;
        lightUpdateCommand.m_maxRadius = maxRadius;
        lightUpdateCommand.m_falloff = falloff;
        lightUpdateCommand.m_packedTintedColor = tintedColor.ToUInt32();
        lightUpdateCommand.m_shadowMapHandle = shadowMapHandle;
        lightUpdateCommand.m_innerConeAngle = innerConeAngle;
        lightUpdateCommand.m_outerConeAngle = outerConeAngle;
        std::memcpy( &lightUpdateCommand.m_lightPosition, &lightPosition, sizeof( Float3 ) );
        std::memcpy( &lightUpdateCommand.m_lightDirection, &lightDirection, sizeof( Float3 ) );

        *( pDstLightUpdateCommand + m_dstTransformUpdateIndex ) = lightUpdateCommand;
    }

    //-------------------------------------------------------------------------------------------------------

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
}
