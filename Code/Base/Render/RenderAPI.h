#pragma once

#include "Base/Esoterica.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    enum class DataSemantic : uint8_t
    {
        Position = 0,
        Normal,
        Tangent,
        BiTangent,
        Color,
        TexCoord,
        BlendIndex,
        BlendWeight,

        None,
    };

    enum class DataFormat : uint8_t
    {
        Unknown = 0,
        UInt_R8,
        UInt_R8G8,
        UInt_R8G8B8A8,

        UNorm_R8,
        UNorm_R8G8,
        UNorm_R8G8B8A8,

        UInt_R32,
        UInt_R32G32,
        UInt_R32G32B32,
        UInt_R32G32B32A32,

        SInt_R32,
        SInt_R32G32,
        SInt_R32G32B32,
        SInt_R32G32B32A32,

        Float_R16,
        Float_R16G16,
        Float_R16G16B16A16,

        Float_R32,
        Float_R32G32,
        Float_R32G32B32,
        Float_R32G32B32A32,

        // Special case format that changes based on texture usage
        Float_X32,

        Count,
    };

    enum Usage : uint8_t
    {
        USAGE_UAV = 1 << 0,
        USAGE_SRV = 1 << 1,
        USAGE_RT_DS = 1 << 2,
        USAGE_STAGING = 1 << 3,
    };

    enum class TextureFiltering : uint8_t
    {
        MinMagMipPoint = 0,
        MinMagPointMipLinear,
        MinPointMagLinearMipPoint,
        MinPointMagMipLinear,
        MinLinearMagMipPoint,
        MinLinearMagPointMipLinear,
        MinMagLinearMipPoint,
        MinMagMipLinear,
        Anisotropic,
        ComparisonMinMagMipPoint,
        ComparisonMinMagPointMipLinear,
        ComparisonMinPointMagLinearMipPoint,
        ComparisonMinPointMagMipLinear,
        ComparisonMinLinearMagMipPoint,
        ComparisonMinLinearMagPointMipLinear,
        ComparisonMinMagLinearMipPoint,
        ComparisonMinMagMipLinear,
        ComparisonAnisotropic,
        MinimumMinMagMipPoint,
        MinimumMinMagPointMipLinear,
        MinimumMinPointMagLinearMipPoint,
        MinimumMinPointMagMipLinear,
        MinimumMinLinearMagMipPoint,
        MinimumMinLinearMagPointMipLinear,
        MinimumMinMagLinearMipPoint,
        MinimumMinMagMipLinear,
        MinimumAnisotropic,
        MaximumMinMagMipPoint,
        MaximumMinMagPointMipLinear,
        MaximumMinPointMagLinearMipPoint,
        MaximumMinPointMagMipLinear,
        MaximumMinLinearMagMipPoint,
        MaximumMinLinearMagPointMipLinear,
        MaximumMinMagLinearMipPoint,
        MaximumMinMagMipLinear,
        MaximumAnisotropic,
    };

    enum class TextureAddressMode : uint8_t
    {
        Wrap = 0,
        Mirror,
        Clamp,
        Border,
    };

    enum class Topology : uint8_t
    {
        PointList = 0,
        LineList,
        LineStrip,
        TriangleList,
        TriangleStrip,

        None,
    };

    enum class WindingMode : uint8_t
    {
        Clockwise = 0,
        CounterClockwise,
    };

    enum class CullMode : uint8_t
    {
        BackFace = 0,
        FrontFace,
        None,
    };

    enum class FillMode : uint8_t
    {
        Solid = 0,
        Wireframe,
    };

    enum class BlendValue : uint8_t
    {
        Zero = 0,
        One,
        SourceColor,
        InverseSourceColor,
        SourceAlpha,
        InverseSourceAlpha,
        DestinationColor,
        InverseDestinationColor,
        DestinationAlpha,
        InverseDestinationAlpha,
        SourceAlphaSaturated,
        BlendFactor,
        InverseBlendFactor,
        Source1Color,
        InverseSource1Color,
        Source1Alpha,
        InverseSource1Alpha,
    };

    enum class BlendOp : uint8_t
    {
        Add = 0,
        SourceMinusDestination,
        DestinationMinusSource,
        Min,
        Max,
    };

    enum class DepthTestMode : uint8_t
    {
        On = 0,
        Off,
        ReadOnly,
    };

    enum class ResourceType : uint8_t
    {
        None,
        Buffer,
        Shader,

        ShaderInputBinding,
        BlendState,
        RasterizerState,
        SamplerState,
        Texture,
        TextureUAV,
        RenderTarget,
        ViewUAV,
        ViewSRV,
        ViewRT,
        ViewDS,

        numTypes,
    };

    enum class PipelineStage : uint8_t
    {
        Vertex = 0,
        Geometry,
        Pixel,
        Hull,
        Domain,
        Compute,

        None,
    };

    struct ScissorRect
    {
        int32_t m_left;
        int32_t m_top;
        int32_t m_right;
        int32_t m_bottom;
    };

    //-------------------------------------------------------------------------

    template <ResourceType T>
    struct ObjectHandle
    {
    public:

        ObjectHandle() = default;

        inline ~ObjectHandle()
        {
            EE_ASSERT( m_pData == nullptr );
        }

        inline bool IsValid() const
        {
            return m_pData != nullptr;
        }

        inline void Reset()
        {
            m_pData = nullptr;
        }

        inline bool operator==( ObjectHandle const& rhs ) const { return m_pData == rhs.m_pData; }
        inline bool operator!=( ObjectHandle const& rhs ) const { return m_pData != rhs.m_pData; }

    public:

        void*           m_pData = nullptr;         // Ptr to the actual API type
    };

    //-------------------------------------------------------------------------

    typedef ObjectHandle<ResourceType::Buffer>                BufferHandle;
    typedef ObjectHandle<ResourceType::Texture>               TextureHandle;
    typedef ObjectHandle<ResourceType::RenderTarget>          RenderTargetHandle;
    typedef ObjectHandle<ResourceType::Shader>                ShaderHandle;
    typedef ObjectHandle<ResourceType::ShaderInputBinding>    ShaderInputBindingHandle;
    typedef ObjectHandle<ResourceType::BlendState>            BlendStateHandle;
    typedef ObjectHandle<ResourceType::RasterizerState>       RasterizerStateHandle;
    typedef ObjectHandle<ResourceType::SamplerState>          SamplerStateHandle;
    typedef ObjectHandle<ResourceType::ViewUAV>               ViewUAVHandle;
    typedef ObjectHandle<ResourceType::ViewSRV>               ViewSRVHandle;
    typedef ObjectHandle<ResourceType::ViewRT>                ViewRTHandle;
    typedef ObjectHandle<ResourceType::ViewDS>                ViewDSHandle;
}