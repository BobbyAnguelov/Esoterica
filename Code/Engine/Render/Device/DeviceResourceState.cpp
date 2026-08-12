
#include "DeviceResourceState.h"

namespace EE::Render
{
    void DeviceResourceStates::FlushBarriers( RHI::CommandBuffer* pCommandBuffer )
    {
        for ( Texture const& texture : m_pendingTextureStates )
        {
            EE_ASSERT( texture.m_pTextureState->m_currentSync.IsAnyFlagSet() );
            EE_ASSERT( texture.m_pTextureState->m_currentAccess.IsAnyFlagSet() );
            EE_ASSERT( texture.m_pTextureState->m_currentState != RHI::TextureState::Undefined );

            TBitFlags<RHI::PipelineStage> destinationSync = texture.m_textureSync;
            TBitFlags<RHI::ResourceAccess> destinationAccess = texture.m_textureAccess;
            RHI::TextureState destinationState = texture.m_textureState;

            EE_ASSERT( destinationSync.IsAnyFlagSet() );
            EE_ASSERT( destinationAccess.IsAnyFlagSet() );
            EE_ASSERT( destinationState != RHI::TextureState::Undefined );

            bool writeable = destinationAccess & RHI::ResourceAccessFlags_AllWriteable;
            writeable = writeable || ( destinationAccess.IsFlagSet( RHI::ResourceAccess::DepthRead ) );

            if ( writeable ||
                 destinationSync != texture.m_pTextureState->m_currentSync ||
                 destinationAccess != texture.m_pTextureState->m_currentAccess ||
                 destinationState != texture.m_pTextureState->m_currentState )
            {
                RHI::CmdBarrier( pCommandBuffer, texture.m_pTextureState->m_pTexture,
                                 texture.m_pTextureState->m_currentSync, destinationSync,
                                 texture.m_pTextureState->m_currentAccess, destinationAccess,
                                 texture.m_pTextureState->m_currentState, destinationState,
                                 {}, {} );

                texture.m_pTextureState->m_currentSync.AppendFlags( destinationSync );
                //texture.m_pTextureState->m_currentSync.ClearFlag( RHI::PipelineStage::Copy );

                texture.m_pTextureState->m_currentAccess.AppendFlags( destinationAccess );
                texture.m_pTextureState->m_currentAccess.ClearFlags( RHI::ResourceAccessFlags_AllWriteable );
                texture.m_pTextureState->m_currentState = destinationState;
            }

            if ( writeable )
            {
                texture.m_pTextureState->m_currentSync = destinationSync;
                texture.m_pTextureState->m_currentAccess = destinationAccess;
            }
        }

        m_pendingTextureStates.clear();
    }

    bool DeviceResourceStates::HasPendingBarriers() const
    {
        return !m_pendingTextureStates.empty();
    }

    //-------------------------------------------------------------------------

    static void ValidateReadOnlyAccess( TBitFlags<RHI::PipelineStage> sync, TBitFlags<RHI::ResourceAccess> access )
    {
        EE_ASSERT( !access.IsFlagSet( RHI::ResourceAccess::AccelerationStructureWrite ) );
        EE_ASSERT( !access.IsFlagSet( RHI::ResourceAccess::DepthWrite ) );
        EE_ASSERT( !access.IsFlagSet( RHI::ResourceAccess::VideoDecodeWrite ) );
        EE_ASSERT( !access.IsFlagSet( RHI::ResourceAccess::VideoEncodeWrite ) );
        EE_ASSERT( !access.IsFlagSet( RHI::ResourceAccess::VideoProcessWrite ) );
        EE_ASSERT( !access.IsFlagSet( RHI::ResourceAccess::RenderTarget ) );
        EE_ASSERT( !access.IsFlagSet( RHI::ResourceAccess::UnorderedAccess ) );
        EE_ASSERT( !access.IsFlagSet( RHI::ResourceAccess::CopyDestination ) );
    }

    static void ValidateWriteableAccess( TBitFlags<RHI::PipelineStage> sync, TBitFlags<RHI::ResourceAccess> access )
    {
        EE_ASSERT( access.AreAnyFlagsSet( RHI::ResourceAccess::DepthRead, RHI::ResourceAccess::DepthWrite, RHI::ResourceAccess::RenderTarget, RHI::ResourceAccess::UnorderedAccess, RHI::ResourceAccess::CopyDestination ) );
        EE_ASSERT( sync == TBitFlags<RHI::PipelineStage>( RHI::PipelineStage::All ) || sync.AreAnyFlagsSet( RHI::PipelineStage::AllShader, RHI::PipelineStage::ComputeShader, RHI::PipelineStage::NonPixelShader, RHI::PipelineStage::PixelShader, RHI::PipelineStage::Draw, RHI::PipelineStage::Copy ) );
    }

    void DeviceResourceStates::ReadOnly( DeviceTextureState& texture, TBitFlags<RHI::PipelineStage> sync, TBitFlags<RHI::ResourceAccess> access, RHI::TextureState state )
    {
        EE_ASSERT( texture.m_pTexture );

        ValidateReadOnlyAccess( sync, access );

        EE_ASSERT( state != RHI::TextureState::DepthWrite );
        EE_ASSERT( state != RHI::TextureState::VideoDecodeWrite );
        EE_ASSERT( state != RHI::TextureState::VideoEncodeWrite );
        EE_ASSERT( state != RHI::TextureState::VideoProcessWrite );
        EE_ASSERT( state != RHI::TextureState::RenderTarget );
        EE_ASSERT( state != RHI::TextureState::UnorderedAccess );

        m_pendingTextureStates.emplace_back( &texture, sync, access, state );
    }

    void DeviceResourceStates::Writeable( DeviceTextureState& texture, TBitFlags<RHI::PipelineStage> sync, TBitFlags<RHI::ResourceAccess> access, RHI::TextureState state )
    {
        EE_ASSERT( texture.m_pTexture );

        ValidateWriteableAccess( sync, access );

        m_pendingTextureStates.emplace_back( &texture, sync, access, state );
    }
}
