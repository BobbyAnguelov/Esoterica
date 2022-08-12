#include "RenderDevice_DX11.h"
#include "TextureLoader_Win32.h"
#include "System/Render/RenderDefaultResources.h"
#include "System/IniFile.h"
#include "System/Profiling.h"
#include "System/Log.h"

//-------------------------------------------------------------------------

#define EE_ENABLE_RENDERDEVICE_DEBUG 1

//-------------------------------------------------------------------------

namespace EE::Render
{
    static bool CreateDepthStencilState( ID3D11Device* pDevice, bool enable, bool writeEnable, ID3D11DepthStencilState** ppState )
    {
        EE_ASSERT( pDevice != nullptr );

        D3D11_DEPTH_STENCIL_DESC desc;
        Memory::MemsetZero( &desc, sizeof( desc ) );

        desc.DepthEnable = enable;
        desc.DepthWriteMask = writeEnable ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
        desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

        desc.StencilEnable = false;
        desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
        desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

        desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;

        desc.BackFace = desc.FrontFace;
        return( SUCCEEDED( pDevice->CreateDepthStencilState( &desc, ppState ) ) );
    }

    //-------------------------------------------------------------------------

    RenderDevice::~RenderDevice()
    {
        EE_ASSERT( RenderContext::s_pDepthTestingReadOnly == nullptr );
        EE_ASSERT( RenderContext::s_pDepthTestingOff == nullptr );
        EE_ASSERT( RenderContext::s_pDepthTestingOn == nullptr );
        EE_ASSERT( !m_immediateContext.IsValid() );

        EE_ASSERT( !m_primaryWindow.IsValid() );
        EE_ASSERT( !m_primaryWindow.m_renderTarget.IsValid() );
        EE_ASSERT( m_pDevice == nullptr );
    }

    bool RenderDevice::IsInitialized() const
    {
        return m_pDevice != nullptr;
    }

    bool RenderDevice::Initialize( IniFile const& iniFile )
    {
        EE_ASSERT( iniFile.IsValid() );

        m_resolution.m_x = iniFile.GetIntOrDefault( "Render:ResolutionX", 1280 );
        m_resolution.m_y = iniFile.GetIntOrDefault( "Render:ResolutionX", 720 );
        m_refreshRate = iniFile.GetFloatOrDefault( "Render:ResolutionX", 60 );
        m_isFullscreen = iniFile.GetBoolOrDefault( "Render:Fullscreen", false );

        //-------------------------------------------------------------------------

        if ( m_resolution.m_x < 0 || m_resolution.m_y < 0 || m_refreshRate < 0 )
        {
            EE_LOG_ERROR( "Render", "Render Device", "Invalid render settings read from ini file." );
            return false;
        }

        return Initialize();
    }

    bool RenderDevice::Initialize()
    {
        if ( !CreateDeviceAndSwapchain() )
        {
            return false;
        }

        if ( !CreateDefaultDepthStencilStates() )
        {
            return false;
        }

        // Set OM default state
        m_immediateContext.SetRenderTarget( m_primaryWindow.m_renderTarget );
        m_immediateContext.ClearRenderTargetViews( m_primaryWindow.m_renderTarget );
        m_immediateContext.m_pDeviceContext->OMSetDepthStencilState( RenderContext::s_pDepthTestingOn, 0 );

        DefaultResources::Initialize( this );

        return true;
    }

    void RenderDevice::Shutdown()
    {
        DefaultResources::Shutdown( this );

        m_immediateContext.m_pDeviceContext->ClearState();
        m_immediateContext.m_pDeviceContext->Flush();

        DestroyDefaultDepthStencilStates();
        DestroyDeviceAndSwapchain();
    }

    //-------------------------------------------------------------------------

    bool RenderDevice::CreateDeviceAndSwapchain()
    {
        DXGI_SWAP_CHAIN_DESC swapChainDesc;
        EE::Memory::MemsetZero( &swapChainDesc, sizeof( swapChainDesc ) );

        HWND pActiveWindow = GetActiveWindow();
        if ( pActiveWindow == nullptr )
        {
            return false;
        }

        // Set buffer dimensions and format
        swapChainDesc.BufferCount = 2;
        swapChainDesc.BufferDesc.Width = m_resolution.m_x;
        swapChainDesc.BufferDesc.Height = m_resolution.m_y;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferDesc.RefreshRate.Numerator = (uint32_t) m_refreshRate;
        swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        swapChainDesc.OutputWindow = pActiveWindow;
        swapChainDesc.Windowed = !m_isFullscreen;

        // Set debug flags on D3D device in debug build
        UINT flags = 0;
        #if EE_ENABLE_RENDERDEVICE_DEBUG
        flags |= D3D10_CREATE_DEVICE_DEBUG;
        #endif

        // Setup D3D feature levels
        D3D_FEATURE_LEVEL featureLevelsRequested[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
        D3D_FEATURE_LEVEL featureLevelSupported;
        uint32_t const numLevelsRequested = 2;

        // Create the D3D device and swap chain
        IDXGISwapChain* pSwapChain = nullptr;
        HRESULT result = D3D11CreateDeviceAndSwapChain( nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, featureLevelsRequested, numLevelsRequested, D3D11_SDK_VERSION, &swapChainDesc, &pSwapChain, &m_pDevice, &featureLevelSupported, &m_immediateContext.m_pDeviceContext );
        if ( FAILED( result ) )
        {
            if ( m_immediateContext.m_pDeviceContext != nullptr )
            {
                m_immediateContext.m_pDeviceContext->Release();
                m_immediateContext.m_pDeviceContext = nullptr;
            }

            if ( pSwapChain != nullptr )
            {
                pSwapChain->Release();
            }

            EE_LOG_ERROR( "Rendering", "Render Device", "Device Creation Failed" );
            return false;
        }

        m_primaryWindow.m_pSwapChain = pSwapChain;
        CreateWindowRenderTarget( m_primaryWindow, m_resolution );

        //-------------------------------------------------------------------------

        IDXGIDevice* pDXGIDevice = nullptr;
        IDXGIAdapter* pDXGIAdapter = nullptr;

        if ( FAILED( m_pDevice->QueryInterface( IID_PPV_ARGS( &pDXGIDevice ) ) ) )
        {
            EE_HALT();
        }

        if ( FAILED( pDXGIDevice->GetParent( IID_PPV_ARGS( &pDXGIAdapter ) ) ) )
        {
            EE_HALT();
        }

        if ( FAILED( pDXGIAdapter->GetParent( IID_PPV_ARGS( &m_pFactory ) ) ) )
        {
            EE_HALT();
        }

        pDXGIAdapter->Release();
        pDXGIDevice->Release();

        return true;
    }

    void RenderDevice::DestroyDeviceAndSwapchain()
    {
        DestroyRenderTarget( m_primaryWindow.m_renderTarget );

        if ( m_primaryWindow.m_pSwapChain != nullptr )
        {
            reinterpret_cast<IDXGISwapChain*>( m_primaryWindow.m_pSwapChain )->Release();
            m_primaryWindow.m_pSwapChain = nullptr;
        }

        if ( m_immediateContext.m_pDeviceContext != nullptr )
        {
            m_immediateContext.m_pDeviceContext->Release();
            m_immediateContext.m_pDeviceContext = nullptr;
        }

        //-------------------------------------------------------------------------

        m_pFactory->Release();

        if ( m_pDevice != nullptr )
        {
            #if EE_ENABLE_RENDERDEVICE_DEBUG
            ID3D11Debug* pDebug = nullptr;
            m_pDevice->QueryInterface( IID_PPV_ARGS( &pDebug ) );
            #endif

            m_pDevice->Release();
            m_pDevice = nullptr;

            #if EE_ENABLE_RENDERDEVICE_DEBUG
            if ( pDebug != nullptr )
            {
                pDebug->ReportLiveDeviceObjects( D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL );
                pDebug->Release();
            }
            #endif
        }
    }

    bool RenderDevice::CreateDefaultDepthStencilStates()
    {
        // Create the three depth testing modes
        if ( !CreateDepthStencilState( m_pDevice, true, true, &RenderContext::s_pDepthTestingOn ) )
        {
            EE_LOG_ERROR( "Rendering", "Render Device", "Depth stencil state creation failed" );
            return false;
        }

        if ( !CreateDepthStencilState( m_pDevice, false, false, &RenderContext::s_pDepthTestingOff ) )
        {
            EE_LOG_ERROR( "Rendering", "Render Device", "Depth stencil state creation failed" );
            return false;
        }

        if ( !CreateDepthStencilState( m_pDevice, true, false, &RenderContext::s_pDepthTestingReadOnly ) )
        {
            EE_LOG_ERROR( "Rendering", "Render Device", "Depth stencil state creation failed" );
            return false;
        }

        return true;
    }

    void RenderDevice::DestroyDefaultDepthStencilStates()
    {
        if ( RenderContext::s_pDepthTestingOn != nullptr )
        {
            RenderContext::s_pDepthTestingOn->Release();
            RenderContext::s_pDepthTestingOn = nullptr;
        }

        if ( RenderContext::s_pDepthTestingOff != nullptr )
        {
            RenderContext::s_pDepthTestingOff->Release();
            RenderContext::s_pDepthTestingOff = nullptr;
        }

        if ( RenderContext::s_pDepthTestingReadOnly != nullptr )
        {
            RenderContext::s_pDepthTestingReadOnly->Release();
            RenderContext::s_pDepthTestingReadOnly = nullptr;
        }
    }

    //-------------------------------------------------------------------------

    void RenderDevice::PresentFrame()
    {
        EE_PROFILE_FUNCTION_RENDER();

        EE_ASSERT( IsInitialized() );

        // Show rendered frame, and clear buffers
        m_immediateContext.Present( m_primaryWindow );
        m_immediateContext.SetRenderTarget( m_primaryWindow.m_renderTarget );
        m_immediateContext.ClearRenderTargetViews( m_primaryWindow.m_renderTarget );
    }

    void RenderDevice::ResizePrimaryWindowRenderTarget( Int2 const& dimensions )
    {
        EE_ASSERT( dimensions.m_x > 0 && dimensions.m_y > 0 );
        ResizeWindow( m_primaryWindow, dimensions );
        m_immediateContext.SetRenderTarget( m_primaryWindow.m_renderTarget );
        m_immediateContext.ClearRenderTargetViews( m_primaryWindow.m_renderTarget );
        m_resolution = dimensions;
    }

    //-------------------------------------------------------------------------

    void RenderDevice::CreateSecondaryRenderWindow( RenderWindow& window, HWND platformWindowHandle )
    {
        EE_ASSERT( platformWindowHandle != 0 );

        RECT rect;
        ::GetClientRect( platformWindowHandle, &rect );

        DXGI_SWAP_CHAIN_DESC sd;
        ZeroMemory( &sd, sizeof( sd ) );
        sd.BufferDesc.Width = rect.right - rect.left;
        sd.BufferDesc.Height = rect.bottom - rect.top;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.BufferCount = 2;
        sd.OutputWindow = platformWindowHandle;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        sd.Flags = 0;

        m_immediateContext.m_pDeviceContext->ClearState();
        m_immediateContext.m_pDeviceContext->Flush();

        IDXGISwapChain* pSwapChain = nullptr;
        m_pFactory->CreateSwapChain( m_pDevice, &sd, &pSwapChain );
        EE_ASSERT( pSwapChain != nullptr );

        window.m_pSwapChain = pSwapChain;
        CreateWindowRenderTarget( window, Int2( sd.BufferDesc.Width, sd.BufferDesc.Height ) );
    }

    void RenderDevice::DestroySecondaryRenderWindow( RenderWindow& window )
    {
        EE_ASSERT( window.IsValid() );

        DestroyRenderTarget( window.m_renderTarget );

        //-------------------------------------------------------------------------

        auto pSC = reinterpret_cast<IDXGISwapChain*>( m_primaryWindow.m_pSwapChain );
        pSC->Release();
        window.m_pSwapChain = nullptr;

        m_immediateContext.m_pDeviceContext->ClearState();
        m_immediateContext.m_pDeviceContext->Flush();
    }

    bool RenderDevice::CreateWindowRenderTarget( RenderWindow& window, Int2 dimensions )
    {
        EE_ASSERT( window.m_pSwapChain != nullptr );

        // Get a render target view for the back buffer
        //-------------------------------------------------------------------------

        ID3D11Texture2D* pBackBuffer = nullptr;
        if ( FAILED( reinterpret_cast<IDXGISwapChain*>( window.m_pSwapChain )->GetBuffer( 0, __uuidof( ID3D11Texture2D ), (LPVOID*) &pBackBuffer ) ) )
        {
            EE_LOG_ERROR( "Rendering", "Render Device", "Failed to get back buffer texture resource" );
            return false;
        }

        ID3D11RenderTargetView* pRenderTargetView = nullptr;
        HRESULT result = m_pDevice->CreateRenderTargetView( pBackBuffer, nullptr, &pRenderTargetView );

        if ( FAILED( result ) )
        {
            EE_LOG_ERROR( "Rendering", "Render Device", "Failed to create render target" );
            return false;
        }

        //-------------------------------------------------------------------------

        window.m_renderTarget.m_RT.m_textureHandle.m_pData = pBackBuffer;
        window.m_renderTarget.m_RT.m_renderTargetView.m_pData = pRenderTargetView;
        window.m_renderTarget.m_RT.m_dimensions = dimensions;
        CreateTexture( window.m_renderTarget.m_DS, DataFormat::Float_X32, dimensions, USAGE_RT_DS ) ;

        return true;
    }

    void RenderDevice::ResizeWindow( RenderWindow& window, Int2 const& dimensions )
    {
        EE_ASSERT( window.IsValid() );
        auto pSC = reinterpret_cast<IDXGISwapChain*>( window.m_pSwapChain );

        // Release render target and depth stencil
        m_immediateContext.SetRenderTarget( nullptr );
        DestroyRenderTarget( window.m_renderTarget );
        m_immediateContext.m_pDeviceContext->Flush();

        // Update buffer sizes
        if ( FAILED( reinterpret_cast<IDXGISwapChain*>( window.m_pSwapChain )->ResizeBuffers( 2, dimensions.m_x, dimensions.m_y, DXGI_FORMAT_UNKNOWN, 0 ) ) )
        {
            EE_LOG_ERROR( "Rendering", "Render Device", "Failed to resize swap chain buffers" );
            EE_HALT();
        }

        if ( !CreateWindowRenderTarget( window, dimensions ) )
        {
            EE_LOG_ERROR( "Rendering", "Render Device", "Failed to create render targets/depth stencil view" );
            EE_HALT();
        }
    }

    //-------------------------------------------------------------------------

    void RenderDevice::CreateShader( Shader& shader )
    {
        EE_ASSERT( IsInitialized() && !shader.IsValid() );

        if ( shader.GetPipelineStage() == PipelineStage::Vertex )
        {
            ID3D11VertexShader* pVertexShader;
            if ( SUCCEEDED( m_pDevice->CreateVertexShader( &shader.m_byteCode[0], shader.m_byteCode.size(), nullptr, &pVertexShader ) ) )
            {
                shader.m_shaderHandle.m_pData = pVertexShader;
            }
        }
        else if ( shader.GetPipelineStage() == PipelineStage::Pixel )
        {
            ID3D11PixelShader* pPixelShader;
            if ( SUCCEEDED( m_pDevice->CreatePixelShader( &shader.m_byteCode[0], shader.m_byteCode.size(), nullptr, &pPixelShader ) ) )
            {
                shader.m_shaderHandle.m_pData = pPixelShader;
            }
        }
        else if ( shader.GetPipelineStage() == PipelineStage::Geometry )
        {
            ID3D11GeometryShader* pGeometryShader;
            if ( SUCCEEDED( m_pDevice->CreateGeometryShader( &shader.m_byteCode[0], shader.m_byteCode.size(), nullptr, &pGeometryShader ) ) )
            {
                shader.m_shaderHandle.m_pData = pGeometryShader;
            }
        }
        else if ( shader.GetPipelineStage() == PipelineStage::Compute )
        {
            ID3D11ComputeShader* pComputeShader;
            if ( SUCCEEDED( m_pDevice->CreateComputeShader( &shader.m_byteCode[0], shader.m_byteCode.size(), nullptr, &pComputeShader ) ) )
            {
                shader.m_shaderHandle.m_pData = pComputeShader;
            }
        }
        else //  Hull / Compute / etc...
        {
            EE_UNIMPLEMENTED_FUNCTION();
        }

        // Create buffers const for shader
        for ( auto& cbuffer : shader.m_cbuffers )
        {
            CreateBuffer( cbuffer );
            EE_ASSERT( cbuffer.IsValid() );
        }

        EE_ASSERT( shader.IsValid() );
    }

    void RenderDevice::DestroyShader( Shader& shader )
    {
        EE_ASSERT( IsInitialized() && shader.IsValid() && shader.GetPipelineStage() != PipelineStage::None );

        if ( shader.GetPipelineStage() == PipelineStage::Vertex )
        {
            ( (ID3D11VertexShader*) shader.m_shaderHandle.m_pData )->Release();
        }
        else if ( shader.GetPipelineStage() == PipelineStage::Pixel )
        {
            ( (ID3D11PixelShader*) shader.m_shaderHandle.m_pData )->Release();
        }
        else if ( shader.GetPipelineStage() == PipelineStage::Geometry )
        {
            ( (ID3D11GeometryShader*) shader.m_shaderHandle.m_pData )->Release();
        }
        else if ( shader.GetPipelineStage() == PipelineStage::Hull )
        {
            ( (ID3D11HullShader*) shader.m_shaderHandle.m_pData )->Release();
        }
        else if ( shader.GetPipelineStage() == PipelineStage::Compute )
        {
            ( (ID3D11ComputeShader*) shader.m_shaderHandle.m_pData )->Release();
        }

        shader.m_shaderHandle.Reset();

        for ( auto& cbuffer : shader.m_cbuffers )
        {
            DestroyBuffer( cbuffer );
        }
        shader.m_cbuffers.clear();
    }

    //-------------------------------------------------------------------------

    void RenderDevice::CreateBuffer( RenderBuffer& buffer, void const* pInitializationData )
    {
        EE_ASSERT( IsInitialized() && !buffer.IsValid() );

        D3D11_BUFFER_DESC bufferDesc;
        EE::Memory::MemsetZero( &bufferDesc, sizeof( D3D11_BUFFER_DESC ) );
        bufferDesc.ByteWidth = buffer.m_byteSize;
        bufferDesc.StructureByteStride = buffer.m_byteStride;

        switch ( buffer.m_usage )
        {
            case RenderBuffer::Usage::CPU_and_GPU:
            bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
            bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            break;

            case RenderBuffer::Usage::GPU_only:
            bufferDesc.Usage = D3D11_USAGE_DEFAULT;
            break;

            default:
            bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
            break;
        }

        switch ( buffer.m_type )
        {
            case RenderBuffer::Type::Constant:
            bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            break;

            case RenderBuffer::Type::Vertex:
            bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            break;

            case RenderBuffer::Type::Index:
            bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
            EE_ASSERT( buffer.m_byteStride == 2 || buffer.m_byteStride == 4 ); // only 16/32 bit indices support
            break;

            default:
            EE_HALT();
        }

        // If there is initialization data set it
        D3D11_SUBRESOURCE_DATA initData;
        if ( pInitializationData != nullptr )
        {
            EE::Memory::MemsetZero( &initData, sizeof( D3D11_SUBRESOURCE_DATA ) );
            initData.pSysMem = pInitializationData;
        }

        // Create and store buffer
        m_pDevice->CreateBuffer( &bufferDesc, pInitializationData == nullptr ? nullptr : &initData, (ID3D11Buffer**) &buffer.m_resourceHandle.m_pData );
        EE_ASSERT( buffer.IsValid() );
    }

    void RenderDevice::ResizeBuffer( RenderBuffer& buffer, uint32_t newSize )
    {
        EE_ASSERT( buffer.IsValid() && newSize % buffer.m_byteStride == 0 );

        // Release D3D buffer
        ( (ID3D11Buffer*) buffer.m_resourceHandle.m_pData )->Release();
        buffer.m_resourceHandle.m_pData = nullptr;
        buffer.m_byteSize = newSize;
        CreateBuffer( buffer );
    }

    void RenderDevice::DestroyBuffer( RenderBuffer& buffer )
    {
        EE_ASSERT( IsInitialized() );

        if ( buffer.IsValid() )
        {
            ( (ID3D11Buffer*) buffer.m_resourceHandle.m_pData )->Release();
            buffer.m_resourceHandle.Reset();
            buffer = RenderBuffer();
        }
    }

    //-------------------------------------------------------------------------

    namespace DX11
    {
        static char const* const g_semanticNames[] = { "POSITION", "NORMAL", "TANGENT", "BINORMAL", "COLOR", "TEXCOORD", "BLENDINDICES", "BLENDWEIGHTS" };

        EE_FORCE_INLINE static char const* GetNameForSemantic( DataSemantic semantic )
        {
            EE_ASSERT( semantic < DataSemantic::None );
            return g_semanticNames[(uint8_t) semantic];
        }

        static const DXGI_FORMAT g_formatConversion[(size_t) DataFormat::Count] =
        {
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_R8_UINT,
            DXGI_FORMAT_R8G8_UINT,
            DXGI_FORMAT_R8G8B8A8_UINT,

            DXGI_FORMAT_R8_UNORM,
            DXGI_FORMAT_R8G8_UNORM,
            DXGI_FORMAT_R8G8B8A8_UNORM,

            DXGI_FORMAT_R32_UINT,
            DXGI_FORMAT_R32G32_UINT,
            DXGI_FORMAT_R32G32B32_UINT,
            DXGI_FORMAT_R32G32B32A32_UINT,

            DXGI_FORMAT_R32_SINT,
            DXGI_FORMAT_R32G32_SINT,
            DXGI_FORMAT_R32G32B32_SINT,
            DXGI_FORMAT_R32G32B32A32_SINT,

            DXGI_FORMAT_R16_FLOAT,
            DXGI_FORMAT_R16G16_FLOAT,
            DXGI_FORMAT_R16G16B16A16_FLOAT,

            DXGI_FORMAT_R32_FLOAT,
            DXGI_FORMAT_R32G32_FLOAT,
            DXGI_FORMAT_R32G32B32_FLOAT,
            DXGI_FORMAT_R32G32B32A32_FLOAT,

            DXGI_FORMAT_R32_TYPELESS
        };

        EE_FORCE_INLINE static DXGI_FORMAT GetDXGIFormat( DataFormat format  )
        {
            EE_ASSERT( format < DataFormat::Count );
            return g_formatConversion[(uint8_t) format];
        }

        EE_FORCE_INLINE static DXGI_FORMAT GetDXGIFormat_SRV_UAV( DataFormat format )
        {
            EE_ASSERT( format < DataFormat::Count );

            if ( format == DataFormat::Float_X32 )
            {
                return DXGI_FORMAT_R32_FLOAT;
            }

            return g_formatConversion[(uint8_t) format];
        }

        EE_FORCE_INLINE static DXGI_FORMAT GetDXGIFormat_RT_DS( DataFormat format )
        {
            EE_ASSERT( format < DataFormat::Count );

            if ( format == DataFormat::Float_X32 )
            {
                return DXGI_FORMAT_D32_FLOAT;
            }

            return g_formatConversion[(uint8_t) format];
        }

        EE_FORCE_INLINE static bool IsDepthStencilFormat( DataFormat format )
        {
            return format == DataFormat::Float_X32;
        }
    }

    void RenderDevice::CreateShaderInputBinding( VertexShader const& shader, VertexLayoutDescriptor const& vertexLayoutDesc, ShaderInputBindingHandle& inputBinding )
    {
        EE_ASSERT( IsInitialized() && shader.GetPipelineStage() == PipelineStage::Vertex && vertexLayoutDesc.IsValid() );

        bool bindingSucceeded = true;
        TVector<D3D11_INPUT_ELEMENT_DESC> inputLayout;
        auto const& shaderVertexLayoutDesc = shader.GetVertexLayoutDesc();
        for ( auto const& shaderVertexElementDesc : shaderVertexLayoutDesc.m_elementDescriptors )
        {
            bool inputBound = false;
            for ( auto const& vertexElement : vertexLayoutDesc.m_elementDescriptors )
            {
                if ( shaderVertexElementDesc.m_semantic == vertexElement.m_semantic && shaderVertexElementDesc.m_semanticIndex == vertexElement.m_semanticIndex )
                {
                    D3D11_INPUT_ELEMENT_DESC elementDesc;
                    elementDesc.SemanticName = DX11::GetNameForSemantic( shaderVertexElementDesc.m_semantic );
                    elementDesc.SemanticIndex = shaderVertexElementDesc.m_semanticIndex;
                    elementDesc.Format = DX11::GetDXGIFormat( shaderVertexElementDesc.m_format );
                    elementDesc.InputSlot = 0;
                    elementDesc.AlignedByteOffset = vertexElement.m_offset;
                    elementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
                    elementDesc.InstanceDataStepRate = 0;
                    inputLayout.push_back( elementDesc );

                    inputBound = true;
                    break;
                }
            }

            if ( !inputBound )
            {
                bindingSucceeded = false;
                break;
            }
        }

        // Try to create Input Layout
        ID3D11InputLayout* pInputLayout = nullptr;
        if ( bindingSucceeded )
        {
            m_pDevice->CreateInputLayout( &inputLayout[0], (UINT) inputLayout.size(), &shader.m_byteCode[0], shader.m_byteCode.size(), &pInputLayout );
            inputBinding.m_pData = pInputLayout;
        }
    }

    void RenderDevice::DestroyShaderInputBinding( ShaderInputBindingHandle& inputBinding )
    {
        EE_ASSERT( IsInitialized() && inputBinding.IsValid() );
        ( (ID3D11InputLayout*) inputBinding.m_pData )->Release();
        inputBinding.Reset();
    }

    //-------------------------------------------------------------------------

    void RenderDevice::CreateRasterizerState( RasterizerState& state )
    {
        EE_ASSERT( IsInitialized() && !state.IsValid() );

        D3D11_RASTERIZER_DESC rdesc;
        EE::Memory::MemsetZero( &rdesc, sizeof( D3D11_RASTERIZER_DESC ) );
        rdesc.FrontCounterClockwise = true;
        rdesc.DepthClipEnable = true;

        switch ( state.m_cullMode )
        {
            case CullMode::None: rdesc.CullMode = D3D11_CULL_NONE; break;
            case CullMode::FrontFace: rdesc.CullMode = D3D11_CULL_FRONT; break;
            case CullMode::BackFace: rdesc.CullMode = D3D11_CULL_BACK; break;
        }

        switch ( state.m_fillMode )
        {
            case FillMode::Solid: rdesc.FillMode = D3D11_FILL_SOLID; break;
            case FillMode::Wireframe: rdesc.FillMode = D3D11_FILL_WIREFRAME; break;
        }

        rdesc.ScissorEnable = state.m_scissorCulling;
        auto result = m_pDevice->CreateRasterizerState( &rdesc, (ID3D11RasterizerState**) &state.m_resourceHandle.m_pData );
        EE_ASSERT( SUCCEEDED( result ) );
    }

    void RenderDevice::DestroyRasterizerState( RasterizerState& state )
    {
        EE_ASSERT( IsInitialized() );
        EE_ASSERT( state.IsValid() );
        ( (ID3D11RasterizerState*) state.m_resourceHandle.m_pData )->Release();
        state.m_resourceHandle.Reset();
    }

    //-------------------------------------------------------------------------

    namespace DX11
    {
        static D3D11_BLEND GetBlendValue( BlendValue value )
        {
            switch ( value )
            {
                case BlendValue::Zero: return D3D11_BLEND_ZERO; break;
                case BlendValue::One: return D3D11_BLEND_ONE; break;
                case BlendValue::SourceColor: return D3D11_BLEND_SRC_COLOR; break;
                case BlendValue::InverseSourceColor: return D3D11_BLEND_INV_SRC_COLOR; break;
                case BlendValue::SourceAlpha: return D3D11_BLEND_SRC_ALPHA; break;
                case BlendValue::InverseSourceAlpha: return D3D11_BLEND_INV_SRC_ALPHA; break;
                case BlendValue::DestinationAlpha: return D3D11_BLEND_DEST_ALPHA; break;
                case BlendValue::InverseDestinationAlpha: return D3D11_BLEND_INV_DEST_ALPHA; break;
                case BlendValue::DestinationColor: return D3D11_BLEND_DEST_COLOR; break;
                case BlendValue::InverseDestinationColor: return D3D11_BLEND_INV_DEST_COLOR; break;
                case BlendValue::SourceAlphaSaturated: return D3D11_BLEND_SRC_ALPHA_SAT; break;
                case BlendValue::BlendFactor: return D3D11_BLEND_BLEND_FACTOR; break;
                case BlendValue::InverseBlendFactor: return D3D11_BLEND_INV_BLEND_FACTOR; break;
                case BlendValue::Source1Color: return D3D11_BLEND_SRC1_COLOR; break;
                case BlendValue::InverseSource1Color: return D3D11_BLEND_INV_SRC1_COLOR; break;
                case BlendValue::Source1Alpha: return D3D11_BLEND_SRC1_ALPHA; break;
                case BlendValue::InverseSource1Alpha: return D3D11_BLEND_INV_SRC1_ALPHA; break;
            }

            return D3D11_BLEND_ZERO;
        }

        static D3D11_BLEND_OP GetBlendOp( BlendOp op )
        {
            switch ( op )
            {
                case BlendOp::Add: return D3D11_BLEND_OP_ADD; break;
                case BlendOp::SourceMinusDestination: return D3D11_BLEND_OP_SUBTRACT; break;
                case BlendOp::DestinationMinusSource: return D3D11_BLEND_OP_REV_SUBTRACT; break;
                case BlendOp::Min: return D3D11_BLEND_OP_MIN; break;
                case BlendOp::Max: return D3D11_BLEND_OP_MAX; break;
            }

            return D3D11_BLEND_OP_ADD;
        }
    }

    void RenderDevice::CreateBlendState( BlendState& state )
    {
        EE_ASSERT( IsInitialized() );

        D3D11_BLEND_DESC blendDesc;
        EE::Memory::MemsetZero( &blendDesc, sizeof( D3D11_BLEND_DESC ) );

        blendDesc.AlphaToCoverageEnable = false;
        blendDesc.IndependentBlendEnable = true;

        blendDesc.RenderTarget[0].BlendEnable = state.m_blendEnable;
        blendDesc.RenderTarget[0].SrcBlend = DX11::GetBlendValue( state.m_srcValue );
        blendDesc.RenderTarget[0].DestBlend = DX11::GetBlendValue( state.m_dstValue );
        blendDesc.RenderTarget[0].BlendOp = DX11::GetBlendOp( state.m_blendOp );
        blendDesc.RenderTarget[0].SrcBlendAlpha = DX11::GetBlendValue( state.m_srcAlphaValue );
        blendDesc.RenderTarget[0].DestBlendAlpha = DX11::GetBlendValue( state.m_dstAlphaValue );
        blendDesc.RenderTarget[0].BlendOpAlpha = DX11::GetBlendOp( state.m_blendOpAlpha );
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        for ( auto i = 1; i < 8; i++ )
        {
            blendDesc.RenderTarget[i].BlendEnable = false;
            blendDesc.RenderTarget[i].SrcBlend = D3D11_BLEND_ONE;
            blendDesc.RenderTarget[i].DestBlend = D3D11_BLEND_ZERO;
            blendDesc.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
            blendDesc.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
            blendDesc.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_ZERO;
            blendDesc.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
            blendDesc.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        }

        m_pDevice->CreateBlendState( &blendDesc, (ID3D11BlendState**) &state.m_resourceHandle.m_pData );
        EE_ASSERT( state.IsValid() );
    }

    void RenderDevice::DestroyBlendState( BlendState& state )
    {
        EE_ASSERT( IsInitialized() && state.IsValid() );
        ( (ID3D11BlendState*) state.m_resourceHandle.m_pData )->Release();
        state.m_resourceHandle.Reset();
    }

    //-------------------------------------------------------------------------

    void RenderDevice::CreateDataTexture( Texture& texture, TextureFormat format, uint8_t const* pRawData, size_t rawDataSize )
    {
        switch ( format )
        {
            case TextureFormat::Raw: CreateRawTexture( texture, pRawData, rawDataSize ); break;
            case TextureFormat::DDS: CreateDDSTexture( texture, pRawData, rawDataSize ); break;
        }
    }

    void RenderDevice::CreateRawTexture( Texture& texture, uint8_t const* pRawData, size_t rawDataSize )
    {
        EE_ASSERT( IsInitialized() && !texture.IsValid() );

        // Create texture
        //-------------------------------------------------------------------------

        D3D11_TEXTURE2D_DESC texDesc;
        Memory::MemsetZero( &texDesc, sizeof( texDesc ) );
        texDesc.Width = texture.m_dimensions.m_x;
        texDesc.Height = texture.m_dimensions.m_y;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        texDesc.CPUAccessFlags = 0;

        ID3D11Texture2D* pTexture = nullptr;
        D3D11_SUBRESOURCE_DATA subResource;
        subResource.pSysMem = pRawData;
        subResource.SysMemPitch = texDesc.Width * 4;
        subResource.SysMemSlicePitch = 0;
        if ( m_pDevice->CreateTexture2D( &texDesc, &subResource, &pTexture ) != S_OK )
        {
            EE_HALT();
            return;
        }

        // Create texture view
        //-------------------------------------------------------------------------

        ID3D11ShaderResourceView* pTextureSRV = nullptr;

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        Memory::MemsetZero( &srvDesc, sizeof( srvDesc ) );
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = texDesc.MipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;

        if ( m_pDevice->CreateShaderResourceView( pTexture, &srvDesc, &pTextureSRV ) != S_OK )
        {
            EE_HALT();
            return;
        }

        // Update handle
        //-------------------------------------------------------------------------

        texture.m_textureHandle.m_pData = pTexture;
        texture.m_shaderResourceView.m_pData = pTextureSRV;
    }

    void RenderDevice::CreateDDSTexture( Texture& texture, uint8_t const* pRawData, size_t rawDataSize )
    {
        EE_ASSERT( IsInitialized() && !texture.IsValid() );

        ID3D11Resource* pResource = nullptr;
        ID3D11ShaderResourceView* pTextureSRV = nullptr;

        if ( DirectX::CreateDDSTextureFromMemory( m_pDevice, m_immediateContext.m_pDeviceContext, pRawData, rawDataSize, &pResource, &pTextureSRV ) != S_OK )
        {
            EE_HALT();
            return;
        }

        // Get texture desc to read dimensions
        auto pTexture = (ID3D11Texture2D*) pResource;
        D3D11_TEXTURE2D_DESC textureDesc;
        pTexture->GetDesc( &textureDesc );
        texture.m_dimensions = Int2( textureDesc.Width, textureDesc.Height );

        // Update handle
        texture.m_textureHandle.m_pData = pTexture;
        texture.m_shaderResourceView.m_pData = pTextureSRV;
    }

    void RenderDevice::CreateTexture( Texture& texture, DataFormat format, Int2 dimensions, uint32_t usage )
    {
        EE_ASSERT( IsInitialized() && !texture.IsValid() );

        texture.m_dimensions = dimensions;

        // Create texture resource
        //-------------------------------------------------------------------------

        D3D11_TEXTURE2D_DESC texDesc;
        Memory::MemsetZero( &texDesc, sizeof( texDesc ) );
        texDesc.Width = dimensions.m_x;
        texDesc.Height = dimensions.m_y;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DX11::GetDXGIFormat( format );
        texDesc.SampleDesc.Count = 1;
        texDesc.SampleDesc.Quality = 0;
        texDesc.Usage = D3D11_USAGE_DEFAULT;

        // Set usage flags
        if ( usage & USAGE_STAGING )
        {
            texDesc.Usage = D3D11_USAGE_STAGING;
            texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        }

        // Set bind flags
        texDesc.BindFlags = 0;
        texDesc.BindFlags |= ( usage & USAGE_SRV ) ? D3D11_BIND_SHADER_RESOURCE : 0;
        texDesc.BindFlags |= ( usage & USAGE_UAV ) ? D3D11_BIND_UNORDERED_ACCESS : 0;
        if ( usage & USAGE_RT_DS )
        {
            texDesc.BindFlags |= DX11::IsDepthStencilFormat( format ) ? D3D11_BIND_DEPTH_STENCIL : D3D11_BIND_RENDER_TARGET;
        }

        // Create actual texture
        ID3D11Texture2D* pTexture = nullptr;
        if ( m_pDevice->CreateTexture2D( &texDesc, nullptr, &pTexture ) != S_OK )
        {
            EE_HALT();
            return;
        }

        // Create texture views
        //-------------------------------------------------------------------------

        ID3D11ShaderResourceView* pTextureSRV = nullptr;

        if ( usage & USAGE_SRV )
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
            Memory::MemsetZero( &srvDesc, sizeof( srvDesc ) );
            srvDesc.Format = DX11::GetDXGIFormat_SRV_UAV( format );
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = texDesc.MipLevels;
            srvDesc.Texture2D.MostDetailedMip = 0;

            if ( m_pDevice->CreateShaderResourceView( pTexture, &srvDesc, &pTextureSRV ) != S_OK )
            {
                EE_HALT();
                return;
            }
        }

        ID3D11UnorderedAccessView* pTextureUAV = nullptr;

        if ( usage & USAGE_UAV )
        {
            D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
            Memory::MemsetZero( &uavDesc, sizeof( uavDesc ) );
            uavDesc.Format = DX11::GetDXGIFormat_SRV_UAV( format );
            uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
            uavDesc.Texture2D.MipSlice = 0;

            if ( m_pDevice->CreateUnorderedAccessView( pTexture, &uavDesc, &pTextureUAV ) != S_OK )
            {
                EE_HALT();
                return;
            }
        }

        ID3D11DepthStencilView* pDSV = nullptr;
        ID3D11RenderTargetView* pRTV = nullptr;
        if ( usage & USAGE_RT_DS )
        {
            // Create the depth stencil view
            if ( DX11::IsDepthStencilFormat( format ) )
            {
                D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
                descDSV.Format = DX11::GetDXGIFormat_RT_DS( format );
                descDSV.Flags = 0;
                descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                descDSV.Texture2D.MipSlice = 0;

                if ( m_pDevice->CreateDepthStencilView( pTexture, &descDSV, &pDSV ) != S_OK )
                {
                    EE_HALT();
                    return;
                }
            }
            else // Create as render target
            {
                if ( m_pDevice->CreateRenderTargetView( pTexture, nullptr, &pRTV ) != S_OK )
                {
                    EE_HALT();
                    return;
                }
            }
        }

        // Update handle
        //-------------------------------------------------------------------------

        texture.m_textureHandle.m_pData = pTexture;
        texture.m_shaderResourceView.m_pData = pTextureSRV;
        texture.m_unorderedAccessView.m_pData = pTextureUAV;
        texture.m_renderTargetView.m_pData = pRTV;
        texture.m_depthStencilView.m_pData = pDSV;
    }

    void RenderDevice::DestroyTexture( Texture& texture )
    {
        EE_ASSERT( IsInitialized() && texture.IsValid() );

        auto pTexture = (ID3D11Texture2D*) texture.m_textureHandle.m_pData;
        pTexture->Release();
        texture.m_textureHandle.Reset();

        auto pTextureSRV = (ID3D11ShaderResourceView*) texture.m_shaderResourceView.m_pData;
        if ( pTextureSRV )
        {
            pTextureSRV->Release();
            texture.m_shaderResourceView.Reset();
        }

        auto pTextureUAV = (ID3D11UnorderedAccessView*) texture.m_unorderedAccessView.m_pData;
        if ( pTextureUAV )
        {
            pTextureUAV->Release();
            texture.m_unorderedAccessView.Reset();
        }

        auto pRTV = (ID3D11RenderTargetView*) texture.m_renderTargetView.m_pData;
        if ( pRTV )
        {
            pRTV->Release();
            texture.m_renderTargetView.Reset();
        }

        auto pDSV = (ID3D11DepthStencilView*) texture.m_depthStencilView.m_pData;
        if ( pDSV )
        {
            pDSV->Release();
            texture.m_depthStencilView.Reset();
        }
    }

    //-------------------------------------------------------------------------

    namespace DX11
    {
        D3D11_FILTER GetTextureFilter( TextureFiltering filtering )
        {
            switch ( filtering )
            {
                case TextureFiltering::MinMagMipPoint: return D3D11_FILTER_MIN_MAG_MIP_POINT; break;
                case TextureFiltering::MinMagPointMipLinear: return D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR; break;
                case TextureFiltering::MinPointMagLinearMipPoint: return D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT; break;
                case TextureFiltering::MinPointMagMipLinear: return D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR; break;
                case TextureFiltering::MinLinearMagMipPoint: return D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT; break;
                case TextureFiltering::MinLinearMagPointMipLinear: return D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR; break;
                case TextureFiltering::MinMagLinearMipPoint: return D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT; break;
                case TextureFiltering::MinMagMipLinear: return D3D11_FILTER_MIN_MAG_MIP_LINEAR; break;
                case TextureFiltering::Anisotropic: return D3D11_FILTER_ANISOTROPIC; break;
                case TextureFiltering::ComparisonMinMagMipPoint: return D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT; break;
                case TextureFiltering::ComparisonMinMagPointMipLinear: return D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR; break;
                case TextureFiltering::ComparisonMinPointMagLinearMipPoint: return D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT; break;
                case TextureFiltering::ComparisonMinPointMagMipLinear: return D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR; break;
                case TextureFiltering::ComparisonMinLinearMagMipPoint: return D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT; break;
                case TextureFiltering::ComparisonMinLinearMagPointMipLinear: return D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR; break;
                case TextureFiltering::ComparisonMinMagLinearMipPoint: return D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT; break;
                case TextureFiltering::ComparisonMinMagMipLinear: return D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR; break;
                case TextureFiltering::ComparisonAnisotropic: return D3D11_FILTER_COMPARISON_ANISOTROPIC; break;
                case TextureFiltering::MinimumMinMagMipPoint: return D3D11_FILTER_MINIMUM_MIN_MAG_MIP_POINT; break;
                case TextureFiltering::MinimumMinMagPointMipLinear: return D3D11_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR; break;
                case TextureFiltering::MinimumMinPointMagLinearMipPoint: return D3D11_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT; break;
                case TextureFiltering::MinimumMinPointMagMipLinear: return D3D11_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR; break;
                case TextureFiltering::MinimumMinLinearMagMipPoint: return D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT; break;
                case TextureFiltering::MinimumMinLinearMagPointMipLinear:return D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR; break;
                case TextureFiltering::MinimumMinMagLinearMipPoint:  return D3D11_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT; break;
                case TextureFiltering::MinimumMinMagMipLinear: return D3D11_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR; break;
                case TextureFiltering::MinimumAnisotropic: return D3D11_FILTER_MINIMUM_ANISOTROPIC; break;
                case TextureFiltering::MaximumMinMagMipPoint: return D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_POINT; break;
                case TextureFiltering::MaximumMinMagPointMipLinear: return D3D11_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR; break;
                case TextureFiltering::MaximumMinPointMagLinearMipPoint: return D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT; break;
                case TextureFiltering::MaximumMinPointMagMipLinear: return D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR; break;
                case TextureFiltering::MaximumMinLinearMagMipPoint: return D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT; break;
                case TextureFiltering::MaximumMinLinearMagPointMipLinear: return D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR; break;
                case TextureFiltering::MaximumMinMagLinearMipPoint: return D3D11_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT; break;
                case TextureFiltering::MaximumMinMagMipLinear: return D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR; break;
                case TextureFiltering::MaximumAnisotropic: return D3D11_FILTER_MAXIMUM_ANISOTROPIC; break;
            }

            return D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
        }

        D3D11_TEXTURE_ADDRESS_MODE GetTextureAddressMode( TextureAddressMode mode )
        {
            switch ( mode )
            {
                case TextureAddressMode::Wrap: return D3D11_TEXTURE_ADDRESS_WRAP; break;
                case TextureAddressMode::Mirror: return D3D11_TEXTURE_ADDRESS_MIRROR; break;
                case TextureAddressMode::Clamp: return D3D11_TEXTURE_ADDRESS_CLAMP; break;
                case TextureAddressMode::Border: return D3D11_TEXTURE_ADDRESS_BORDER; break;
            }

            return D3D11_TEXTURE_ADDRESS_WRAP;
        }
    }

    void RenderDevice::CreateSamplerState( SamplerState& state )
    {
        EE_ASSERT( IsInitialized() );

        D3D11_SAMPLER_DESC samplerDesc;
        EE::Memory::MemsetZero( &samplerDesc, sizeof( D3D11_SAMPLER_DESC ) );
        samplerDesc.Filter = DX11::GetTextureFilter( TextureFiltering::Anisotropic );
        samplerDesc.AddressU = DX11::GetTextureAddressMode( state.m_addressModeU );
        samplerDesc.AddressV = DX11::GetTextureAddressMode( state.m_addressModeV );
        samplerDesc.AddressW = DX11::GetTextureAddressMode( state.m_addressModeW );
        memcpy( samplerDesc.BorderColor, &state.m_borderColor, sizeof( state.m_borderColor ) );
        samplerDesc.MipLODBias = state.m_LODBias;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        samplerDesc.MinLOD = state.m_minLOD;
        samplerDesc.MaxLOD = state.m_maxLOD;
        samplerDesc.MaxAnisotropy = state.m_maxAnisotropyValue;

        m_pDevice->CreateSamplerState( &samplerDesc, (ID3D11SamplerState**) &state.m_resourceHandle.m_pData );
    }

    void RenderDevice::DestroySamplerState( SamplerState& state )
    {
        EE_ASSERT( IsInitialized() );
        EE_ASSERT( state.IsValid() );
        ( (ID3D11SamplerState*) state.m_resourceHandle.m_pData )->Release();
        state.m_resourceHandle.Reset();
    }

    //-------------------------------------------------------------------------

    void RenderDevice::CreateRenderTarget( RenderTarget& renderTarget, Int2 const& dimensions, bool createPickingTarget )
    {
        EE_ASSERT( IsInitialized() && !renderTarget.IsValid() );
        EE_ASSERT( dimensions.m_x >= 0 && dimensions.m_y >= 0 );
        CreateTexture( renderTarget.m_RT, DataFormat::UNorm_R8G8B8A8, dimensions, USAGE_SRV | USAGE_RT_DS );
        CreateTexture( renderTarget.m_DS, DataFormat::Float_X32, dimensions, USAGE_RT_DS );

        if ( createPickingTarget )
        {
            CreateTexture( renderTarget.m_pickingRT, DataFormat::UInt_R32G32B32A32, dimensions, USAGE_SRV | USAGE_RT_DS );
            CreateTexture( renderTarget.m_pickingStagingTexture, DataFormat::UInt_R32G32B32A32, Int2( 1, 1 ), USAGE_STAGING );
        }
    }

    void RenderDevice::ResizeRenderTarget( RenderTarget& renderTarget, Int2 const& newDimensions )
    {
        EE_ASSERT( IsInitialized() && renderTarget.IsValid() );
        bool const createPickingRT = renderTarget.HasPickingRT();
        DestroyRenderTarget( renderTarget );
        CreateRenderTarget( renderTarget, newDimensions, createPickingRT );
    }

    void RenderDevice::DestroyRenderTarget( RenderTarget& renderTarget )
    {
        EE_ASSERT( IsInitialized() && renderTarget.IsValid() );
        DestroyTexture( renderTarget.m_RT );
        DestroyTexture( renderTarget.m_DS );

        if ( renderTarget.m_pickingRT.IsValid() )
        {
            DestroyTexture( renderTarget.m_pickingRT );
            DestroyTexture( renderTarget.m_pickingStagingTexture );
        }
    }

    PickingID RenderDevice::ReadBackPickingID( RenderTarget const& renderTarget, Int2 const& pixelCoords )
    {
        EE_ASSERT( IsInitialized() && renderTarget.IsValid() );
        
        PickingID pickingID;

        //-------------------------------------------------------------------------

        if ( pixelCoords.m_x < 0 && pixelCoords.m_x >= renderTarget.GetDimensions().m_x )
        {
            return pickingID;
        }

        if ( pixelCoords.m_y < 0 && pixelCoords.m_y >= renderTarget.GetDimensions().m_y )
        {
            return pickingID;
        }

        //-------------------------------------------------------------------------

        LockDevice();
        {
            // Copy from the RT to the staging texture
            D3D11_BOX srcBox;
            srcBox.left = pixelCoords.m_x;
            srcBox.right = pixelCoords.m_x + 1;
            srcBox.top = pixelCoords.m_y;
            srcBox.bottom = pixelCoords.m_y + 1;
            srcBox.front = 0;
            srcBox.back = 1;

            auto pPickingTexture = (ID3D11Texture2D*) renderTarget.m_pickingRT.m_textureHandle.m_pData;
            auto pStagingTexture = (ID3D11Texture2D*) renderTarget.m_pickingStagingTexture.m_textureHandle.m_pData;
            m_immediateContext.m_pDeviceContext->CopySubresourceRegion( pStagingTexture, 0, 0, 0, 0, pPickingTexture, 0, &srcBox );

            // Map the staging texture and read back the value
            D3D11_MAPPED_SUBRESOURCE msr = {};
            m_immediateContext.m_pDeviceContext->Map( pStagingTexture, 0, D3D11_MAP::D3D11_MAP_READ, 0, &msr );

            uint32_t* pData = reinterpret_cast<uint32_t*>( msr.pData );
            pickingID.m_0 = pData[1];
            pickingID.m_0 = pickingID.m_0 << 32;
            pickingID.m_0 |= pData[0];

            pickingID.m_1 = pData[3];
            pickingID.m_1 = pickingID.m_1 << 32;
            pickingID.m_1 |= pData[2];

            m_immediateContext.m_pDeviceContext->Unmap( pStagingTexture, 0 );
        }
        UnlockDevice();

        return pickingID;
    }
}