#pragma once

#include "Base/Types/Arrays.h"
#include "Base/Types/String.h"
#include "Base/Types/BitFlags.h"

#include "Base/Math/Math.h"
#include "Base/Types/StringID.h"

#include "Base/Profiling.h"

//-------------------------------------------------------------------------

namespace EE::Memory::Allocators
{
    EE_BASE_API extern MemoryAllocator g_RHI;
}

namespace EE::Render::RHI
{
    enum Limits
    {
        MaxInstanceExtensions = 64,
        MaxDeviceExtensions = 64,
        MaxRenderTargets = 8,

        MaxDebugNameLength = 256,
        MaxVendorNameLength = 256,
        MaxEntryPointNameLength = 64,

        MaxPendingFrames = 2, // Set this value to 2 for double buffering, or 3 for triple buffering.

        MaxDispatchSize = 65535U,
    };

    enum struct QueryType
    {
        Timestamp,
        PipelineStatistics,
    };

    enum struct QueueFlags
    {
        DisableTimeout,
    };

    enum struct QueuePriority
    {
        Normal,
        High,
        GlobalRealtime,
    };

    enum struct QueueType
    {
        Graphics,
        Compute,
        Transfer
    };

    enum struct LoadActionType
    {
        DontCare,
        Load,
        Clear
    };

    enum struct StoreActionType
    {
        DontCare,
        Store,
        None,
    };

    enum struct DataFormat
    {
        // NOTE: Bump the texture resource version if you add or remove members of this enum

        Undefined,

        // Uncompressed formats
        R1_UNorm,
        RGB565_UNorm,
        BGR565_UNorm,
        BGR555_A1_UNorm,
        R8_UNorm,
        R8_SNorm,
        R8_UInt,
        R8_SInt,
        RG8_UNorm,
        RG8_SNorm,
        RG8_UInt,
        RG8_SInt,
        BGRA4_UNorm,
        RGBA8_UNorm,
        RGBA8_SNorm,
        RGBA8_UInt,
        RGBA8_SInt,
        RGBA8_sRGB,
        BGRA8_UNorm,
        BGRA8_sRGB,
        RGB10_A2_UNorm,
        RGB10_A2_UInt,
        R16_UNorm,
        R16_SNorm,
        R16_UInt,
        R16_SInt,
        R16_SFloat,
        RG16_UNorm,
        RG16_SNorm,
        RG16_UInt,
        RG16_SInt,
        RG16_SFloat,
        RGBA16_UNorm,
        RGBA16_SNorm,
        RGBA16_UInt,
        RGBA16_SInt,
        RGBA16_SFloat,
        R32_UInt,
        R32_SInt,
        R32_SFloat,
        RG32_UInt,
        RG32_SInt,
        RG32_SFloat,
        RGB32_UInt,
        RGB32_SInt,
        RGB32_SFloat,
        RGBA32_UInt,
        RGBA32_SInt,
        RGBA32_SFloat,
        RG11_B10_UFloat,
        RGB9_E5_UFloat,
        D32_SFloat,
        D32_SFloat_S8_UInt,
        S8_Uint,

        // Compressed DXBC formats
        DXBC1_RGB_UNorm,
        DXBC1_RGB_sRGB,
        DXBC1_RGBA_UNorm,
        DXBC1_RGBA_sRGB,
        DXBC2_UNorm,
        DXBC2_sRGB,
        DXBC3_UNorm,
        DXBC3_sRGB,
        DXBC4_UNorm,
        DXBC4_SNorm,
        DXBC5_UNorm,
        DXBC5_SNorm,
        DXBC6H_UFloat,
        DXBC6H_SFloat,
        DXBC7_UNorm,
        DXBC7_sRGB,

        // Compressed ASTC formats
        ASTC_4x4_UNorm,
        ASTC_4x4_sRGB,
        ASTC_5x4_UNorm,
        ASTC_5x4_sRGB,
        ASTC_5x5_UNorm,
        ASTC_5x5_sRGB,
        ASTC_6x5_UNorm,
        ASTC_6x5_sRGB,
        ASTC_6x6_UNorm,
        ASTC_6x6_sRGB,
        ASTC_8x5_UNorm,
        ASTC_8x5_sRGB,
        ASTC_8x6_UNorm,
        ASTC_8x6_sRGB,
        ASTC_8x8_UNorm,
        ASTC_8x8_sRGB,
        ASTC_10x5_UNorm,
        ASTC_10x5_sRGB,
        ASTC_10x6_UNorm,
        ASTC_10x6_sRGB,
        ASTC_10x8_UNorm,
        ASTC_10x8_sRGB,
        ASTC_10x10_UNorm,
        ASTC_10x10_sRGB,
        ASTC_12x10_UNorm,
        ASTC_12x10_sRGB,
        ASTC_12x12_UNorm,
        ASTC_12x12_sRGB,
    };
    static constexpr uint32_t NumDataFormats = static_cast<size_t>( DataFormat::ASTC_12x12_sRGB ) + 1;

    static const uint32_t BufferAlignment = 4;
    static const uint32_t IndirectCommandAlignment = 16;

    enum struct ResourceMemoryType
    {
        DeviceLocal,
        DeviceToHost,
        HostToDevice,
    };

    enum struct IndirectArgumentType
    {
        Invalid,
        Draw,
        DrawIndexed,
        DispatchCompute,
        DispatchMesh,
        DispatchRays,
        VertexBuffer,
        IndexBuffer,
        Constant,
        ConstantBufferView,
        ShaderResourceView,
        UnorderedAccessView,
    };

    enum struct DescriptorTypeFlags
    {
        Sampler,
        Texture,
        TextureCube,
        RWTexture,
        Buffer,
        RWBuffer,
        Raw,
        ConstantBuffer,
        IndexBuffer,
        IndirectArgumentBuffer,
        RootConstant,
        RenderTarget,
        AccelerationStructure,
    };

    enum struct ShaderStage
    {
        Vertex,
        Pixel,
        Task,
        Mesh,
        Compute,
        RayTracing,
    };

    static TBitFlags<ShaderStage> const ShaderStageFlagAllGraphics =
        TBitFlags<ShaderStage>( ShaderStage::Vertex, ShaderStage::Pixel );

    static TBitFlags<ShaderStage> const ShaderStageFlagAllMesh =
        TBitFlags<ShaderStage>( ShaderStage::Task, ShaderStage::Mesh );

    enum struct PrimitiveTopology
    {
        TriangleList,
        TriangleStrip,
    };

    enum struct IndexType
    {
        Uint16,
        Uint32,
    };

    enum struct BlendConstant
    {
        Zero,
        One,
        SrcColor,
        OneMinusSrcColor,
        DstColor,
        OneMinusDstColor,
        SrcAlpha,
        OneMinusSrcAlpha,
        DstAlpha,
        OneMinusDstAlpha,
        SrcAlphaSaturate,
        BlendFactor,
        OneMinusBlendFactor,
    };

    enum struct BlendMode
    {
        Add,
        Subtract,
        ReverseSubtract,
        Min,
        Max
    };

    enum struct CompareMode
    {
        Never,
        Less,
        Equal,
        LessEqual,
        Greater,
        NotEqual,
        GreaterEqual,
        Always,
    };

    enum struct StencilOp
    {
        Keep,
        SetZero,
        Replace,
        Invert,
        Increment,
        Decrement,
        IncrementSaturate,
        DecrementSaturate,
    };

    enum struct BlendStateTargetFlags
    {
        Target0,
        Target1,
        Target2,
        Target3,
        Target4,
        Target5,
        Target6,
        Target7,
    };

    static TBitFlags<BlendStateTargetFlags> const BlendStateTargetFlagAll =
        TBitFlags<BlendStateTargetFlags>( BlendStateTargetFlags::Target0,
                                          BlendStateTargetFlags::Target1,
                                          BlendStateTargetFlags::Target2,
                                          BlendStateTargetFlags::Target3,
                                          BlendStateTargetFlags::Target4,
                                          BlendStateTargetFlags::Target5,
                                          BlendStateTargetFlags::Target6,
                                          BlendStateTargetFlags::Target7 );

    enum struct CullMode
    {
        None,
        Back,
        Front,
    };

    enum struct FrontFace
    {
        CounterClockWise,
        ClockWise
    };

    enum struct FillMode
    {
        Solid,
        Wireframe
    };

    enum struct PipelineType
    {
        Undefined,
        Graphics,
        Compute,
        RayTracing,
    };

    enum struct PipelineCacheFlags
    {
        ExternallySynchronized,
    };

    enum struct FilterType
    {
        Point,
        Linear,
    };

    enum struct FilterMode
    {
        Default,
        Compare,
        Min,
        Max,
    };

    enum struct MipMapMode
    {
        Point,
        Linear,
    };

    enum struct AddressMode
    {
        Wrap,
        ClampToEdge,
        ClampToBorder,
        Mirror,
    };

    enum struct ShadingRate
    {
        NotSupported,
        Full,
        Half,
        Quarter,
        Rate1x2,
        Rate2x1,
        Rate2x4,
        Rate4x2,
    };

    enum struct ShadingRateCombiner
    {
        Passthrough,
        Override,
        Min,
        Max,
        Sum
    };

    enum struct ShadingRateCaps
    {
        NotSupported,
        PerDraw,
        PerTile
    };

    enum struct BufferFlags
    {
        OwnMemory,
        PersistentMap,
        NoDescriptors,
        ESRAM,
        SubAllocations
    };

    enum struct TextureFlags
    {
        OwnMemory,
        ExportHandle,
        ExportAdapter,
        ImportHandle,
        ESRAM,
        DisableCompression,
        OnTile,
        AllowDisplayTarget,
    };

    enum struct ViewDimension
    {
        Undefined,
        Buffer,
        Texture1D,
        Texture1DArray,
        Texture2D,
        Texture2DArray,
        Texture2DMultisample,
        Texture2DMultisampleArray,
        Texture3D,
        TextureCube,
        TextureCubeArray,
        AccelerationStructure,
    };
    static constexpr uint32_t NumViewDimensions = static_cast<size_t>( ViewDimension::AccelerationStructure ) + 1;

    enum struct SamplerRange
    {
        Full,
        Narrow,
    };

    enum struct SamplerModelConversion
    {
        RgbIdentity,
        YCbCrIdentity,
        YCbCr709,
        YCbCr601,
        YCbCr2020,
    };

    enum struct RootSignatureFlags
    {
        Local,
    };

    enum struct MarkerTypeFlags
    {
        Default,
        In,
        Out,
        InOut,
    };

    enum struct WaveOpsSupportFlags
    {
        Basic,
        Vote,
        Arithmetic,
        Ballot,
        Shuffle,
        ShuffleRelative,
        Clustered,
        Quad,
        Partitioned,
    };

    enum struct AccelerationStructureBuildFlags
    {
        AllowUpdate,
        AllowCompaction,
        PreferFastTrace,
        PreferFastBuild,
        MinimizeMemory,
        PerformUpdate,
    };

    enum struct AccelerationStructureGeometryFlags
    {
        Opaque,
        NoDuplicateAnyhitInvocation,
    };

    enum struct AccelerationStructureInstanceFlags
    {
        TriangleCullDisable,
        TriangleFrontCCW,
        ForceOpaque,
        ForceNonOpaque,
    };

    enum struct ShaderModel
    {
        ShaderModel_6_6,
        ShaderModel_6_7,
    };

    enum struct DeviceMode
    {
        Single,
        Linked,
        Unlinked
    };

    enum struct DeviceSelectionPreference
    {
        PreferPerformance,
        PreferPowerEfficiency,
        UseProvidedIndex,
    };

    struct DeviceVendorInfo
    {
        TInlineString<MaxVendorNameLength>                  m_vendorID = {};
        TInlineString<MaxVendorNameLength>                  m_deviceID = {};
        TInlineString<MaxVendorNameLength>                  m_revisionID = {};
        TInlineString<MaxVendorNameLength>                  m_deviceName = {};
        uint32_t                                            m_numRaytracingCores = 0;
    };

    struct DeviceCapabilities
    {
        uint64_t                                            m_dedicatedVideoMemory = 0;
        uint32_t                                            m_constantBufferAlignment = 0;
        uint32_t                                            m_uploadBufferTextureAlignment = 0;
        uint32_t                                            m_uploadBufferTextureRowAlignment = 0;
        uint32_t                                            m_optimalRootSignatureSizeInDWORDs = 0;
        uint32_t                                            m_numWaveLanes = 0;
        TBitFlags<WaveOpsSupportFlags>                      m_waveOpsSupportFlags = {};
        ShadingRate                                         m_shadingRate = ShadingRate::NotSupported;
        TBitFlags<ShadingRateCaps>                          m_shadingRateCaps = ShadingRateCaps::NotSupported;
        uint32_t                                            m_shadingRateTexelWidth = 0;
        uint32_t                                            m_shadingRateTexelHeight = 0;
        bool                                                m_multiDrawIndirect = false;
        bool                                                m_indirectRootConstant = false;
        bool                                                m_rasterizerOrderViews = false;
        bool                                                m_breadcrumbs = false;
        bool                                                m_hdr = false;
        TArray<bool, NumDataFormats>                        m_canShaderReadFrom = {};
        TArray<bool, NumDataFormats>                        m_canShaderWriteTo = {};
        TArray<bool, NumDataFormats>                        m_canRenderTargetWriteTo = {};
    };

    struct ResourceAllocationStatistic
    {
        TBitFlags<DescriptorTypeFlags>  m_descriptorTypes;
        uint64_t                        m_numAllocations;
        uint64_t                        m_numBytes;
    };

    struct BufferSubAllocation
    {
        uint64_t                        m_offset = ~0ULL;
        uint64_t                        m_internal = ~0ULL;

        inline bool IsValid() const { return m_offset != ~0ULL; }
    };

    struct EE_BASE_API GenericResource
    {
        virtual ~GenericResource();
    };

    struct Context;
    struct Queue;
    struct Buffer;
    struct Texture;
    struct Sampler;
    struct QueryPool;
    struct Shader;
    struct ShaderByteCode;
    struct RootSignature;
    struct Pipeline;
    struct PipelineCache;
    struct AccelerationStructure;
    struct RaytracingShaderTable;
    struct CommandPool;
    struct CommandBuffer;
    struct CommandSignature;
    struct Swapchain;

    struct Context : GenericResource
    {
        DeviceVendorInfo                                    m_vendorInfo = {};
        DeviceCapabilities                                  m_deviceCapabilities = {};
        uint32_t                                            m_numLinkedNodes = 0;
        uint32_t                                            m_unlinkedNodeIndex = 0;
        DeviceMode                                          m_deviceMode = DeviceMode::Single;
        ShaderModel                                         m_shaderModel = ShaderModel::ShaderModel_6_6;
        bool                                                m_hostValidation = false;
        bool                                                m_deviceValidation = false;
        bool                                                m_deviceBreadcrumbs = false;
        bool                                                m_renderDoc = false;
        bool                                                m_amdAgsEnabled = false;
    };

    struct ContextParameters
    {
        char const*                                         m_pApplicationName = nullptr;
        char const*                                         m_pEngineName = nullptr;
        ShaderModel                                         m_shaderModel = ShaderModel::ShaderModel_6_6;
        DeviceMode                                          m_deviceMode = DeviceMode::Single;
        DeviceSelectionPreference                           m_deviceSelection = DeviceSelectionPreference::PreferPerformance;
        uint32_t                                            m_deviceIndex = 0; // This is used to force a specific device when deviceSelection == DeviceSelectionPreference::UseProvidedIndex
        bool                                                m_enableHostValidation = false;
        bool                                                m_enableDeviceValidation = false;
        bool                                                m_enableDeviceBreadcrumbs = false;
        bool                                                m_enableRenderDoc = false;
        bool                                                m_enablePix = false;
        bool                                                m_enableAmdAgs = false;
    };

    union ClearValue
    {
        struct
        {
            float                                           m_red;
            float                                           m_green;
            float                                           m_blue;
            float                                           m_alpha;
        };
        struct
        {
            float                                           m_depth;
            uint8_t                                         m_stencil;
        };
    };

    struct ShaderResource
    {
        TBitFlags<DescriptorTypeFlags>                      m_descriptorTypeFlags = {};
        uint32_t                                            m_setIndex = 0;
        uint32_t                                            m_registerIndex = 0;
        uint32_t                                            m_numConstants = 0;
        TBitFlags<ShaderStage>                              m_usedStages = {};
        String                                              m_name = {}; // TODO: Make it StringView and point to an internal pool?
        ViewDimension                                       m_viewDimension = ViewDimension::Undefined;
    };

    struct ShaderVariable
    {
        String                                              m_name = {}; // TODO: Make it StringView and point to an internal pool?
        uint32_t                                            m_parentResourceIndex = 0;
        uint32_t                                            m_offset = 0;
        uint32_t                                            m_size = 0;
    };

    struct ShaderReflection
    {
        TVector<ShaderResource>                             m_shaderResources{ Memory::Allocators::g_RHI };
        TVector<ShaderVariable>                             m_shaderVariables{ Memory::Allocators::g_RHI };
        TBitFlags<ShaderStage>                              m_shaderStages = {};
        uint32_t                                            m_threadsPerGroup[3] = {};
    };

    struct PipelineReflection
    {
        TBitFlags<ShaderStage>                              m_shaderStages = {};
        TArrayView<ShaderReflection const>                  m_shaderStageReflections = {};
        uint32_t                                            m_vertexStageIndex = 0;
        uint32_t                                            m_pixelStageIndex = 0;
        TArrayView<ShaderResource const>                    m_shaderResources = {};
        TArrayView<ShaderVariable const>                    m_shaderVariables = {};
    };

    struct DescriptorReflection
    {
        String                                              m_name = {};
        TBitFlags<DescriptorTypeFlags>                      m_descriptorTypeFlags = {};
        ViewDimension                                       m_viewDimension = ViewDimension::Undefined;
        uint32_t                                            m_numConstants = 0;
        int32_t                                             m_parameterIndex = 0;
        int32_t                                             m_setIndex = 0;
    };

    struct IndirectDrawArguments
    {
        uint32_t                                m_numVertices = 0;
        uint32_t                                m_numInstances = 0;
        uint32_t                                m_firstVertex = 0;
        uint32_t                                m_firstInstance = 0;
    };

    struct IndirectDrawIndexedArguments
    {
        uint32_t                                m_numIndices = 0;
        uint32_t                                m_numInstances = 0;
        uint32_t                                m_firstIndex = 0;
        int32_t                                 m_baseVertex = 0;
        uint32_t                                m_firstInstance = 0;
    };

    struct IndirectDispatchArguments
    {
        uint32_t                                m_groupCountX;
        uint32_t                                m_groupCountY;
        uint32_t                                m_groupCountZ;
    };

    enum struct PipelineStage
    {
        All,
        Draw,
        PixelShader,
        NonPixelShader,
        ComputeShader,
        Raytracing,
        AllShader,
        Copy,
        ExecuteIndirect,
        VideoDecode,
        VideoProcess,
        VideoEncode,
        BuildAccelerationStructure,
        CopyAccelerationStructure,
    };

    static TBitFlags<PipelineStage> const PipelineStageFlags_AllShader = TBitFlags<PipelineStage>
        (
            PipelineStage::PixelShader,
            PipelineStage::NonPixelShader,
            PipelineStage::ComputeShader,
            PipelineStage::AllShader
        );

    // These flags are only allowed on the graphics queue
    static TBitFlags<PipelineStage> const PipelineStageFlags_GraphicsQueueOnly = TBitFlags<PipelineStage>
        (
            PipelineStage::Draw,
            PipelineStage::PixelShader
        );

    // These flags are only allowed on the video queue
    static TBitFlags<PipelineStage> const PipelineStageFlags_VideoQueueOnly = TBitFlags<PipelineStage>
        (
            PipelineStage::VideoDecode,
            PipelineStage::VideoProcess,
            PipelineStage::VideoEncode
        );

    enum struct ResourceAccess
    {
        NoAccess,

        // Writeable
        Present,
        RenderTarget,
        UnorderedAccess,
        DepthWrite,
        CopyDestination,
        AccelerationStructureWrite,
        VideoDecodeWrite,
        VideoProcessWrite,
        VideoEncodeWrite,

        // Read only
        Common,
        ConstantBuffer,
        IndexBuffer,
        DepthRead,
        ShaderResource,
        IndirectArgument,
        CopySource,
        AccelerationStructureRead,
        ShadingRateSource,
        VideoDecodeRead,
        VideoProcessRead,
        VideoEncodeRead,
    };

    static TBitFlags<ResourceAccess> const ResourceAccessFlags_AllWriteable = TBitFlags<ResourceAccess>
        (
            ResourceAccess::Present,
            ResourceAccess::RenderTarget,
            ResourceAccess::UnorderedAccess,
            ResourceAccess::DepthWrite,
            ResourceAccess::CopyDestination,
            ResourceAccess::AccelerationStructureWrite,
            ResourceAccess::VideoDecodeWrite,
            ResourceAccess::VideoProcessWrite,
            ResourceAccess::VideoEncodeWrite
        );

    static TBitFlags<ResourceAccess> const ResourceAccessFlags_AllReadOnly = TBitFlags<ResourceAccess>
        (
            ResourceAccess::Common,
            ResourceAccess::ConstantBuffer,
            ResourceAccess::IndexBuffer,
            ResourceAccess::DepthRead,
            ResourceAccess::ShaderResource,
            ResourceAccess::IndirectArgument,
            ResourceAccess::CopySource,
            ResourceAccess::AccelerationStructureRead,
            ResourceAccess::ShadingRateSource,
            ResourceAccess::VideoDecodeRead,
            ResourceAccess::VideoProcessRead,
            ResourceAccess::VideoEncodeRead
        );

    // These flags are only allowed on the graphics queue
    static TBitFlags<ResourceAccess> const ResourceAccessFlags_GraphicsQueueOnly = TBitFlags<ResourceAccess>
        (
            ResourceAccess::Present,
            ResourceAccess::RenderTarget,
            ResourceAccess::DepthWrite,
            ResourceAccess::DepthRead
        );

    // These flags are only allowed on the video queue
    static TBitFlags<ResourceAccess> const ResourceAccessFlags_VideoQueueOnly = TBitFlags<ResourceAccess>
        (
            ResourceAccess::VideoDecodeRead,
            ResourceAccess::VideoDecodeWrite,
            ResourceAccess::VideoProcessRead,
            ResourceAccess::VideoProcessWrite,
            ResourceAccess::VideoEncodeRead,
            ResourceAccess::VideoEncodeWrite
        );

    enum struct TextureState
    {
        Undefined,
        Common,
        ShaderResource,
        UnorderedAccess,
        Present,
        RenderTarget,
        DepthWrite,
        DepthRead,
        ShadingRateSource,
        VideoDecodeRead,
        VideoDecodeWrite,
        VideoProcessRead,
        VideoProcessWrite,
        VideoEncodeRead,
        VideoEncodeWrite
    };

    enum struct TextureBarrierFlags
    {
        Discard,
    };

    struct TextureBarrierRegion
    {
        uint32_t                                m_mipLevel = 0;
        uint32_t                                m_numMipLevels = 0; // 0 means will use the entire texture
        uint32_t                                m_arraySlice = 0;
        uint32_t                                m_numArraySlices = 0; // 0 means will use the entire texture
    };

    struct TextureCopyRegion
    {
        uint32_t                                m_x = 0;
        uint32_t                                m_width = 1;

        uint32_t                                m_y = 0;
        uint32_t                                m_height = 1;

        uint32_t                                m_z = 0;
        uint32_t                                m_depth = 1;

        uint32_t                                m_mipLevel = 0;
        uint32_t                                m_arrayLayer = 0;

        // Very common operation when iterating mip levels and array slices
        TextureCopyRegion SkipTo( uint32_t newMipLevel, uint32_t newArrayLayer ) const;
    };

    inline TextureCopyRegion TextureCopyRegion::SkipTo( uint32_t newMipLevel, uint32_t newArrayLayer ) const
    {
        uint32_t const mipDifference = ( newMipLevel - m_mipLevel );

        TextureCopyRegion result = *this;
        result.m_mipLevel = newMipLevel;
        result.m_arrayLayer = newArrayLayer;
        result.m_x >>= mipDifference;
        result.m_width = Math::Max( result.m_width >> mipDifference, 1U );
        result.m_y >>= mipDifference;
        result.m_height = Math::Max( result.m_height >> mipDifference, 1U );
        result.m_z >>= mipDifference;
        result.m_depth = Math::Max( result.m_depth >> mipDifference, 1U );
        return result;
    }

    struct Queue : GenericResource
    {
        QueueType                                           m_queueType = QueueType::Graphics;
        uint32_t                                            m_nodeIndex = 0;
        bool                                                m_unifiedMemory = false;
    };

    struct Buffer : GenericResource
    {
        void*                                               m_pMappedAddress_WriteCombined = nullptr;
        uint64_t                                            m_size = 0;
        uint64_t                                            m_stride = 0;
        uint64_t                                            m_deviceAddress = 0;
        ResourceMemoryType                                  m_memoryType = ResourceMemoryType::DeviceLocal;
        uint32_t                                            m_nodeIndex = 0;
        TBitFlags<DescriptorTypeFlags>                      m_descriptorTypes = {};
    };

    struct Texture : GenericResource
    {
        uint32_t                                            m_width = 0;
        uint32_t                                            m_height = 0;
        uint32_t                                            m_depth = 0;
        uint32_t                                            m_arrayLayers = 0;
        uint32_t                                            m_mipLevels = 0;
        DataFormat                                          m_format = DataFormat::Undefined;
        uint32_t                                            m_numSamples = 1;
        uint32_t                                            m_sampleQuality = 0;
        uint32_t                                            m_nodeIndex = 0;
        ClearValue                                          m_clearValue = {};
        TBitFlags<DescriptorTypeFlags>                      m_descriptorTypes = {};
        TextureState                                        m_initialState = TextureState::ShaderResource;
    };

    struct Sampler : GenericResource
    {
        uint32_t                                            m_nodeIndex = 0;
    };

    struct QueryPool : GenericResource
    {
        uint32_t                                            m_numQueries = 0;
    };

    struct Shader : GenericResource
    {
        TBitFlags<ShaderStage>                              m_stages = {};
        TVector<ShaderReflection>                           m_stageReflections{ Memory::Allocators::g_RHI };
        TVector<String>                                     m_stageEntryNames{ Memory::Allocators::g_RHI };
        int32_t                                             m_vertexStageIndex = -1;
        int32_t                                             m_pixelStageIndex = -1;
        int32_t                                             m_taskStageIndex = -1;
        int32_t                                             m_meshStageIndex = -1;
        int32_t                                             m_computeStageIndex = -1;
    };

    struct RootSignature : GenericResource
    {
        TVector<DescriptorReflection>                       m_descriptorReflections{ Memory::Allocators::g_RHI };
        TVector<ShaderResource>                             m_shaderResources{ Memory::Allocators::g_RHI };
    };

    struct Pipeline : GenericResource
    {
        PipelineType                                        m_pipelineType = PipelineType::Undefined;
        PipelineReflection                                  m_pipelineReflection = {};
        RootSignature*                                      m_pRootSignature = nullptr;
    };

    struct PipelineCache : GenericResource
    {};

    struct AccelerationStructure : GenericResource
    {};

    struct RaytracingShaderTable : GenericResource
    {
        Pipeline*                                           m_pPipeline = nullptr;
        RootSignature*                                      m_pGlobalRootSignature = nullptr;
        char const*                                         m_rayGenShader = nullptr;
        TArrayView<char const*>                             m_rayMissShaders = {};
        TArrayView<char const*>                             m_hitGroups;
    };

    struct CommandPool : GenericResource
    {
        Queue*                                              m_pQueue = nullptr;
    };

    struct CommandBuffer : GenericResource
    {
        Queue*                                              m_pQueue = nullptr;
        CommandPool*                                        m_pCommandPool = nullptr;
        RootSignature*                                      m_pBoundRootSignature = nullptr;
        Pipeline*                                           m_pBoundPipeline = nullptr;
        uint32_t                                            m_nodeIndex = 0;
    };

    struct CommandSignature : GenericResource
    {
        IndirectArgumentType                                m_argumentType = IndirectArgumentType::Invalid;
        uint32_t                                            m_stride = 0;
    };

    struct Swapchain : GenericResource
    {
        TArray<Texture*, MaxPendingFrames>                  m_renderTargets = {};
        bool                                                m_vsync = false;
    };

    struct ReadRange
    {
        uint64_t                                            m_offset = 0;
        uint64_t                                            m_size = 0;
    };

    struct IndirectArgument
    {
        IndirectArgumentType                                m_type = IndirectArgumentType::Invalid;
        uint32_t                                            m_offset;
    };

    struct IndirectArgumentDescriptor
    {
        IndirectArgumentType                                m_type = IndirectArgumentType::Invalid;
        uint32_t                                            m_index = 0;
        uint32_t                                            m_byteSize = 0;
    };

    struct LoadAction
    {
        TArray<LoadActionType, MaxRenderTargets>            m_loadActionsColor = {};
        LoadActionType                                      m_loadActionDepth = {};
        LoadActionType                                      m_loadActionStencil = {};
        TArray<ClearValue, MaxRenderTargets>                m_colorClearValues = {};
        ClearValue                                          m_depthClearValue = {};
        TArray<StoreActionType, MaxRenderTargets>           m_storeActionsColor = {};
        StoreActionType                                     m_storeActionsDepth = {};
        StoreActionType                                     m_storeActionStencil = {};
    };

    struct BlendState
    {
        TArray<BlendConstant, MaxRenderTargets>             m_srcFactors = {};
        TArray<BlendConstant, MaxRenderTargets>             m_dstFactors = {};
        TArray<BlendConstant, MaxRenderTargets>             m_srcAlphaFactors = {};
        TArray<BlendConstant, MaxRenderTargets>             m_dstAlphaFactors = {};
        TArray<BlendMode, MaxRenderTargets>                 m_blendModes = {};
        TArray<BlendMode, MaxRenderTargets>                 m_blendModesAlpha = {};
        TArray<uint8_t, MaxRenderTargets>                   m_writeMasks = { 0x0F, 0x0F, 0x0F,0x0F,0x0F,0x0F,0x0F,0x0F };
        TBitFlags<BlendStateTargetFlags>                    m_renderTargetMask = RHI::BlendStateTargetFlags::Target0;
        bool                                                m_alphaToCoverage = false;
        bool                                                m_independentBlend = false;
        bool                                                m_blendEnabled = false;
    };

    struct DepthStencilState
    {
        bool                                                m_depthTest = false;
        bool                                                m_depthWrite = false;
        CompareMode                                         m_depthCompareMode;
        bool                                                m_stencilTest = false;
        uint8_t                                             m_stencilReadMask = 0;
        uint8_t                                             m_stencilWriteMask = 0;
        CompareMode                                         m_stencilFrontCompareMode = CompareMode::Never;
        StencilOp                                           m_stencilFrontFail = StencilOp::Keep;
        StencilOp                                           m_stencilFrontPass = StencilOp::Keep;
        StencilOp                                           m_depthFrontFail = StencilOp::Keep;
        CompareMode                                         m_stencilBackCompareMode = CompareMode::Never;
        StencilOp                                           m_stencilBackFail = StencilOp::Keep;
        StencilOp                                           m_stencilBackPass = StencilOp::Keep;
        StencilOp                                           m_depthBackFail = StencilOp::Keep;
    };

    struct RasterizerState
    {
        CullMode                                            m_cullMode = CullMode::Back;
        int32_t                                             m_depthBias = 0;
        float                                               m_depthBiasClamp = 0.0F;
        float                                               m_slopeScaledDepthBias = 0.0F;
        FillMode                                            m_fillMode = FillMode::Solid;
        FrontFace                                           m_frontFace = FrontFace::CounterClockWise;
        bool                                                m_multisample = false;
        bool                                                m_scissor = false;
        bool                                                m_depthClip = false;
    };

    struct RaytracingHitGroup
    {
        RootSignature*                                      m_pRootSignature = nullptr;
        Shader*                                             m_pIntersectionShader = nullptr;
        Shader*                                             m_pAnyHitShader = nullptr;
        Shader*                                             m_pClosestHitShader = nullptr;
        StringView                                          m_intersectionEntryPoint = {};
        StringView                                          m_anyHitEntryPoint = {};
        StringView                                          m_closestHitEntryPoint = {};
        StringView                                          m_hitGroupName = {};
    };

    struct QueueParameters
    {
        QueueType                                           m_queueType = QueueType::Graphics;
        TBitFlags<QueueFlags>                               m_flags = {};
        QueuePriority                                       m_priority = QueuePriority::Normal;
        uint32_t                                            m_nodeIndex = 0;
        TInlineString<MaxDebugNameLength>                   m_debugName = {};
    };

    struct QueryPoolParameters
    {
        QueryType                                           m_queryType = QueryType::Timestamp;
        uint32_t                                            m_numQueries = 0;
        uint32_t                                            m_nodeIndex = 0;
        TInlineString<MaxDebugNameLength>                   m_debugName = {};
    };

    struct BufferParameters
    {
        uint64_t                                            m_bufferSize = 0;
        uint64_t                                            m_bufferStride = 0;
        uint64_t                                            m_firstElement = 0;
        uint32_t                                            m_alignment = BufferAlignment;
        ResourceMemoryType                                  m_memoryType = ResourceMemoryType::DeviceLocal;
        TBitFlags<BufferFlags>                              m_flags = {};
        QueueType                                           m_queueType = QueueType::Graphics;
        DataFormat                                          m_format = DataFormat::Undefined;
        TBitFlags<DescriptorTypeFlags>                      m_descriptorTypes = DescriptorTypeFlags::Buffer;
        Buffer*                                             m_pCounterBuffer = nullptr;
        uint32_t                                            m_nodeIndex = 0;
        TArrayView<uint32_t const>                          m_sharedNodeIndices = {};
        TInlineString<MaxDebugNameLength>                   m_debugName = {};
    };

    struct TextureParameters
    {
        ClearValue                                          m_clearValue = {};
        ResourceMemoryType                                  m_memoryType = ResourceMemoryType::DeviceLocal;
        TBitFlags<TextureFlags>                             m_textureFlags = {};
        uint32_t                                            m_width = 1;
        uint32_t                                            m_height = 1;
        uint32_t                                            m_depth = 1;
        uint32_t                                            m_arrayLayers = 1;
        uint32_t                                            m_mipLevels = 1;
        uint32_t                                            m_numSamples = 1;
        uint32_t                                            m_sampleQuality = 0;
        TextureState                                        m_initialState = TextureState::ShaderResource;
        DataFormat                                          m_format = DataFormat::Undefined;
        TBitFlags<DescriptorTypeFlags>                      m_descriptorTypes = DescriptorTypeFlags::Texture;
        uint32_t                                            m_nodeIndex = 0;
        TArrayView<uint32_t const>                          m_sharedNodeIndices = {};
        void*                                               m_pNativeHandle = nullptr;
        Texture*                                            m_pTextureToAlias = nullptr;
        TInlineString<MaxDebugNameLength>                   m_debugName = {};
    };

    struct SamplerParameters
    {
        FilterType                                          m_minFilter = FilterType::Point;
        FilterType                                          m_magFilter = FilterType::Point;
        MipMapMode                                          m_mipMapMode = MipMapMode::Linear;
        AddressMode                                         m_addressModeU = AddressMode::Wrap;
        AddressMode                                         m_addressModeV = AddressMode::Wrap;
        AddressMode                                         m_addressModeW = AddressMode::Wrap;
        float                                               m_mipLODBias = 0.0F;
        float                                               m_setLODRange = 0.0F;
        float                                               m_minLOD = 0.0F;
        float                                               m_maxLOD = 1000.0F;
        uint32_t                                            m_maxAnisotropy = 0;
        CompareMode                                         m_compareMode = CompareMode::Never;
        FilterMode                                          m_filterMode = FilterMode::Default;
        float                                               m_borderColor[4] = {};
        uint32_t                                            m_nodeIndex = 0;
    };

    struct RootSignatureParameters
    {
        Shader*                                             m_pShader = nullptr;
        TArrayView<char const*>                             m_staticSamplerNames = {};
        TArrayView<Sampler* const>                          m_staticSamplers = {};
        TInlineString<MaxDebugNameLength>                   m_debugName = {};
    };

    struct ShaderByteCode
    {
        ShaderStage                                         m_stage;
        StringID                                            m_ID;
        char const*                                         m_pCompressedData = nullptr;
        uint32_t                                            m_decompressedSize = 0;
        uint32_t                                            m_decodedSize = 0;
    };

    struct CommandPoolParameters
    {
        Queue*                                              m_pQueue = nullptr;
        TInlineString<MaxDebugNameLength>                   m_debugName = {};
    };

    struct CommandBufferParameters
    {
        CommandPool*                                        m_pCommandPool = nullptr;
        TInlineString<MaxDebugNameLength>                   m_debugName = {};
    };

    struct GraphicsPipelineParameters
    {
        PipelineCache*                                      m_pPipelineCache = nullptr;
        TArrayView<void const*>                             m_pipelineExtensions;
        RootSignature*                                      m_pRootSignature = nullptr;
        Shader*                                             m_pShader = nullptr;
        BlendState                                          m_blendState = {};
        DepthStencilState                                   m_depthStencilState = {};
        RasterizerState                                     m_rasterizerState = {};
        TArrayView<DataFormat const>                        m_colorFormats = {};
        uint32_t                                            m_numRenderTargets = 0;
        DataFormat                                          m_depthStencilFormat = DataFormat::Undefined;
        PrimitiveTopology                                   m_primitiveTopology = PrimitiveTopology::TriangleList;
        uint32_t                                            m_numSamples = 1;
        uint32_t                                            m_sampleQuality = 0;
        TInlineString<MaxDebugNameLength>                   m_debugName = {};
    };

    struct MeshPipelineParameters : GraphicsPipelineParameters
    {};

    struct ComputePipelineParameters
    {
        PipelineCache*                                      m_pPipelineCache = nullptr;
        TArrayView<void const*>                             m_pipelineExtensions;
        RootSignature*                                      m_pRootSignature = {};
        Shader*                                             m_pShader = {};
        TInlineString<MaxDebugNameLength>                   m_debugName = {};
    };

    struct RaytracingPipelineParameters
    {
        PipelineCache*                                      m_pPipelineCache = nullptr;
        TArrayView<void const*>                             m_pipelineExtensions;
        RootSignature*                                      m_pGlobalRootSignature = nullptr;
        RootSignature*                                      m_pEmptyRootSignature = nullptr;
        RootSignature*                                      m_pRayGenRootSignature = nullptr;
        Shader*                                             m_pRayGenShader = nullptr;
        StringView                                          m_rayGenEntryPoint = {};
        TArrayView<RootSignature*>                          m_rayMissRootSignatures = {};
        TArrayView<Shader* const>                           m_rayMissShaders = {};
        TArrayView<StringView const>                        m_rayMissEntryPoints = {};
        TArrayView<RaytracingHitGroup const>                m_hitGroups = {};
        uint32_t                                            m_payloadSize = 0;
        uint32_t                                            m_attributeSize = 0;
        uint32_t                                            m_maxTraceRecursionDepth = 0;
        uint32_t                                            m_maxNumRays = 0;
        TInlineString<MaxDebugNameLength>                   m_debugName = {};
    };

    struct PipelineCacheParameters
    {
        TBitFlags<PipelineCacheFlags>                       m_flags = {};
        TArrayView<uint8_t const>                           m_initialCacheData = {};
    };

    struct SwapchainParameters
    {
        void*                                               m_pNativeWindowHandle = nullptr;
        TArrayView<Queue* const>                            m_presentQueues = {};
        uint32_t                                            m_numImages = MaxPendingFrames;
        uint32_t                                            m_width = 0;
        uint32_t                                            m_height = 0;
        DataFormat                                          m_colorFormat = DataFormat::RGBA8_UNorm;
        DataFormat                                          m_renderTargetFormat = DataFormat::RGBA8_sRGB;
        ClearValue                                          m_clearValue = {};
        bool                                                m_enableVSync = true;
        bool                                                m_useFlipSwapEffect = true;
    };

    struct CommandSignatureParameters
    {
        RootSignature*                                      m_pRootSignature = nullptr;
        TArrayView<IndirectArgumentDescriptor const>        m_indirectArgumentParameters = {};
        TInlineString<MaxDebugNameLength>                   m_debugName = {};
    };

    struct AccelerationStructureGeometry
    {
        Buffer*                                             m_pIndexBuffer = nullptr;
        Buffer*                                             m_pVertexBuffer = nullptr;
        TBitFlags<AccelerationStructureGeometryFlags>       m_flags = {};
        uint64_t                                            m_vertexOffset = 0;
        uint32_t                                            m_numVertices = 0;
        DataFormat                                          m_vertexFormat = DataFormat::Undefined;
        uint64_t                                            m_indexOffset = 0;
        uint32_t                                            m_numIndices = 0;
        IndexType                                           m_indexType = IndexType::Uint16;
    };

    struct AccelerationStructureInstance
    {
        float                                               m_transform[12];
        uint32_t                                            m_instanceCustomIndex : 24;
        uint32_t                                            m_mask : 8;
        uint32_t                                            m_instanceShaderBindingTableRecordOffset : 24;
        uint32_t                                            m_flags : 8;
        uint64_t                                            m_accelerationStructureReference;
    };

    struct AccelerationStructureBottomLevelCreateParameters
    {
        TBitFlags<AccelerationStructureBuildFlags>          m_flags = {};
        TArrayView<AccelerationStructureGeometry const>     m_geometries = {};
    };

    struct AccelerationStructureTopLevelCreateParameters
    {
        TBitFlags<AccelerationStructureBuildFlags>          m_flags = {};
        Buffer*                                             m_pInstanceBuffer = nullptr;
        uint64_t                                            m_instanceBufferOffset = 0;
        uint64_t                                            m_numInstances = 0;
    };

    inline bool IsCompressedFormat( DataFormat format )
    {
        uint32_t formatU32 = uint32_t( format );
        EE_ASSERT( formatU32 > uint32_t( DataFormat::Undefined ) && formatU32 < NumDataFormats );
        return formatU32 >= uint32_t( DataFormat::DXBC1_RGB_UNorm );
    }

    inline uint32_t FormatBlockBitSize( DataFormat format )
    {
        switch ( format )
        {
            case DataFormat::R1_UNorm: return 8;
            case DataFormat::R8_UNorm: return 8;
            case DataFormat::R8_SNorm: return 8;
            case DataFormat::R8_UInt: return 8;
            case DataFormat::R8_SInt: return 8;
            case DataFormat::BGRA4_UNorm: return 16;
            case DataFormat::RGB565_UNorm: return 16;
            case DataFormat::BGR565_UNorm: return 16;
            case DataFormat::BGR555_A1_UNorm: return 16;
            case DataFormat::RG8_UNorm: return 16;
            case DataFormat::RG8_SNorm: return 16;
            case DataFormat::RG8_UInt: return 16;
            case DataFormat::RG8_SInt: return 16;
            case DataFormat::R16_UNorm: return 16;
            case DataFormat::R16_SNorm: return 16;
            case DataFormat::R16_UInt: return 16;
            case DataFormat::R16_SInt: return 16;
            case DataFormat::R16_SFloat: return 16;
            case DataFormat::RGBA16_UNorm: return 64;
            case DataFormat::RGBA16_SNorm: return 64;
            case DataFormat::RGBA16_UInt: return 64;
            case DataFormat::RGBA16_SInt: return 64;
            case DataFormat::RGBA16_SFloat: return 64;
            case DataFormat::RG32_UInt: return 64;
            case DataFormat::RG32_SInt: return 64;
            case DataFormat::RG32_SFloat: return 64;
            case DataFormat::RGB32_UInt: return 96;
            case DataFormat::RGB32_SInt: return 96;
            case DataFormat::RGB32_SFloat: return 96;
            case DataFormat::RGBA32_UInt: return 128;
            case DataFormat::RGBA32_SInt: return 128;
            case DataFormat::RGBA32_SFloat: return 128;
            case DataFormat::D32_SFloat_S8_UInt: return 64;
            case DataFormat::DXBC1_RGB_UNorm: return 64;
            case DataFormat::DXBC1_RGB_sRGB: return 64;
            case DataFormat::DXBC1_RGBA_UNorm: return 64;
            case DataFormat::DXBC1_RGBA_sRGB: return 64;
            case DataFormat::DXBC2_UNorm: return 128;
            case DataFormat::DXBC2_sRGB: return 128;
            case DataFormat::DXBC3_UNorm: return 128;
            case DataFormat::DXBC3_sRGB: return 128;
            case DataFormat::DXBC4_UNorm: return 64;
            case DataFormat::DXBC4_SNorm: return 64;
            case DataFormat::DXBC5_UNorm: return 128;
            case DataFormat::DXBC5_SNorm: return 128;
            case DataFormat::DXBC6H_UFloat: return 128;
            case DataFormat::DXBC6H_SFloat: return 128;
            case DataFormat::DXBC7_UNorm: return 128;
            case DataFormat::DXBC7_sRGB: return 128;
            case DataFormat::ASTC_4x4_UNorm: return 128;
            case DataFormat::ASTC_4x4_sRGB: return 128;
            case DataFormat::ASTC_5x4_UNorm: return 128;
            case DataFormat::ASTC_5x4_sRGB: return 128;
            case DataFormat::ASTC_5x5_UNorm: return 128;
            case DataFormat::ASTC_5x5_sRGB: return 128;
            case DataFormat::ASTC_6x5_UNorm: return 128;
            case DataFormat::ASTC_6x5_sRGB: return 128;
            case DataFormat::ASTC_6x6_UNorm: return 128;
            case DataFormat::ASTC_6x6_sRGB: return 128;
            case DataFormat::ASTC_8x5_UNorm: return 128;
            case DataFormat::ASTC_8x5_sRGB: return 128;
            case DataFormat::ASTC_8x6_UNorm: return 128;
            case DataFormat::ASTC_8x6_sRGB: return 128;
            case DataFormat::ASTC_8x8_UNorm: return 128;
            case DataFormat::ASTC_8x8_sRGB: return 128;
            case DataFormat::ASTC_10x5_UNorm: return 128;
            case DataFormat::ASTC_10x5_sRGB: return 128;
            case DataFormat::ASTC_10x6_UNorm: return 128;
            case DataFormat::ASTC_10x6_sRGB: return 128;
            case DataFormat::ASTC_10x8_UNorm: return 128;
            case DataFormat::ASTC_10x8_sRGB: return 128;
            case DataFormat::ASTC_10x10_UNorm: return 128;
            case DataFormat::ASTC_10x10_sRGB: return 128;
            case DataFormat::ASTC_12x10_UNorm: return 128;
            case DataFormat::ASTC_12x10_sRGB: return 128;
            case DataFormat::ASTC_12x12_UNorm: return 128;
            case DataFormat::ASTC_12x12_sRGB: return 128;
            default: return 32;
        }
    }

    inline uint32_t FormatBlockWidth( DataFormat format )
    {
        switch ( format )
        {
            case DataFormat::DXBC2_UNorm: return 4;
            case DataFormat::DXBC2_sRGB: return 4;
            case DataFormat::DXBC3_UNorm: return 4;
            case DataFormat::DXBC3_sRGB: return 4;
            case DataFormat::DXBC4_UNorm: return 4;
            case DataFormat::DXBC4_SNorm: return 4;
            case DataFormat::DXBC5_UNorm: return 4;
            case DataFormat::DXBC5_SNorm: return 4;
            case DataFormat::DXBC6H_UFloat: return 4;
            case DataFormat::DXBC6H_SFloat: return 4;
            case DataFormat::DXBC7_UNorm: return 4;
            case DataFormat::DXBC7_sRGB: return 4;
            case DataFormat::ASTC_4x4_UNorm: return 4;
            case DataFormat::ASTC_4x4_sRGB: return 4;
            case DataFormat::ASTC_5x4_UNorm: return 5;
            case DataFormat::ASTC_5x4_sRGB: return 5;
            case DataFormat::ASTC_5x5_UNorm: return 5;
            case DataFormat::ASTC_5x5_sRGB: return 5;
            case DataFormat::ASTC_6x5_UNorm: return 6;
            case DataFormat::ASTC_6x5_sRGB: return 6;
            case DataFormat::ASTC_6x6_UNorm: return 6;
            case DataFormat::ASTC_6x6_sRGB: return 6;
            case DataFormat::ASTC_8x5_UNorm: return 8;
            case DataFormat::ASTC_8x5_sRGB: return 8;
            case DataFormat::ASTC_8x6_UNorm: return 8;
            case DataFormat::ASTC_8x6_sRGB: return 8;
            case DataFormat::ASTC_8x8_UNorm: return 8;
            case DataFormat::ASTC_8x8_sRGB: return 8;
            case DataFormat::ASTC_10x5_UNorm: return 10;
            case DataFormat::ASTC_10x5_sRGB: return 10;
            case DataFormat::ASTC_10x6_UNorm: return 10;
            case DataFormat::ASTC_10x6_sRGB: return 10;
            case DataFormat::ASTC_10x8_UNorm: return 10;
            case DataFormat::ASTC_10x8_sRGB: return 10;
            case DataFormat::ASTC_10x10_UNorm: return 10;
            case DataFormat::ASTC_10x10_sRGB: return 10;
            case DataFormat::ASTC_12x10_UNorm: return 12;
            case DataFormat::ASTC_12x10_sRGB: return 12;
            case DataFormat::ASTC_12x12_UNorm: return 12;
            case DataFormat::ASTC_12x12_sRGB: return 12;
            default: return 1;
        }
    }

    inline uint32_t FormatBlockHeight( DataFormat format )
    {
        switch ( format )
        {
            case DataFormat::DXBC1_RGB_UNorm: return 4;
            case DataFormat::DXBC1_RGB_sRGB: return 4;
            case DataFormat::DXBC1_RGBA_UNorm: return 4;
            case DataFormat::DXBC1_RGBA_sRGB: return 4;
            case DataFormat::DXBC2_UNorm: return 4;
            case DataFormat::DXBC2_sRGB: return 4;
            case DataFormat::DXBC3_UNorm: return 4;
            case DataFormat::DXBC3_sRGB: return 4;
            case DataFormat::DXBC4_UNorm: return 4;
            case DataFormat::DXBC4_SNorm: return 4;
            case DataFormat::DXBC5_UNorm: return 4;
            case DataFormat::DXBC5_SNorm: return 4;
            case DataFormat::DXBC6H_UFloat: return 4;
            case DataFormat::DXBC6H_SFloat: return 4;
            case DataFormat::DXBC7_UNorm: return 4;
            case DataFormat::DXBC7_sRGB: return 4;
            case DataFormat::ASTC_4x4_UNorm: return 4;
            case DataFormat::ASTC_4x4_sRGB: return 4;
            case DataFormat::ASTC_5x4_UNorm: return 4;
            case DataFormat::ASTC_5x4_sRGB: return 4;
            case DataFormat::ASTC_5x5_UNorm: return 5;
            case DataFormat::ASTC_5x5_sRGB: return 5;
            case DataFormat::ASTC_6x5_UNorm: return 5;
            case DataFormat::ASTC_6x5_sRGB: return 5;
            case DataFormat::ASTC_6x6_UNorm: return 6;
            case DataFormat::ASTC_6x6_sRGB: return 6;
            case DataFormat::ASTC_8x5_UNorm: return 5;
            case DataFormat::ASTC_8x5_sRGB: return 5;
            case DataFormat::ASTC_8x6_UNorm: return 6;
            case DataFormat::ASTC_8x6_sRGB: return 6;
            case DataFormat::ASTC_8x8_UNorm: return 8;
            case DataFormat::ASTC_8x8_sRGB: return 8;
            case DataFormat::ASTC_10x5_UNorm: return 5;
            case DataFormat::ASTC_10x5_sRGB: return 5;
            case DataFormat::ASTC_10x6_UNorm: return 6;
            case DataFormat::ASTC_10x6_sRGB: return 6;
            case DataFormat::ASTC_10x8_UNorm: return 8;
            case DataFormat::ASTC_10x8_sRGB: return 8;
            case DataFormat::ASTC_10x10_UNorm: return 10;
            case DataFormat::ASTC_10x10_sRGB: return 10;
            case DataFormat::ASTC_12x10_UNorm: return 10;
            case DataFormat::ASTC_12x10_sRGB: return 10;
            case DataFormat::ASTC_12x12_UNorm: return 12;
            case DataFormat::ASTC_12x12_sRGB: return 12;
            default: return 1;
        }
    }

    inline uint32_t ComputeFormatRowStride( DataFormat format, uint32_t width )
    {
        uint32_t blockWidth = FormatBlockWidth( format );
        uint32_t blockSizeInBytes = FormatBlockBitSize( format ) / 8;
        return ( ( width + blockWidth - 1 ) / blockWidth ) * blockSizeInBytes;
    }

    inline uint32_t ComputeFormatNumRows( DataFormat format, uint32_t height )
    {
        uint32_t blockHeight = FormatBlockHeight( format );
        return ( height + blockHeight - 1 ) / blockHeight;
    }

    inline uint32_t ComputeTextureMipLevels( uint32_t width, uint32_t height, uint32_t depth )
    {
        // Formula is log2(min(width, height, depth)) + 1 - 2, for integers this happens to be MSB of the value.
        // We don't want the lowest mip level to be smaller than 4x4 so we subtract 2 mip levels from the result.

        uint32_t numMipLevels = 0;
        if ( depth > 1 )
        {
            numMipLevels = Math::GetMostSignificantBit( Math::Min( width, Math::Min( height, depth ) ) );
        }
        else if ( height > 1 )
        {
            numMipLevels = Math::GetMostSignificantBit( Math::Min( width, height ) );
        }
        else
        {
            numMipLevels = Math::GetMostSignificantBit( width );
        }
        return numMipLevels + 1 - 2;
    }

    using GenericResourceHandle = uint16_t;
    static constexpr GenericResourceHandle InvalidResourceHandle = UINT16_MAX;

    using SamplerStateHandle = GenericResourceHandle;
    using BufferHandle = GenericResourceHandle;
    using TextureHandle = GenericResourceHandle;
    using AccelerationStructureHandle = GenericResourceHandle;

    //-------------------------------------------------------------------------------------------------------

    EE_BASE_API Context* CreateContext( ContextParameters const& parameters );
    EE_BASE_API void DestroyContext( Context*&& context );
    EE_BASE_API uint64_t GetTotalAllocatedDeviceMemory( Context* pContext );
    EE_BASE_API void GetDetailedMemoryStatistics( Context* pContext, uint64_t& localUsageBytes, uint64_t& localAvailableBytes, uint64_t& nonLocalUsageBytes, uint64_t& nonLocalAvailableBytes );
    EE_BASE_API void GetResourceAllocationStatistics( Context* pContext, TVector<ResourceAllocationStatistic>& outBufferStats, TVector<ResourceAllocationStatistic>& outTextureStats );

    EE_BASE_API void BeginFrameCapture( Context* pContext );
    EE_BASE_API void EndFrameCapture( Context* pContext );

    EE_BASE_API Queue* CreateQueue( Context* pContext, QueueParameters const& parameters );
    EE_BASE_API void DestroyQueue( Context* pContext, Queue*&& pQueue );
    EE_BASE_API uint64_t QueueGetCurrentSemaphore( Queue* pQueue );
    EE_BASE_API uint64_t QueueGetCompletedSemaphore( Queue* pQueue );
    EE_BASE_API void QueueHostWait( Queue* pQueue, uint64_t semaphore );
    EE_BASE_API void QueueDeviceWait( Queue* pQueueThatWaits, Queue* pQueueToWaitFor, uint64_t semaphore );
    EE_BASE_API uint64_t QueueSubmit( Queue* pQueue, TArrayView<CommandBuffer*> commandBuffer );
    EE_BASE_API uint64_t QueuePresent( Queue* pQueue, Swapchain* pSwapchain, uint32_t imageIndex );
    EE_BASE_API void WaitQueueIdle( Queue* pQueue );

    EE_BASE_API Swapchain* CreateSwapchain( Context* pContext, SwapchainParameters const& parameters );
    EE_BASE_API void DestroySwapchain( Context* pContext, Swapchain*&& pSwapchain );
    EE_BASE_API uint32_t AcquireNextImage( Context* pContext, Swapchain* pSwapchain );
    EE_BASE_API void SetVSync( Swapchain* pSwapchain, bool vsync );

    EE_BASE_API CommandPool* CreateCommandPool( Context* pContext, CommandPoolParameters const& parameters );
    EE_BASE_API void DestroyCommandPool( Context* pContext, CommandPool*&& pCommandPool );
    EE_BASE_API void ResetCommandPool( Context* pContext, CommandPool* pCommandPool );

    EE_BASE_API CommandBuffer* CreateCommandBuffer( Context* pContext, CommandBufferParameters const& parameters );
    EE_BASE_API void DestroyCommandBuffer( Context* pContext, CommandBuffer*&& pCommandBuffer );
    EE_BASE_API void BeginCommandBuffer( CommandBuffer* pCommandBuffer );
    EE_BASE_API void EndCommandBuffer( CommandBuffer* pCommandBuffer );
    EE_BASE_API void CmdSetRenderTargets( CommandBuffer* pCommandBuffer, TArrayView<Texture* const> renderTargets, Texture* pDepthStencil, LoadAction* pLoadAction = nullptr, TArrayView<uint32_t const> colorArraySlices = {}, TArrayView<uint32_t const> colorMipSlices = {}, uint32_t depthArraySlice = 0, uint32_t depthMipSlice = 0 );
    EE_BASE_API void CmdSetShadingRate( CommandBuffer* pCommandBuffer, ShadingRate shadingRate, Texture* pShadingRateTexture, ShadingRateCombiner postRasterizerCombiner, ShadingRateCombiner finalCombiner );
    EE_BASE_API void CmdSetViewport( CommandBuffer* pCommandBuffer, float x, float y, float width, float height, float minDepth, float maxDepth );
    EE_BASE_API void CmdSetScissor( CommandBuffer* pCommandBuffer, uint32_t x, uint32_t y, uint32_t width, uint32_t height );
    EE_BASE_API void CmdSetStencilReference( CommandBuffer* pCommandBuffer, uint32_t value );
    EE_BASE_API void CmdSetPipeline( CommandBuffer* pCommandBuffer, Pipeline* pPipeline );
    EE_BASE_API void CmdSetRootConstants( CommandBuffer* pCommandBuffer, uint32_t constantIndex, void const* pConstantData, size_t constantSize );
    EE_BASE_API void CmdSetRootParameter( CommandBuffer* pCommandBuffer, uint32_t parameterIndex, Buffer* pBuffer, size_t bufferOffset );
    EE_BASE_API void CmdSetIndexBuffer( CommandBuffer* pCommandBuffer, Buffer const* pIndexBuffer, IndexType indexType, uint64_t offset );
    EE_BASE_API void CmdDraw( CommandBuffer* pCommandBuffer, uint32_t numVertices, uint32_t firstVertex );
    EE_BASE_API void CmdDrawInstanced( CommandBuffer* pCommandBuffer, uint32_t numVertices, uint32_t numInstances, uint32_t firstVertex, uint32_t firstInstance );
    EE_BASE_API void CmdDrawIndexed( CommandBuffer* pCommandBuffer, uint32_t numIndices, uint32_t firstIndex );
    EE_BASE_API void CmdDrawIndexedInstanced( CommandBuffer* pCommandBuffer, uint32_t numIndices, uint32_t numInstances, uint32_t firstIndex, uint32_t firstInstance );
    EE_BASE_API void CmdDispatchCompute( CommandBuffer* pCommandBuffer, uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ );
    EE_BASE_API void CmdDispatchMesh( CommandBuffer* pCommandBuffer, uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ );
    EE_BASE_API void CmdDispatchRays( CommandBuffer* pCommandBuffer, RaytracingShaderTable* pShaderTable, AccelerationStructure* pAccelerationStructure, uint32_t width, uint32_t height );
    EE_BASE_API void CmdExecuteIndirect( CommandBuffer* pCommandBuffer, CommandSignature const* pCommandSignature, uint32_t maxNumCommands, Buffer const* pIndirectBuffer, uint64_t indirectBufferOffset, Buffer const* pCounterBuffer, uint64_t counterBufferOffset );
    EE_BASE_API void CmdClearTexture( CommandBuffer* pCommandBuffer, Texture const* pTexture, uint32_t clearValue );
    EE_BASE_API void CmdClearBuffer( CommandBuffer* pCommandBuffer, Buffer const* pBuffer, uint32_t clearValue );
    EE_BASE_API void CmdBuildAccelerationStructure( CommandBuffer* pCommandBuffer, TArrayView<AccelerationStructure* const> accelerationStructures, TArrayView<uint32_t const> bottomLevelAccelerationStructureIndices );
    EE_BASE_API void CmdBarrier( CommandBuffer* pCommandBuffer, TBitFlags<PipelineStage> sourceSync, TBitFlags<PipelineStage> destinationSync, TBitFlags<ResourceAccess> sourceAccess, TBitFlags<ResourceAccess> destinationAccess );
    EE_BASE_API void CmdBarrier( CommandBuffer* pCommandBuffer, Buffer* pBuffer, TBitFlags<PipelineStage> sourceSync, TBitFlags<PipelineStage> destinationSync, TBitFlags<ResourceAccess> sourceAccess, TBitFlags<ResourceAccess> destinationAccess );
    EE_BASE_API void CmdBarrier( CommandBuffer* pCommandBuffer, Texture* pTexture, TBitFlags<PipelineStage> sourceSync, TBitFlags<PipelineStage> destinationSync, TBitFlags<ResourceAccess> sourceAccess, TBitFlags<ResourceAccess> destinationAccess, TextureState sourceState, TextureState destinationState, TextureBarrierRegion region, TBitFlags<TextureBarrierFlags> flags );
    EE_BASE_API void CmdResetQueryPool( CommandBuffer* pCommandBuffer, QueryPool* pQueryPool, uint32_t startQuery, uint32_t numQueries );
    EE_BASE_API void CmdBeginQuery( CommandBuffer* pCommandBuffer, QueryPool* pQueryPool, uint32_t queryIndex );
    EE_BASE_API void CmdEndQuery( CommandBuffer* pCommandBuffer, QueryPool* pQueryPool, uint32_t queryIndex );
    EE_BASE_API void CmdResolveQuery( CommandBuffer* pCommandBuffer, QueryPool* pQueryPool, Buffer const* pReadbackBuffer, uint32_t startQuery, uint32_t numQueries );
    EE_BASE_API void CmdCopyBuffer( CommandBuffer* pCommandBuffer, Buffer const* pDstBuffer, uint64_t dstOffset, Buffer const* pSrcBuffer, uint64_t srcOffset, uint64_t srcSize );
    EE_BASE_API void CmdCopyTexture( CommandBuffer* pCommandBuffer, Texture const* pDstTexture, TextureCopyRegion const& dstRegion, Buffer const* pSrcBuffer, uint64_t srcOffset );
    EE_BASE_API void CmdCopyTexture( CommandBuffer* pCommandBuffer, Buffer const* pDstBuffer, uint64_t dstOffset, Texture const* pSrcTexture, TextureCopyRegion const& srcRegion );
    EE_BASE_API void CmdBeginDebugMarker( CommandBuffer* pCommandBuffer, char const* pName );
    EE_BASE_API void CmdEndDebugMarker( CommandBuffer* pCommandBuffer );
    EE_BASE_API uint32_t CmdWriteDebugMarker( CommandBuffer* pCommandBuffer, TBitFlags<MarkerTypeFlags> const& markerType, uint32_t markerValue, Buffer* pBuffer, size_t offset, bool useAutoFlags );

    EE_BASE_API CommandSignature* CreateCommandSignature( Context* pContext, CommandSignatureParameters const& parameters );
    EE_BASE_API void DestroyCommandSignature( Context* pContext, CommandSignature*&& pCommandSignature );

    EE_BASE_API AccelerationStructure* CreateAccelerationStructure( Context* pContext, AccelerationStructureTopLevelCreateParameters const& topLevelParameters, AccelerationStructureBottomLevelCreateParameters const& bottomLevelParameters );
    EE_BASE_API AccelerationStructureHandle GetAccelerationStructureHandle( AccelerationStructure const* pAccelerationStructure );

    EE_BASE_API Buffer* CreateBuffer( Context* pContext, BufferParameters const& parameters );
    EE_BASE_API void DestroyBuffer( Context* pContext, Buffer*&& buffer );
    EE_BASE_API void MapBuffer( Context* pContext, Buffer* pBuffer, ReadRange range );
    EE_BASE_API void UnmapBuffer( Context* pContext, Buffer* pBuffer );
    EE_BASE_API BufferHandle GetBufferHandle( Buffer const* pBuffer, DescriptorTypeFlags descriptorType );
    EE_BASE_API BufferSubAllocation BufferSubAllocate( Buffer* pBuffer, uint64_t size, uint64_t alignment );
    EE_BASE_API void BufferSubDeallocate( Buffer* pBuffer, BufferSubAllocation&& subAllocation );

    EE_BASE_API Texture* CreateTexture( Context* pContext, TextureParameters const& parameters );
    EE_BASE_API void DestroyTexture( Context* pContext, Texture*&& pTexture );
    EE_BASE_API uint32_t GetTextureCopyRowStride( Texture const* pTexture, uint32_t mipLevel, uint32_t arrayLayer );
    EE_BASE_API TextureHandle GetTextureHandle( Texture const* pTexture, DescriptorTypeFlags descriptorType, uint32_t rwTextureMipLevel );

    EE_BASE_API Sampler* CreateSampler( Context* pContext, SamplerParameters const& parameters );
    EE_BASE_API void DestroySampler( Context* pContext, Sampler*&& pSampler );
    EE_BASE_API SamplerStateHandle GetSamplerStateHandle( Sampler const* pSampler );

    EE_BASE_API Shader* CreateShader( Context* pContext, TInlineVector<ShaderByteCode, 2> const& shaderParameters );
    EE_BASE_API void DestroyShader( Context* pContext, Shader*&& pShader );

    EE_BASE_API RootSignature* CreateRootSignature( Context* pContext, RootSignatureParameters const& parameter );
    EE_BASE_API void DestroyRootSignature( Context* pContext, RootSignature*&& pRootSignature );

    EE_BASE_API PipelineCache* CreatePipelineCache( Context* pContext, PipelineCacheParameters const& parameters );
    EE_BASE_API void DestroyPipelineCache( Context* pContext, PipelineCache*&& pPipelineCache );
    EE_BASE_API TArrayView<uint8_t> GetPipelineCacheData( Context* pContext, PipelineCache* pPipelineCache );

    EE_BASE_API Pipeline* CreatePipeline( Context* pContext, GraphicsPipelineParameters const& parameters );
    EE_BASE_API Pipeline* CreatePipeline( Context* pContext, MeshPipelineParameters const& parameters );
    EE_BASE_API Pipeline* CreatePipeline( Context* pContext, ComputePipelineParameters const& parameters );
    EE_BASE_API Pipeline* CreatePipeline( Context* pContext, RaytracingPipelineParameters const& parameters );
    EE_BASE_API void DestroyPipeline( Context* pContext, Pipeline*&& pPipeline );

    EE_BASE_API QueryPool* CreateQueryPool( Context* pContext, QueryPoolParameters const& parameters );
    EE_BASE_API void DestroyQueryPool( Context* pContext, QueryPool*&& pQueryPool );
    EE_BASE_API double GetQueryTimestampFrequency( Queue* pQueue );

    EE_BASE_API void SetDebugName( Context* pContext, Queue* pQueue, StringView debugName );
    EE_BASE_API void SetDebugName( Context* pContext, QueryPool* pQueryPool, StringView debugName );
    EE_BASE_API void SetDebugName( Context* pContext, Buffer* pBuffer, StringView debugName );
    EE_BASE_API void SetDebugName( Context* pContext, Texture* pTexture, StringView debugName );
    EE_BASE_API void SetDebugName( Context* pContext, RootSignature* pRootSignature, StringView debugName );
    EE_BASE_API void SetDebugName( Context* pContext, CommandSignature* pCommandSignature, StringView debugName );
    EE_BASE_API void SetDebugName( Context* pContext, Pipeline* pPipeline, StringView debugName );
    EE_BASE_API void SetDebugName( Context* pContext, CommandPool* pCommandPool, StringView debugName );
    EE_BASE_API void SetDebugName( Context* pContext, CommandBuffer* pCommandBuffer, StringView debugName );

    EE_BASE_API void ReportDeviceMemoryLeaks();

    struct CommandBufferMarkerScope
    {
        RHI::CommandBuffer*                                 m_pCommandBuffer = nullptr;

        inline CommandBufferMarkerScope( RHI::CommandBuffer* pCommandBuffer, char const* pName ) : m_pCommandBuffer( pCommandBuffer )
        {
            CmdBeginDebugMarker( m_pCommandBuffer, pName );
        }

        inline ~CommandBufferMarkerScope()
        {
            CmdEndDebugMarker( m_pCommandBuffer );
        }
    };

    #if !EE_SHIPPING
    #define EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE(CB, Name) \
            EE_PROFILE_SCOPE_RENDER(Name); \
            EE::Render::RHI::CommandBufferMarkerScope EE_CONCATENATE(COMMAND_BUFFER_SCOPE_, __LINE__)( CB, Name );
    #else
    #define EE_RHI_COMMAND_BUFFER_PROFILE_SCOPE(CB, Name)
    #endif
}
