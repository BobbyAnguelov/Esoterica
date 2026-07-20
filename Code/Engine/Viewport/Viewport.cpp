#include "Viewport.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------

namespace EE
{
    Viewport::Viewport( Int2 const& topLeft, Int2 const& dimensions, Math::ViewVolume const& viewVolume )
        : m_topLeftPosition( topLeft )
        , m_size( dimensions )
    {
        EE_ASSERT( IsValid() );
        SetViewVolume( viewVolume );
    }

    Viewport::~Viewport()
    {
        EE_ASSERT( m_settings.empty() );
    }

    Viewport& Viewport::operator=( Viewport&& rhs )
    {
        m_ID = rhs.m_ID;
        m_viewVolume = rhs.m_viewVolume;
        m_topLeftPosition = rhs.m_topLeftPosition;
        m_size = rhs.m_size;

        // Do not copy visualization or debug draw system

        return *this;
    }

    Viewport& Viewport::operator=( Viewport const& rhs )
    {
        m_ID = rhs.m_ID;
        m_viewVolume = rhs.m_viewVolume;
        m_topLeftPosition = rhs.m_topLeftPosition;
        m_size = rhs.m_size;

        // Do not copy visualization or debug draw system

        return *this;
    }

    void Viewport::Resize( Int2 const& topLeftPosition, Int2 const& dimensions )
    {
        m_size = Float2( dimensions );
        m_topLeftPosition = Float2( topLeftPosition );
        EE_ASSERT( IsValid() );
    }

    void Viewport::Resize( Math::Rectangle const& rect )
    {
        m_size = rect.GetSize();
        m_topLeftPosition = rect.GetTL();
        EE_ASSERT( IsValid() );
    }

    void Viewport::SetViewVolume( Math::ViewVolume const& viewVolume )
    {
        m_viewVolume = viewVolume;

        // Update the aspect ratio
        float const aspectRatio = m_size.m_x / m_size.m_y;
        if ( m_viewVolume.GetAspectRatio() != aspectRatio )
        {
            m_viewVolume.SetAspectRatio( aspectRatio );
        }
    }

    //-------------------------------------------------------------------------

    Vector Viewport::WorldSpaceToClipSpace( Vector const& pointWS ) const
    {
        Vector pointCS = m_viewVolume.GetViewProjectionMatrix().TransformVector3( pointWS );
        Vector const W = pointCS.GetSplatW().GetAbs();
        if ( !W.IsZero4() )
        {
            pointCS /= W;
        }

        return pointCS;
    }

    Float2 Viewport::ClipSpaceToScreenSpace( Vector const& pointCS ) const
    {
        // Convert from [-1,1] to [0,1]
        float m_x = ( pointCS.GetX() + 1 ) / 2;
        float m_y = ( pointCS.GetY() + 1 ) / 2;

        // Invert Y since screen space origin (0,0) is the top left and in CS it is the bottom right
        m_y = 1.0f - m_y;

        // Convert to pixels based on viewport dimensions
        Float2 pointSS;
        pointSS.m_x = m_x * m_size.m_x;
        pointSS.m_y = m_y * m_size.m_y;
        return pointSS + m_topLeftPosition;
    }

    Vector Viewport::ScreenSpaceToClipSpace( Float2 const& pointSS ) const
    {
        Float4 pointCS( 0.0f, 0.0f, 0.0f, 1.0f );

        // To Normalized pixel space
        pointCS.m_x = ( pointSS.m_x - m_topLeftPosition.m_x ) / m_size.m_x;
        pointCS.m_y = ( pointSS.m_y - m_topLeftPosition.m_y ) / m_size.m_y;

        // Invert Y
        pointCS.m_y = 1.0f - pointCS.m_y;

        // Convert from [0,1] to [-1,1]
        pointCS.m_x = ( pointCS.m_x * 2 ) - 1.0f;
        pointCS.m_y = ( pointCS.m_y * 2 ) - 1.0f;
        return pointCS;
    }

    LineSegment Viewport::ClipSpaceToWorldSpace( Vector const& pointCS ) const
    {
        Float2 const p = pointCS.ToFloat2();
        Vector nearPoint( p.m_x, p.m_y, 0.0f, 1.0f );
        Vector farPoint( p.m_x, p.m_y, 1.0f, 1.0f );

        Matrix const& invViewProj = m_viewVolume.GetInverseViewProjectionMatrix();
        nearPoint = invViewProj.TransformVector3( nearPoint );
        nearPoint /= nearPoint.GetSplatW();

        farPoint = invViewProj.TransformVector3( farPoint );
        farPoint /= farPoint.GetSplatW();

        return LineSegment( nearPoint, farPoint );
    }

    Vector Viewport::ScreenSpaceToWorldSpaceNearPlane( Vector const& pointSS ) const
    {
        Vector pointCS = ScreenSpaceToClipSpace( pointSS ).SetW1();
        pointCS.SetZ( 0.0f );
        pointCS = m_viewVolume.GetInverseViewProjectionMatrix().TransformVector4( pointCS );
        pointCS /= pointCS.GetSplatW();
        return pointCS;
    }

    Vector Viewport::ScreenSpaceToWorldSpaceFarPlane( Vector const& pointSS ) const
    {
        Vector pointCS = ScreenSpaceToClipSpace( pointSS ).SetW1();
        pointCS.SetZ( 1.0f );
        pointCS = m_viewVolume.GetInverseViewProjectionMatrix().TransformVector4( pointCS );
        pointCS /= pointCS.GetSplatW();
        return pointCS;
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void Viewport::CreateViewportSettings( TypeSystem::TypeRegistry& typeRegistry )
    {
        TVector<TypeSystem::TypeInfo const*> visualizationSettingsTypeInfos = typeRegistry.GetAllDerivedTypes( IViewportSettings::GetStaticTypeID(), false, false, true );

        auto SortPredicate = [] ( TypeSystem::TypeInfo const* pA, TypeSystem::TypeInfo const* pB )
        {
            auto pLHS = pA->GetDefaultInstance<IViewportSettings>();
            auto pRHS = pB->GetDefaultInstance<IViewportSettings>();
            return strcmp( pLHS->GetFriendlyName(), pRHS->GetFriendlyName() ) < 0;
        };

        eastl::sort( visualizationSettingsTypeInfos.begin(), visualizationSettingsTypeInfos.end(), SortPredicate );

        for ( auto pTypeInfo : visualizationSettingsTypeInfos )
        {
            auto pSettings = pTypeInfo->CreateType<IViewportSettings>();
            m_settings.emplace_back( pSettings );
        }
    }

    void Viewport::DestroyViewportSettings()
    {
        for ( auto pSettings : m_settings )
        {
            EE::Delete( pSettings );
        }
        m_settings.clear();
    }
    #endif
}