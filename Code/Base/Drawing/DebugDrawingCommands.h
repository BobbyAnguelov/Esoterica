#pragma once

#include "Base/_Module/API.h"
#include "Base/Math/Math.h"
#include "Base/Math/NumericRange.h"
#include "Base/Types/Arrays.h"
#include "Base/Types/String.h"
#include "Base/Types/BitFlags.h"
#include "Base/Types/Color.h"
#include "Base/Threading/Threading.h"
#include "Base/Math/Matrix.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    enum class DebugDrawLayer : uint8_t
    {
        World = 0,          // Tests and writes to the depth buffer
        WorldOverlay,       // Still tests the depth buffer but doesnt write (this semantic is just clearer for general engine code)
        Screen,             // Drawn using a separate depth buffer and on top of the rendered world
    };

    //-------------------------------------------------------------------------

    enum class DebugTextAlign : uint8_t
    {
        TopLeft = 0,
        TopCenter,
        TopRight,
        MiddleLeft,
        MiddleCenter,
        MiddleRight,
        BottomLeft,
        BottomCenter,
        BottomRight,
    };

    enum class DebugFont : uint8_t
    {
        Normal = 0,
        Small
    };

    enum class DebugMeshID : uint64_t
    {
        Invalid = 0,
        Box = 1,
        Sphere,
        Hemisphere,
        Cylinder,
        OpenCylinder,

        NumDebugDrawMeshIDs,
    };

    enum class DebugMeshStyle
    {
        Default = 0,    // Solid + Unlit
        Lit,            // Solid + Lit
        Wireframe       // Wireframe
    };
}

//-------------------------------------------------------------------------

namespace EE::DebugDrawInternal
{
    constexpr static uint64_t const g_InvalidHitTestID = UINT64_MAX;

    //-------------------------------------------------------------------------

    enum DebugCommandFlags : uint32_t
    {
        DebugCommandFlagPoint = 1U << 0U,
        DebugCommandFlagLine = 1U << 1U,
        DebugCommandFlagTriangle = 1U << 2U,

        DebugCommandFlagDisableAA = 1U << 10U,
    };

    //-------------------------------------------------------------------------

    struct PointCommand final
    {
        PointCommand( Float3 const& position, Float4 const& color, float pointThickness, Seconds TTL )
            : m_position( position )
            , m_thickness( pointThickness )
            , m_color( color )
            , m_TTL( TTL )
        {}

        EE_FORCE_INLINE bool IsTransparent() const { return m_color.IsTransparent(); }

        uint32_t    m_commandFlags = DebugCommandFlagPoint;
        Float3      m_position = Float3::Zero;
        float       m_thickness = 0.0F;
        Color       m_color = {};
        Seconds     m_TTL = 0;
        uint32_t    m_padding0 = 0;
        uint64_t    m_hitTestID = g_InvalidHitTestID;
        Int4        m_padding1 = Int4::Zero;
        Int2        m_padding2 = Int2::Zero;
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

        EE_FORCE_INLINE bool IsTransparent() const { return m_startColor.IsTransparent() || m_endColor.IsTransparent(); }

        uint32_t    m_commandFlags = DebugCommandFlagLine;
        Float3      m_startPosition = Float3::Zero;
        float       m_startThickness = 0.0F;
        Color       m_startColor = {};
        Float3      m_endPosition = Float3::Zero;
        float       m_endThickness = 0.0F;;
        Color       m_endColor = {};
        Seconds     m_TTL = 0;
        uint64_t    m_hitTestID = g_InvalidHitTestID;
        Int2        m_padding0 = Int2::Zero;
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

        EE_FORCE_INLINE bool IsTransparent() const { return m_color0.IsTransparent() || m_color1.IsTransparent() || m_color2.IsTransparent(); }

        uint32_t    m_commandFlags = DebugCommandFlagTriangle;
        Float3      m_vertex0 = Float3::Zero;
        Color       m_color0 = {};
        Float3      m_vertex1 = Float3::Zero;
        Color       m_color1 = {};
        Float3      m_vertex2 = Float3::Zero;
        Color       m_color2 = {};
        Seconds     m_TTL = 0;
        uint64_t    m_hitTestID = g_InvalidHitTestID;
    };

    //-------------------------------------------------------------------------

    struct TextCommand
    {
        TextCommand( Float2 const& position, char const* pText, Float4 const& color, DebugFont size, DebugTextAlign alignment, bool background, Seconds TTL )
            : m_color( color )
            , m_position( position.m_x, position.m_y, 0 )
            , m_font( size )
            , m_alignment( alignment )
            , m_isScreenText( true )
            , m_hasBackground( background )
            , m_TTL( TTL )
            , m_text( pText )
        {}

        TextCommand( Float3 const& position, char const* pText, Float4 const& color, DebugFont size, DebugTextAlign alignment, bool background, Seconds TTL )
            : m_color( color )
            , m_position( position )
            , m_font( size )
            , m_alignment( alignment )
            , m_isScreenText( false )
            , m_hasBackground( background )
            , m_TTL( TTL )
            , m_text( pText )
        {}

        EE_FORCE_INLINE bool IsTransparent() const { return m_color.IsTransparent(); }

        Color             m_color;
        Float3            m_position;
        DebugFont         m_font;
        DebugTextAlign    m_alignment;
        bool              m_isScreenText;
        bool              m_hasBackground;
        Seconds           m_TTL = 0;
        TInlineString<24> m_text;
        uint64_t          m_hitTestID = g_InvalidHitTestID;
    };

    //-------------------------------------------------------------------------

    struct MeshCommand
    {
        MeshCommand( uint64_t meshID, Matrix const& transform, Float4 const& tintColor, DebugMeshStyle style, Seconds TTL )
            : m_meshID( meshID )
            , m_isWireframe( style == DebugMeshStyle::Wireframe )
            , m_useFakeLighting( style == DebugMeshStyle::Lit )
            , m_transform( transform )
            , m_tintColor( tintColor )
            , m_TTL( TTL )
        {}

        EE_FORCE_INLINE bool IsTransparent() const { return m_tintColor.IsTransparent(); }

        uint64_t          m_meshID = 0;
        bool              m_isWireframe = false;
        bool              m_useFakeLighting = false;
        Matrix            m_transform = Matrix::Identity;
        Color             m_tintColor;
        Seconds           m_TTL = 0;
        uint64_t          m_hitTestID = g_InvalidHitTestID;
    };

    //-------------------------------------------------------------------------

    struct EE_BASE_API CommandBuffer
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

            m_meshCommands.reserve( m_meshCommands.size() + buffer.m_meshCommands.size() );
            m_meshCommands.insert( m_meshCommands.end(), buffer.m_meshCommands.begin(), buffer.m_meshCommands.end() );
        }

        inline void Clear()
        {
            m_pointCommands.clear();
            m_lineCommands.clear();
            m_triangleCommands.clear();
            m_textCommands.clear();
            m_meshCommands.clear();
        }

        inline bool IsEmpty() const
        {
            return m_pointCommands.empty() && m_lineCommands.empty() && m_triangleCommands.empty() && m_textCommands.empty() && m_meshCommands.empty();
        }

        void Reset( Seconds deltaTime );

    public:

        TAlignedVector<PointCommand>    m_pointCommands;
        TAlignedVector<LineCommand>     m_lineCommands;
        TAlignedVector<TriangleCommand> m_triangleCommands;
        TAlignedVector<TextCommand>     m_textCommands;
        TAlignedVector<MeshCommand>     m_meshCommands;
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

        EE_FORCE_INLINE void AddCommand( PointCommand&& cmd, DebugDrawLayer layer )
        {
            CommandBuffer* pBuffer = GetCommandBuffer( layer );
            cmd.m_hitTestID = m_hitTestID;
            pBuffer->m_pointCommands.emplace_back( eastl::move( cmd ) );
        }

        EE_FORCE_INLINE void AddCommand( LineCommand&& cmd, DebugDrawLayer layer )
        {
            CommandBuffer* pBuffer = GetCommandBuffer( layer );
            cmd.m_hitTestID = m_hitTestID;
            pBuffer->m_lineCommands.emplace_back( eastl::move( cmd ) );
        }

        EE_FORCE_INLINE void AddCommand( TriangleCommand&& cmd, DebugDrawLayer layer )
        {
            CommandBuffer* pBuffer = GetCommandBuffer( layer );
            cmd.m_hitTestID = m_hitTestID;
            pBuffer->m_triangleCommands.emplace_back( eastl::move( cmd ) );
        }

        EE_FORCE_INLINE void AddCommand( TextCommand&& cmd, DebugDrawLayer layer )
        {
            CommandBuffer* pBuffer = GetCommandBuffer( layer );
            cmd.m_hitTestID = m_hitTestID;
            pBuffer->m_textCommands.emplace_back( eastl::move( cmd ) );
        }

        EE_FORCE_INLINE void AddCommand( MeshCommand&& cmd, DebugDrawLayer layer )
        {
            CommandBuffer* pBuffer = GetCommandBuffer( layer );
            cmd.m_hitTestID = m_hitTestID;
            pBuffer->m_meshCommands.emplace_back( eastl::move( cmd ) );
        }

        inline void Clear()
        {
            m_transparentDepthOnWrite.Clear();
            m_transparentDepthOnNoWrite.Clear();
            m_transparentDepthSeparateWrite.Clear();
        }

        CommandBuffer const& GetTransparentDepthOnWrite() const { return m_transparentDepthOnWrite; }
        CommandBuffer const& GetTransparentDepthOnNoWrite() const { return m_transparentDepthOnNoWrite; }
        CommandBuffer const& GetTransparentDepthSeparateWrite() const { return m_transparentDepthSeparateWrite; }

        inline void SetHitTestID( uint64_t ID )
        {
            EE_ASSERT( m_hitTestID == g_InvalidHitTestID );
            m_hitTestID = ID;
        }

        inline uint64_t GetActiveHitTestID() const { return m_hitTestID; }

        void ClearHitTestID()
        {
            EE_ASSERT( m_hitTestID != g_InvalidHitTestID );
            m_hitTestID = g_InvalidHitTestID;
        }

    private:

        inline CommandBuffer* GetCommandBuffer( DebugDrawLayer layer )
        {
            switch ( layer )
            {
                case DebugDrawLayer::WorldOverlay: return &m_transparentDepthOnNoWrite;
                case DebugDrawLayer::World: return &m_transparentDepthOnWrite;
                case DebugDrawLayer::Screen: return &m_transparentDepthSeparateWrite;
            };

            return nullptr;
        }

    private:

        Threading::ThreadID m_ID;
        uint64_t            m_hitTestID = g_InvalidHitTestID;
        CommandBuffer       m_transparentDepthOnWrite;
        CommandBuffer       m_transparentDepthOnNoWrite;
        CommandBuffer       m_transparentDepthSeparateWrite;
    };

    //-------------------------------------------------------------------------
    // Frame Command Buffer
    //-------------------------------------------------------------------------
    // This contains all the commands we need to actually draw this frame
    // Any command with a TTL, will be left in this buffer at the end of the frame to be drawn again

    class FrameCommandBuffer
    {
    public:

        void AddThreadCommands( ThreadCommandBuffer const& threadCommands );

        // Empties the command buffer ignoring any TTL state
        inline void Clear()
        {
            m_transparentDepthOnWrite.Clear();
            m_transparentDepthOnNoWrite.Clear();
            m_transparentDepthSeparateWrite.Clear();
        }

        // Resets the buffer, will remove all commands with an expired TTL
        inline void Reset( Seconds deltaTime )
        {
            m_transparentDepthOnWrite.Reset( deltaTime );
            m_transparentDepthOnNoWrite.Reset( deltaTime );
            m_transparentDepthSeparateWrite.Reset( deltaTime );
        }

        inline bool IsEmpty() const
        {
            return
                m_transparentDepthOnWrite.IsEmpty() &&
                m_transparentDepthOnNoWrite.IsEmpty() &&
                m_transparentDepthSeparateWrite.IsEmpty();
        }

    public:

        CommandBuffer m_transparentDepthOnWrite;
        CommandBuffer m_transparentDepthOnNoWrite;
        CommandBuffer m_transparentDepthSeparateWrite;
    };
}
#endif
