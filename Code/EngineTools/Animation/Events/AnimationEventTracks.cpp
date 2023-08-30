#include "AnimationEventTracks.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    TypeSystem::TypeInfo const* IDEventTrack::GetEventTypeInfo() const
    { 
        return IDEvent::s_pTypeInfo;
    }

    InlineString IDEventTrack::GetItemLabel( Timeline::TrackItem const* pItem ) const
    {
        auto pAnimEvent = GetAnimEvent<IDEvent>( pItem );
        return pAnimEvent->GetID().IsValid() ? pAnimEvent->GetID().c_str() : "Invalid ID";
    }

    //-------------------------------------------------------------------------

    TypeSystem::TypeInfo const* SnapToFrameEventTrack::GetEventTypeInfo() const
    {
        return SnapToFrameEvent::s_pTypeInfo;
    }

    InlineString SnapToFrameEventTrack::GetItemLabel( Timeline::TrackItem const* pItem ) const
    {
        auto pAnimEvent = GetAnimEvent<SnapToFrameEvent>( pItem );
        return pAnimEvent->GetDebugText();
    }
    //-------------------------------------------------------------------------

    TypeSystem::TypeInfo const* FootEventTrack::GetEventTypeInfo() const
    {
        return FootEvent::s_pTypeInfo;
    }

    InlineString FootEventTrack::GetItemLabel( Timeline::TrackItem const* pItem ) const
    {
        auto pAnimEvent = GetAnimEvent<FootEvent>( pItem );
        return FootEvent::GetPhaseName( pAnimEvent->GetFootPhase() );
    }

    Color FootEventTrack::GetItemColor( Timeline::TrackItem const * pItem ) const
    {
        auto pAnimEvent = GetAnimEvent<FootEvent>( pItem );
        return FootEvent::GetPhaseColor( pAnimEvent->GetFootPhase() );
    }

    bool FootEventTrack::DrawContextMenu( Timeline::TrackContext const& context, TVector<Track*>& tracks, float playheadPosition )
    {
        EventTrack::DrawContextMenu( context, tracks, playheadPosition );

        bool updatePhases = false;
        FootEvent::Phase startingPhase = FootEvent::Phase::LeftFootDown;

        if ( ImGui::BeginMenu( EE_ICON_SHOE_PRINT" Set foot events" ) )
        {
            if ( ImGui::MenuItem( "Starting Phase: Left Down") )
            {
                startingPhase = FootEvent::Phase::LeftFootDown;
                updatePhases = true;
            }

            if ( ImGui::MenuItem( "Starting Phase: Right Passing" ) )
            {
                startingPhase = FootEvent::Phase::RightFootPassing;
                updatePhases = true;
            }

            if ( ImGui::MenuItem( "Starting Phase: Right Down" ) )
            {
                startingPhase = FootEvent::Phase::RightFootDown;
                updatePhases = true;
            }

            if ( ImGui::MenuItem( "Starting Phase: Left Passing" ) )
            {
                startingPhase = FootEvent::Phase::LeftFootPassing;
                updatePhases = true;
            }

            ImGui::EndMenu();
        }

        //-------------------------------------------------------------------------

        if ( updatePhases )
        {
            AutoSetPhases( context, startingPhase );
        }

        return false;
    }

    void FootEventTrack::AutoSetPhases( Timeline::TrackContext const& context, FootEvent::Phase startingPhase )
    {
        Timeline::ScopedModification const stm( context );

        FootEvent::Phase currentPhase = startingPhase;

        for ( auto pItem : m_items )
        {
            auto pFootEvent = GetAnimEvent<FootEvent>( pItem );
            pFootEvent->m_phase = currentPhase;
            currentPhase = FootEvent::Phase( ( (int8_t) currentPhase + 1 ) % 4 );
        }
    }

    //-------------------------------------------------------------------------

    TypeSystem::TypeInfo const* TransitionEventTrack::GetEventTypeInfo() const
    {
        return TransitionEvent::s_pTypeInfo;
    }

    InlineString TransitionEventTrack::GetItemLabel( Timeline::TrackItem const* pItem ) const
    {
        auto pAnimEvent = GetAnimEvent<TransitionEvent>( pItem );
        return GetTransitionMarkerName( pAnimEvent->GetMarker() );
    }

    Color TransitionEventTrack::GetItemColor( Timeline::TrackItem const * pItem ) const
    {
        auto pAnimEvent = GetAnimEvent<TransitionEvent>( pItem );
        return GetTransitionMarkerColor( pAnimEvent->GetMarker() );
    }

    //-------------------------------------------------------------------------

    TypeSystem::TypeInfo const* OrientationWarpEventTrack::GetEventTypeInfo() const
    {
        return OrientationWarpEvent::s_pTypeInfo;
    }

    bool OrientationWarpEventTrack::CanCreateNewItems() const
    {
        return m_items.empty();
    }

    Timeline::Track::Status OrientationWarpEventTrack::GetValidationStatus( Timeline::TrackContext const& context ) const
    {
        for ( auto pItem : m_items )
        {
            if ( pItem->GetStartTime() < 1.0f || pItem->GetEndTime() > ( context.GetTimelineLength() - 1 ) )
            {
                m_validationStatueMessage = "Warp event is not allowed to be within the first or last frame!";
                return Status::HasErrors;
            }
        }

        return Status::Valid;
    }

    //-------------------------------------------------------------------------

    TypeSystem::TypeInfo const* RootMotionEventTrack::GetEventTypeInfo() const
    {
        return RootMotionEvent::s_pTypeInfo;
    }

    InlineString RootMotionEventTrack::GetItemLabel( Timeline::TrackItem const * pItem ) const
    {
        auto pAnimEvent = GetAnimEvent<RootMotionEvent>( pItem );
        return pAnimEvent->GetDebugText();
    }

    Timeline::Track::Status RootMotionEventTrack::GetValidationStatus( Timeline::TrackContext const& context ) const
    {
        int32_t const numItems = GetNumItems();
        for ( int32_t i = 0; i < numItems; i++ )
        {
            auto pEvent = GetAnimEvent<RootMotionEvent>( m_items[i]);
            if ( pEvent->GetBlendTime() < 0.0f )
            {
                m_validationStatueMessage = String( String::CtorSprintf(), "Invalid blend time on event %d!", i );
                return Status::HasErrors;
            }
        }

        return Status::Valid;
    }

    //-------------------------------------------------------------------------

    TypeSystem::TypeInfo const* TargetWarpEventTrack::GetEventTypeInfo() const
    {
        return TargetWarpEvent::s_pTypeInfo;
    }

    InlineString TargetWarpEventTrack::GetItemLabel( Timeline::TrackItem const * pItem ) const
    {
        auto pAnimEvent = GetAnimEvent<TargetWarpEvent>( pItem );
        return pAnimEvent->GetDebugText();
    }

    Timeline::Track::Status TargetWarpEventTrack::GetValidationStatus( Timeline::TrackContext const& context ) const
    {
        int32_t numTransXY = 0;
        int32_t numTransZ = 0;
        int32_t numRot = 0;

        for ( auto pItem : m_items )
        {
            auto pEvent = GetAnimEvent<TargetWarpEvent>( pItem );
            TargetWarpRule const warpRule = pEvent->GetRule();

            switch ( warpRule )
            {
                case TargetWarpRule::WarpXY :
                {
                    numTransXY++;
                }
                break;

                case TargetWarpRule::WarpZ:
                {
                    numTransZ++;
                }
                break;

                case TargetWarpRule::WarpXYZ :
                {
                    numTransXY++;
                    numTransZ++;
                }
                break;

                case TargetWarpRule::RotationOnly:
                {
                    numRot++;
                }
                break;
            }
        }

        //-------------------------------------------------------------------------

        if ( numTransXY == 0 )
        {
            m_validationStatueMessage = "Target warps required at least one translation XY event!";
            return Status::HasErrors;
        }

        if ( numTransXY > 1 )
        {
            m_validationStatueMessage = "More than one translation XY event detected! This is not supported!";
            return Status::HasErrors;
        }

        if ( numRot > 1 )
        {
            m_validationStatueMessage = "More than one rotation event detected! This is not supported!";
            return Status::HasErrors;
        }

        //-------------------------------------------------------------------------

        if ( numTransZ == 0 )
        {
            m_validationStatueMessage = "Missing vertical warp translation event! Will only warp in XY!";
            return Status::HasWarnings;
        }

        return Status::Valid;
    }

    Color TargetWarpEventTrack::GetItemColor( Timeline::TrackItem const * pItem ) const
    {
        auto pEvent = GetAnimEvent<TargetWarpEvent>( pItem );
        return GetDebugForWarpRule( pEvent->GetRule() );
    }

    //-------------------------------------------------------------------------

    TypeSystem::TypeInfo const* RagdollEventTrack::GetEventTypeInfo() const
    {
        return RagdollEvent::s_pTypeInfo;
    }

    InlineString RagdollEventTrack::GetItemLabel( Timeline::TrackItem const* pItem ) const
    {
        return "";
    }

    bool RagdollEventTrack::CanCreateNewItems() const
    {
        return GetNumItems() == 0;
    }

    void RagdollEventTrack::DrawDurationItem( Timeline::TrackContext const& context, ImDrawList* pDrawList, ImRect const& itemRect, ItemState itemState, Timeline::TrackItem* pItem )
    {
        constexpr static float const itemMarginY = 2;

        // Draw background
        //-------------------------------------------------------------------------

        ImVec2 const mousePos = ImGui::GetMousePos();
        bool const isHovered = itemRect.Contains( mousePos );
        pDrawList->AddRectFilled( itemRect.GetTL(), itemRect.GetBR(), GetItemBackgroundColor( itemState ) );

        // Draw curve
        //-------------------------------------------------------------------------

        auto pRagdollEvent = GetAnimEvent<RagdollEvent>( pItem );
        FloatCurve const& curve = pRagdollEvent->m_physicsWeightCurve;

        if ( curve.GetNumPoints() > 0 )
        {
            constexpr float const lineWidth = 2.0f;
            float const curveCanvasWidth = itemRect.GetWidth();
            float const curveCanvasHeight = itemRect.GetHeight();

            pDrawList->PushClipRect( itemRect.Min, itemRect.Max, true );
            if ( curve.GetNumPoints() == 1 )
            {
                float const value = curve.GetPoint( 0 ).m_value;
                float const linePosY = itemRect.GetBR().y - ( value * curveCanvasHeight );
                pDrawList->AddLine( ImVec2( itemRect.GetTL().x, linePosY ), ImVec2( itemRect.GetBR().x, linePosY ), Colors::HotPink, lineWidth );

                if ( isHovered )
                {
                    pDrawList->AddLine( ImVec2( mousePos.x, itemRect.GetTL().y ), ImVec2( mousePos.x, itemRect.GetBR().y ), Colors::LightGray, 1.0f );
                    pDrawList->AddCircleFilled( ImVec2( mousePos.x, linePosY ), 3.0f, Colors::LimeGreen );
                    ImGui::SetTooltip( " %.2f ", value );
                }
            }
            else
            {
                FloatRange const horizontalRange = curve.GetParameterRange();
                FloatRange const verticalRange = curve.GetValueRange();

                int32_t const numPointsToDraw = Math::RoundToInt( curveCanvasWidth / 2 ) + 1;
                float const stepT = 1.0f / ( numPointsToDraw - 1 );

                TVector<ImVec2> curvePoints;
                for ( auto i = 0; i < numPointsToDraw; i++ )
                {
                    float const t = ( i * stepT );
                    Float2 curvePoint;
                    curvePoint.m_x = itemRect.GetTL().x + ( t * curveCanvasWidth );
                    curvePoint.m_y = itemRect.GetBR().y - ( curve.Evaluate( t ) * curveCanvasHeight );
                    curvePoints.emplace_back( curvePoint );
                }

                pDrawList->AddPolyline( curvePoints.data(), numPointsToDraw, Colors::HotPink, 0, lineWidth);

                if ( isHovered )
                {
                    Percentage const percentageThroughRange( ( mousePos.x - itemRect.GetTL().x ) / itemRect.GetWidth() );
                    float const value = curve.Evaluate( percentageThroughRange );
                    float const valuePixelY = itemRect.GetBR().y - ( value * curveCanvasHeight );

                    pDrawList->AddLine( ImVec2( mousePos.x, itemRect.GetTL().y ), ImVec2( mousePos.x, itemRect.GetBR().y ), Colors::LightGray, 1.0f );
                    pDrawList->AddCircleFilled( ImVec2( mousePos.x, valuePixelY ), 3.0f, Colors::LimeGreen );
                    ImGui::SetTooltip( " %.2f ", value );
                }
            }
            pDrawList->PopClipRect();
        }
    }

    Timeline::Track::Status RagdollEventTrack::GetValidationStatus( Timeline::TrackContext const& context ) const
    {
        int32_t const numItems = GetNumItems();
        if ( numItems == 1 )
        {
            FloatRange const validRange( 0, 1 );
            auto pRagdollEvent = GetAnimEvent<RagdollEvent>( m_items[0] );
            auto const& curve = pRagdollEvent->m_physicsWeightCurve;
            if ( curve.GetNumPoints() == 0 )
            {
                m_validationStatueMessage = "Curve has no data points!";
                return Timeline::Track::Status::HasWarnings;
            }
            else if ( curve.GetNumPoints() == 1 )
            {
                if ( !validRange.ContainsInclusive( curve.GetPoint( 0 ).m_value ) )
                {
                    m_validationStatueMessage = "Curve values are outside valid range! Keep the curve between 0 and 1 on both axes!";
                    return Timeline::Track::Status::HasErrors;
                }
            }
            else // Check that the value range is valid!
            {
                FloatRange const valueRange = curve.GetValueRange();
                if ( !validRange.ContainsInclusive( valueRange.m_begin ) || !validRange.ContainsInclusive( valueRange.m_end ) )
                {
                    m_validationStatueMessage = "Curve values are outside valid range! Keep the curve between 0 and 1 on both axes!";
                    return Timeline::Track::Status::HasErrors;
                }
            }
        }
        else if( numItems > 1 )
        {
            m_validationStatueMessage = "More than one event detected! This is not allowed!";
            return Timeline::Track::Status::HasErrors;
        }

        //-------------------------------------------------------------------------

        ResetStatusMessage();
        return Timeline::Track::Status::Valid;
    }
}