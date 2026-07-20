
#pragma once

#include "Engine/_Module/API.h"
#include "Base/Render/RHI.h"

namespace EE::Render
{
    struct DeviceBufferState final
    {
        RHI::Buffer*                        m_pBuffer = nullptr;
        TBitFlags<RHI::PipelineStage>       m_currentSync = RHI::PipelineStage::All;
        TBitFlags<RHI::ResourceAccess>      m_currentAccess = RHI::ResourceAccess::Common;

        inline DeviceBufferState() = default;
        inline explicit DeviceBufferState( RHI::Buffer* pResource )
        {
            m_pBuffer = pResource;
        }

        inline DeviceBufferState( const DeviceBufferState& ) = default;
        inline DeviceBufferState& operator=( const DeviceBufferState& ) = default;

        inline DeviceBufferState( DeviceBufferState&& other ) noexcept
        {
            m_pBuffer = other.m_pBuffer;
            m_currentSync = other.m_currentSync;
            m_currentAccess = other.m_currentAccess;

            other.m_pBuffer = nullptr;
        }

        inline DeviceBufferState& operator=( DeviceBufferState&& other ) noexcept
        {
            if ( this != &other )
            {
                m_pBuffer = other.m_pBuffer;
                m_currentSync = other.m_currentSync;
                m_currentAccess = other.m_currentAccess;

                other.m_pBuffer = nullptr;
                other.m_currentAccess = {};
                other.m_currentSync = {};
            }
            return *this;
        }

        inline DeviceBufferState& operator=( RHI::Buffer* pResource )
        {
            m_pBuffer = pResource;
            m_currentSync = RHI::PipelineStage::All;
            m_currentAccess = RHI::ResourceAccess::Common;
            return *this;
        }

        inline RHI::Buffer* operator->()
        {
            return m_pBuffer;
        }

        inline RHI::Buffer const* operator->() const
        {
            return m_pBuffer;
        }

        inline operator RHI::Buffer*( ) & noexcept { return m_pBuffer; }
        inline operator RHI::Buffer const*( ) const & noexcept { return m_pBuffer; }
        inline operator RHI::Buffer*&&( ) && noexcept { return std::move( m_pBuffer ); }
    };

    //-------------------------------------------------------------------------------------------------------

    struct DeviceTextureState final
    {
        RHI::Texture*                   m_pTexture = nullptr;

        TBitFlags<RHI::PipelineStage>   m_currentSync = RHI::PipelineStage::All;
        TBitFlags<RHI::ResourceAccess>  m_currentAccess = RHI::ResourceAccess::Common;
        RHI::TextureState               m_currentState = RHI::TextureState::ShaderResource;

        inline DeviceTextureState() = default;
        inline explicit DeviceTextureState( RHI::Texture* pResource )
        {
            m_pTexture = pResource;
            m_currentState = pResource ? pResource->m_initialState : RHI::TextureState::ShaderResource;
        }

        inline DeviceTextureState( const DeviceTextureState& ) = default;
        inline DeviceTextureState& operator=( const DeviceTextureState& ) = default;

        inline DeviceTextureState( DeviceTextureState&& other ) noexcept
        {
            m_pTexture = other.m_pTexture;
            m_currentSync = other.m_currentSync;
            m_currentAccess = other.m_currentAccess;
            m_currentState = other.m_currentState;

            other.m_pTexture = nullptr;
            other.m_currentState = RHI::TextureState::ShaderResource;
        }

        inline DeviceTextureState& operator=( DeviceTextureState&& other ) noexcept
        {
            if ( this != &other )
            {
                m_pTexture = other.m_pTexture;
                m_currentSync = other.m_currentSync;
                m_currentAccess = other.m_currentAccess;
                m_currentState = other.m_currentState;

                other.m_pTexture = nullptr;
                other.m_currentAccess = {};
                other.m_currentSync = {};
                other.m_currentState = RHI::TextureState::ShaderResource;
            }
            return *this;
        }

        inline DeviceTextureState& operator=( RHI::Texture* pResource )
        {
            m_pTexture = pResource;
            m_currentSync = RHI::PipelineStage::All;
            m_currentAccess = RHI::ResourceAccess::Common;
            m_currentState = pResource ? pResource->m_initialState : RHI::TextureState::ShaderResource;
            return *this;
        }

        inline RHI::Texture* operator->()
        {
            return m_pTexture;
        }

        inline RHI::Texture const* operator->() const
        {
            return m_pTexture;
        }

        inline operator RHI::Texture*( ) & noexcept { return m_pTexture; }
        inline operator RHI::Texture const*( ) const & noexcept { return m_pTexture; }
        inline operator RHI::Texture*&&( ) && noexcept { return eastl::move( m_pTexture ); }
    };

    //-------------------------------------------------------------------------------------------------------

    class EE_ENGINE_API DeviceResourceStates final
    {
    public:

        struct Buffer
        {
            DeviceBufferState*                  m_pBufferState = nullptr;
            TBitFlags<RHI::PipelineStage>       m_bufferSync = {};
            TBitFlags<RHI::ResourceAccess>      m_bufferAccess = {};

        };

        struct Texture
        {
            DeviceTextureState*                 m_pTextureState = nullptr;
            TBitFlags<RHI::PipelineStage>       m_textureSync = {};
            TBitFlags<RHI::ResourceAccess>      m_textureAccess = {};
            RHI::TextureState                   m_textureState = {};
        };

    public:

        void FlushBarriers( RHI::CommandBuffer* pCommandBuffer );
        bool HasPendingBarriers() const;

        void ReadOnly( DeviceBufferState& buffer, TBitFlags<RHI::PipelineStage> sync, TBitFlags<RHI::ResourceAccess> access );
        void ReadOnly( DeviceTextureState& texture, TBitFlags<RHI::PipelineStage> sync, TBitFlags<RHI::ResourceAccess> access, RHI::TextureState state );

        void Writeable( DeviceBufferState& buffer, TBitFlags<RHI::PipelineStage> sync, TBitFlags<RHI::ResourceAccess> access );
        void Writeable( DeviceTextureState& texture, TBitFlags<RHI::PipelineStage> sync, TBitFlags<RHI::ResourceAccess> access, RHI::TextureState state );

    private:

        TVector<Buffer>                         m_pendingBufferStates;
        TVector<Texture>                        m_pendingTextureStates;
    };
}
