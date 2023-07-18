#pragma once

#include "../_Module/API.h"
#include "Base/Math/Math.h"
#include "Base/Math/NumericRange.h"
#include "Base/Types/Arrays.h"
#include "Base/Types/String.h"
#include "Base/Types/BitFlags.h"
#include "Base/Threading/Threading.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Drawing
{
    enum DepthTestState : uint8_t
    {
        EnableDepthTest,
        DisableDepthTest
    };

    //-------------------------------------------------------------------------

    enum TextAlignment : uint8_t
    {
        AlignTopLeft = 0,
        AlignTopCenter,
        AlignTopRight,
        AlignMiddleLeft,
        AlignMiddleCenter,
        AlignMiddleRight,
        AlignBottomLeft,
        AlignBottomCenter,
        AlignBottomRight,
    };

    enum FontSize : uint8_t
    {
        FontNormal,
        FontSmall
    };

    //-------------------------------------------------------------------------

    struct PointCommand
    {
        PointCommand( Float3 const& position, Float4 const& color, float pointThickness, Seconds TTL )
            : m_position( position )
            , m_thickness( pointThickness )
            , m_color( color )
            , m_TTL( TTL )
        {}

        EE_FORCE_INLINE bool IsTransparent() const { return m_color[3] != 1.0f; }

        Float3      m_position;
        float       m_thickness;
        Float4      m_color;
        Seconds     m_TTL;
    };

    //-------------------------------------------------------------------------

    struct LineCommand
    {
        LineCommand( Float3 const& startPosition, Float3 const& endPosition, Float4 const& color, float lineThickness, Seconds TTL )
            : m_startPosition( startPosition )
            , m_startThickness( lineThickness )
            , m_startColor( color )
            , m_endPosition( endPosition )
            , m_endThickness( lineThickness )
            , m_endColor( color )
            , m_TTL( TTL )
        {}

        LineCommand( Float3 const& startPosition, Float3 const& endPosition, Float4 const& startColor, Float4 const& endColor, float lineThickness, Seconds TTL )
            : m_startPosition( startPosition )
            , m_startThickness( lineThickness )
            , m_startColor( startColor )
            , m_endPosition( endPosition )
            , m_endThickness( lineThickness )
            , m_endColor( endColor )
            , m_TTL( TTL )
        {}

        LineCommand( Float3 const& startPosition, Float3 const& endPosition, Float4 const& color, float startThickness, float endThickness, Seconds TTL )
            : m_startPosition( startPosition )
            , m_startThickness( startThickness )
            , m_startColor( color )
            , m_endPosition( endPosition )
            , m_endThickness( endThickness )
            , m_endColor( color )
            , m_TTL( TTL )
        {}

        LineCommand( Float3 const& startPosition, Float3 const& endPosition, Float4 const& startColor, Float4 const& endColor, float startThickness, float endThickness, Seconds TTL )
            : m_startPosition( startPosition )
            , m_startThickness( startThickness )
            , m_startColor( startColor )
            , m_endPosition( endPosition )
            , m_endThickness( endThickness )
            , m_endColor( endColor )
            , m_TTL( TTL )
        {}

        EE_FORCE_INLINE bool IsTransparent() const { return m_startColor[3] != 1.0f || m_endColor[3] != 1.0f; }

        Float3      m_startPosition;
        float       m_startThickness;
        Float4      m_startColor;
        float       m_padding; // Needed for VB upload - since each command is 2 vertices
        Float3      m_endPosition;
        float       m_endThickness;
        Float4      m_endColor;
        Seconds     m_TTL;
    };

    //-------------------------------------------------------------------------

    struct TriangleCommand
    {
        TriangleCommand( Float3 const& V0, Float3 const& V1, Float3 const& V2, Float4 const& color, Seconds TTL )
            : m_vertex0( V0 )
            , m_color0( color )
            , m_vertex1( V1 )
            , m_color1( color )
            , m_vertex2( V2 )
            , m_color2( color )
            , m_TTL( TTL )
        {}

        TriangleCommand( Float3 const& V0, Float3 const& V1, Float3 const& V2, Float4 const& color0, Float4 const& color1, Float4 const& color2, Seconds TTL )
            : m_vertex0( V0 )
            , m_color0( color0 )
            , m_vertex1( V1 )
            , m_color1( color1 )
            , m_vertex2( V2 )
            , m_color2( color2 )
            , m_TTL( TTL )
        {}

        EE_FORCE_INLINE bool IsTransparent() const { return m_color0[3] != 1.0f || m_color1[3] != 1.0f || m_color2[3] != 1.0f; }

        Float4      m_vertex0;
        Float4      m_color0;
        float       m_padding0; // Needed for VB upload - since each command is 3 vertices
        Float4      m_vertex1;
        Float4      m_color1;
        float       m_padding1; // Needed for VB upload - since each command is 3 vertices
        Float4      m_vertex2;
        Float4      m_color2;
        Seconds     m_TTL;
    };

    //-------------------------------------------------------------------------

    struct TextCommand
    {
        TextCommand( Float2 const& position, char const* pText, Float4 const& color, FontSize size, TextAlignment alignment, bool background, Seconds TTL )
            : m_color( color )
            , m_position( position.m_x, position.m_y, 0 )
            , m_fontSize( size )
            , m_alignment( alignment )
            , m_isScreenText( true )
            , m_hasBackground( background )
            , m_text( pText )
            , m_TTL( TTL )
        {}

        TextCommand( Float3 const& position, char const* pText, Float4 const& color, FontSize size, TextAlignment alignment, bool background, Seconds TTL )
            : m_color( color )
            , m_position( position )
            , m_fontSize( size )
            , m_alignment( alignment )
            , m_isScreenText( false )
            , m_hasBackground( background )
            , m_text( pText )
            , m_TTL( TTL )
        {}

        EE_FORCE_INLINE bool IsTransparent() const { return m_color[3] != 1.0f; }

        Float4                      m_color;
        Float3                      m_position;
        FontSize                    m_fontSize;
        TextAlignment               m_alignment;
        bool                        m_isScreenText;
        bool                        m_hasBackground;
        TInlineString<24>           m_text;
        Seconds                     m_TTL;
    };

    //-------------------------------------------------------------------------

    struct CommandBuffer
    {
        inline void Append( CommandBuffer const& buffer )
        {
            m_pointCommands.reserve( m_pointCommands.size() + buffer.m_pointCommands.size() );
            m_pointCommands.insert( m_pointCommands.end(), buffer.m_pointCommands.begin(), buffer.m_pointCommands.end() );

            m_lineCommands.reserve( m_lineCommands.size() + buffer.m_lineCommands.size() );
            m_lineCommands.insert( m_lineCommands.end(), buffer.m_lineCommands.begin(), buffer.m_lineCommands.end() );

            m_triangleCommands.reserve( m_triangleCommands.size() + buffer.m_triangleCommands.size() );
            m_triangleCommands.insert( m_triangleCommands.end(), buffer.m_triangleCommands.begin(), buffer.m_triangleCommands.end() );

            m_textCommands.reserve( m_textCommands.size() + buffer.m_textCommands.size() );
            m_textCommands.insert( m_textCommands.end(), buffer.m_textCommands.begin(), buffer.m_textCommands.end() );
        }

        inline void Clear()
        {
            m_pointCommands.clear();
            m_lineCommands.clear();
            m_triangleCommands.clear();
            m_textCommands.clear();
        }

        void Reset( Seconds deltaTime );

    public:

        TVector<PointCommand>       m_pointCommands;
        TVector<LineCommand>        m_lineCommands;
        TVector<TriangleCommand>    m_triangleCommands;
        TVector<TextCommand>        m_textCommands;
    };

    //-------------------------------------------------------------------------
    // Per-Thread command buffer
    //-------------------------------------------------------------------------
    // These are fully cleared each frame

    class ThreadCommandBuffer
    {

    public:

        ThreadCommandBuffer( Threading::ThreadID threadID )
            : m_ID( threadID )
        {}

        inline Threading::ThreadID GetThreadID() const { return m_ID; }

        EE_FORCE_INLINE void AddCommand( PointCommand&& cmd, DepthTestState depthTestState )
        {
            CommandBuffer* pBuffer = GetCommandBuffer( depthTestState, cmd.IsTransparent() );
            pBuffer->m_pointCommands.emplace_back( eastl::move( cmd ) );
        }

        EE_FORCE_INLINE void AddCommand( LineCommand&& cmd, DepthTestState depthTestState )
        {
            CommandBuffer* pBuffer = GetCommandBuffer( depthTestState, cmd.IsTransparent() );
            pBuffer->m_lineCommands.emplace_back( eastl::move( cmd ) );
        }

        EE_FORCE_INLINE void AddCommand( TriangleCommand&& cmd, DepthTestState depthTestState )
        {
            CommandBuffer* pBuffer = GetCommandBuffer( depthTestState, cmd.IsTransparent() );
            pBuffer->m_triangleCommands.emplace_back( eastl::move( cmd ) );
        }

        EE_FORCE_INLINE void AddCommand( TextCommand&& cmd, DepthTestState depthTestState )
        {
            CommandBuffer* pBuffer = GetCommandBuffer( depthTestState, cmd.IsTransparent() );
            pBuffer->m_textCommands.emplace_back( eastl::move( cmd ) );
        }

        inline void Clear()
        {
            m_opaqueDepthOn.Clear();
            m_opaqueDepthOff.Clear();
            m_transparentDepthOn.Clear();
            m_transparentDepthOff.Clear();
        }

        CommandBuffer const& GetOpaqueDepthTestEnabledBuffer() const { return m_opaqueDepthOn; }
        CommandBuffer const& GetOpaqueDepthTestDisabledBuffer() const { return m_opaqueDepthOff; }
        CommandBuffer const& GetTransparentDepthTestEnabledBuffer() const { return m_transparentDepthOn; }
        CommandBuffer const& GetTransparentDepthTestDisabledBuffer() const { return m_transparentDepthOff; }

    private:

        inline CommandBuffer* GetCommandBuffer( DepthTestState depthTestState, bool isTransparent )
        {
            CommandBuffer* pBuffer = nullptr;

            if ( depthTestState == DepthTestState::EnableDepthTest )
            {
                pBuffer = isTransparent ? &m_transparentDepthOn : &m_opaqueDepthOn;
            }
            else // Disable depth test
            {
                pBuffer = isTransparent ? &m_transparentDepthOff : &m_opaqueDepthOff;
            }

            return pBuffer;
        }

    private:

        Threading::ThreadID         m_ID;
        CommandBuffer               m_opaqueDepthOn;
        CommandBuffer               m_opaqueDepthOff;
        CommandBuffer               m_transparentDepthOn;
        CommandBuffer               m_transparentDepthOff;
    };

    //-------------------------------------------------------------------------
    // Frame Buffer
    //-------------------------------------------------------------------------
    // This contains all the commands we need to actually draw this frame
    // Any command with a TTL, will be left in this buffer at the end of the frame to be drawn again

    class EE_BASE_API FrameCommandBuffer
    {
    public:

        void AddThreadCommands( ThreadCommandBuffer const& threadCommands );

        // Empties the command buffer ignoring any TTL state
        inline void Clear()
        {
            m_opaqueDepthOn.Clear();
            m_opaqueDepthOff.Clear();
            m_transparentDepthOn.Clear();
            m_transparentDepthOff.Clear();
        }

        // Resets the buffer, will remove all commands with an expired TTL
        inline void Reset( Seconds deltaTime )
        {
            m_opaqueDepthOn.Reset( deltaTime );
            m_opaqueDepthOff.Reset( deltaTime );
            m_transparentDepthOn.Reset( deltaTime );
            m_transparentDepthOff.Reset( deltaTime );
        }

    public:

        CommandBuffer               m_opaqueDepthOn;
        CommandBuffer               m_opaqueDepthOff;
        CommandBuffer               m_transparentDepthOn;
        CommandBuffer               m_transparentDepthOff;
    };
}
#endif