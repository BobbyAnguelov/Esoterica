#pragma once
#ifdef _WIN32

#include "RenderContext_DX11.h"
#include "Base/Types/Color.h"
#include "Base/Threading/Threading.h"

#include <D3D11.h>

//-------------------------------------------------------------------------

namespace EE { class IniFile; }

//-------------------------------------------------------------------------

namespace EE::Render
{
    class EE_BASE_API RenderDevice
    {

    public:

        RenderDevice() = default;
        ~RenderDevice();

        //-------------------------------------------------------------------------

        bool IsInitialized() const;
        bool Initialize( IniFile const& iniFile );
        bool Initialize();
        void Shutdown();

        inline RenderContext const& GetImmediateContext() const { return m_immediateContext; }
        void PresentFrame();

        // Device locking: required since we create/destroy resources while rendering
        //-------------------------------------------------------------------------

        void LockDevice() { m_deviceMutex.lock(); }
        void UnlockDevice() { m_deviceMutex.unlock(); }

        // Swap Chains
        //-------------------------------------------------------------------------

        RenderTarget const* GetPrimaryWindowRenderTarget() const { return &m_primaryWindow.m_renderTarget; }
        RenderTarget* GetPrimaryWindowRenderTarget() { return &m_primaryWindow.m_renderTarget; }
        inline Int2 GetPrimaryWindowDimensions() const { return m_resolution; }

        void CreateSecondaryRenderWindow( RenderWindow& window, HWND platformWindowHandle );
        void DestroySecondaryRenderWindow( RenderWindow& window );

        void ResizeWindow( RenderWindow& window, Int2 const& dimensions );
        void ResizePrimaryWindowRenderTarget( Int2 const& dimensions );

        // Resource and state management
        //-------------------------------------------------------------------------

        // Shaders
        void CreateShader( Shader& shader );
        void DestroyShader( Shader& shader );

        // Buffers
        void CreateBuffer( RenderBuffer& buffer, void const* pInitializationData = nullptr );
        void ResizeBuffer( RenderBuffer& buffer, uint32_t newSize );
        void DestroyBuffer( RenderBuffer& buffer );

        // Vertex shader input mappings
        void CreateShaderInputBinding( VertexShader const& shader, VertexLayoutDescriptor const& vertexLayoutDesc, ShaderInputBindingHandle& inputBinding );
        void DestroyShaderInputBinding( ShaderInputBindingHandle& inputBinding );

        // Rasterizer
        void CreateRasterizerState( RasterizerState& stateDesc );
        void DestroyRasterizerState( RasterizerState& state );

        void CreateBlendState( BlendState& stateDesc );
        void DestroyBlendState( BlendState& state );

        // Textures and Sampling
        void CreateDataTexture( Texture& texture, TextureFormat format, uint8_t const* rawData, size_t size );
        inline void CreateDataTexture( Texture& texture, TextureFormat format, Blob const& rawData ) { CreateDataTexture( texture, format, rawData.data(), rawData.size() ); }
        void CreateTexture( Texture& texture, DataFormat format, Int2 dimensions, uint32_t usage );
        void DestroyTexture( Texture& texture );

        void CreateSamplerState( SamplerState& state );
        void DestroySamplerState( SamplerState& state );

        // Render Targets
        void CreateRenderTarget( RenderTarget& renderTarget, Int2 const& dimensions, bool createPickingTarget = false );
        void ResizeRenderTarget( RenderTarget& renderTarget, Int2 const& newDimensions );
        void DestroyRenderTarget( RenderTarget& renderTarget );

        // Picking
        PickingID ReadBackPickingID( RenderTarget const& renderTarget, Int2 const& pixelCoords );

    private:

        bool CreateDeviceAndSwapchain();
        void DestroyDeviceAndSwapchain();

        bool CreateWindowRenderTarget( RenderWindow& renderWindow, Int2 dimensions );

        bool CreateDefaultDepthStencilStates();
        void DestroyDefaultDepthStencilStates();

        void CreateRawTexture( Texture& texture, uint8_t const* pRawData, size_t rawDataSize );
        void CreateDDSTexture( Texture& texture, uint8_t const* pRawData, size_t rawDataSize );

    private:

        Int2                        m_resolution = Int2( 1280, 720 );
        float                       m_refreshRate = 60;
        bool                        m_isFullscreen = false;

        // Device Core 
        ID3D11Device*               m_pDevice = nullptr;
        IDXGIFactory*               m_pFactory = nullptr;
        RenderWindow                m_primaryWindow;
        RenderContext               m_immediateContext;

        // Lock to allow loading resources while rendering across different threads
        Threading::RecursiveMutex   m_deviceMutex;
    };
}

#endif