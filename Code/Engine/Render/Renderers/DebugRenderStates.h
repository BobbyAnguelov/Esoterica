#pragma once

#include "Engine/_Module/API.h"
#include "System/Render/RenderDevice.h"
#include "Engine/Render/RenderViewport.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Render
{
    //-------------------------------------------------------------------------
    // Primitives
    //-------------------------------------------------------------------------

    struct EE_ENGINE_API DebugLineRenderState
    {
        constexpr static uint32_t const MaxLinesPerDrawCall = 100000;

    public:

        bool Initialize( RenderDevice* pRenderDevice );
        void Shutdown( RenderDevice* pRenderDevice );
        void SetState( RenderContext const& renderContext, Viewport const& viewport );

    public:

        VertexShader                    m_vertexShader;
        GeometryShader                  m_geometryShader;
        PixelShader                     m_pixelShader;
        ShaderInputBindingHandle        m_inputBinding;
        VertexBuffer                    m_vertexBuffer;
        BlendState                      m_blendState;
        RasterizerState                 m_rasterizerState;
        Blob                            m_stagingVertexData;

        PipelineState                   m_PSO;
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINE_API DebugPointRenderState
    {
        constexpr static uint32_t const MaxPointsPerDrawCall = 100000;

    public:

        bool Initialize( RenderDevice* pRenderDevice );
        void Shutdown( RenderDevice* pRenderDevice );
        void SetState( RenderContext const& renderContext, Viewport const& viewport );

    public:

        VertexShader                    m_vertexShader;
        GeometryShader                  m_geometryShader;
        PixelShader                     m_pixelShader;
        ShaderInputBindingHandle        m_inputBinding;
        VertexBuffer                    m_vertexBuffer;
        BlendState                      m_blendState;
        RasterizerState                 m_rasterizerState;
        Blob                            m_stagingVertexData;

        PipelineState                   m_PSO;
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINE_API DebugPrimitiveRenderState
    {
        constexpr static uint32_t const MaxTrianglesPerDrawCall = 100000;

    public:

        bool Initialize( RenderDevice* pRenderDevice );
        void Shutdown( RenderDevice* pRenderDevice );
        void SetState( RenderContext const& renderContext, Viewport const& viewport );

    public:

        VertexShader                    m_vertexShader;
        PixelShader                     m_pixelShader;
        ShaderInputBindingHandle        m_inputBinding;
        VertexBuffer                    m_vertexBuffer;
        BlendState                      m_blendState;
        RasterizerState                 m_rasterizerState;
        Blob                            m_stagingVertexData;

        PipelineState                   m_PSO;
    };

    //-------------------------------------------------------------------------
    // Text
    //-------------------------------------------------------------------------

    struct DebugFontGlyph
    {
        Float2                          m_positionTL;
        Float2                          m_positionBR;
        Float2                          m_texCoordsTL;
        Float2                          m_texCoordsBR;
        float                           m_advanceX;
    };

    struct DebugFontGlyphVertex
    {
        Float2                          m_pos;
        Float2                          m_texcoord;
        uint32_t                        m_color;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API DebugTextFontAtlas
    {

    public:

        struct FontDesc
        {
            uint8_t const*              m_compressedFontData;
            float                       m_fontSize;
        };

    private:

        struct FontInfo
        {

        public:

            inline float GetAscent() const { return m_ascent; }
            inline float GetDescent() const { return m_descent; }
            inline float GetLineGap() const { return m_lineGap; }
            inline IntRange GetValidGlyphRange() const { return IntRange( 0x20, 0xFF ); }
            inline int32_t GetGlyphIndex( char c ) const { EE_ASSERT( GetValidGlyphRange().ContainsInclusive( c ) ); return c - GetValidGlyphRange().m_begin; }
            inline DebugFontGlyph const& GetGlyph( int32_t glyphIdx ) const { return m_glyphs[glyphIdx]; }

        public:

            TVector<DebugFontGlyph>     m_glyphs;
            float                       m_ascent = 0.0f;
            float                       m_descent = 0.0f;
            float                       m_lineGap = 0.0f;
        };

    public:

        // Generate a font atlas with the specified fonts
        bool Generate( FontDesc* pFonts, int32_t numFonts );

        // Returns the 2D dimensions of the atlas data
        inline Int2 GetDimensions() const { return Int2( 512 ); }

        // Returns the raw font atlas bitmap data
        inline uint8_t const* GetAtlasData() const { return m_atlasData.data(); }

        // Get a list of glyphs indices that correspond to the supplied string
        void GetGlyphsForString( uint32_t fontIdx, TInlineString<24> const& str, TInlineVector<int32_t, 100>& outGlyphIndices ) const;

        // Get a list of glyphs indices that correspond to the supplied string
        void GetGlyphsForString( uint32_t fontIdx, char const* pStr, TInlineVector<int32_t, 100>& outGlyphIndices ) const;

        // Get the 2D pixel size of the string if it were rendered
        Int2 GetTextExtents( uint32_t fontIdx, char const* pText ) const;

        // Fill the supplied vertex and index buffer with the necessary data to render the supplied glyphs
        uint32_t WriteGlyphsToBuffer( DebugFontGlyphVertex* pVertexBuffer, uint16_t indexStartOffset, uint16_t* pIndexBuffer, uint32_t fontIdx, TInlineVector<int32_t, 100> const& glyphIndices, Float2 const& textPosTopLeft, Float4 const& color ) const;

        // Writes a glyph with custom texture coords to the render buffers
        void WriteCustomGlyphToBuffer( DebugFontGlyphVertex* pVertexBuffer, uint16_t indexStartOffset, uint16_t* pIndexBuffer, uint32_t fontIdx, int32_t firstGlyphIdx, Float2 const& texCoords, Float2 const& baselinePos, Int2 const& textExtents, int32_t pixelPadding, Float4 const& color ) const;

    private:

        Blob                   m_atlasData;
        TInlineVector<FontInfo, 3>      m_fonts;
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINE_API DebugTextRenderState
    {
        constexpr static int32_t const MaxGlyphsPerDrawCall = 10000;
        static Float4 const ClipSpaceTopLeft;

    public:

        bool Initialize( RenderDevice* pRenderDevice );
        void Shutdown( RenderDevice* pRenderDevice );
        void SetState( RenderContext const& renderContext, Viewport const& viewport );

    public:

        VertexShader                    m_vertexShader;
        PixelShader                     m_pixelShader;
        ShaderInputBindingHandle        m_inputBinding;
        VertexBuffer                    m_vertexBuffer;
        VertexBuffer                    m_indexBuffer;
        SamplerState                    m_samplerState;
        BlendState                      m_blendState;
        RasterizerState                 m_rasterizerState;

        PipelineState                   m_PSO;

        DebugTextFontAtlas              m_fontAtlas;
        Texture                         m_fontAtlasTexture;
        Float2                          m_nonZeroAlphaTexCoords;
    };
}
#endif