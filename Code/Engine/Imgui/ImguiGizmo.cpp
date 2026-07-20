#include "ImguiGizmo.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Math/MathUtils.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::ImGuiX
{
    void Gizmo::Result::ApplyResult( Transform& transform ) const
    {
        switch ( m_deltaType )
        {
            case ResultDeltaType::Rotation:
            {
                Quaternion const deltaQuat( m_delta );
                transform.SetRotation( transform.GetRotation() * deltaQuat );
            }
            break;

            case ResultDeltaType::Translation:
            {
                transform.AddTranslation( m_delta );
            }
            break;

            case ResultDeltaType::Scale:
            {
                float newScale = transform.GetScale() + m_delta.GetX();
                if ( Math::IsNearZero( newScale ) )
                {
                    newScale = 0.01f;
                }

                transform.SetScale( newScale );
            }
            break;

            case ResultDeltaType::NonUniformScale:
            {
                EE_UNREACHABLE_CODE(); // Not supported for transforms
            }
            break;

            default:
            break;
        }
    }

    //-------------------------------------------------------------------------

    Gizmo::Gizmo()
    {
        m_options.SetAllFlags();
        m_options.ClearFlag( Options::AllowNonUniformScale );
    }

    Gizmo::Gizmo( TBitFlags<Options> options )
        : m_options( options )
    {}

    void Gizmo::Reset()
    {
        m_translationGizmo.Reset();
        m_rotationGizmo.Reset();
        m_scaleGizmo.Reset();
    }

    ImGuiX::GizmoBase* Gizmo::GetActiveGizmo()
    {
        switch ( m_mode )
        {
            case Mode::Translation:
            {
                return &m_translationGizmo;
            }
            break;

            case Mode::Rotation:
            {
                return &m_rotationGizmo;
            }
            break;

            case Mode::Scale:
            {
                return &m_scaleGizmo;
            }
            break;
        }

        return nullptr;
    }

    void Gizmo::SwitchToNextMode()
    {
        EE_ASSERT( !IsManipulating() );

        switch ( m_mode )
        {
            case Mode::Translation:
            {
                SetMode( Mode::Rotation );
            }
            break;

            case Mode::Rotation:
            {
                if ( m_options.IsFlagSet( Options::AllowScale ) )
                {
                    SetMode( Mode::Scale );
                }
                else
                {
                    SetMode( Mode::Translation );
                }
            }
            break;

            case Mode::Scale:
            {
                EE_ASSERT( m_options.IsFlagSet( Options::AllowScale ) );
                SetMode( Mode::Translation );
            }
            break;
        }
    }

    void Gizmo::SetMode( Mode newMode )
    {
        EE_ASSERT( !IsManipulating() );

        if ( newMode == m_mode )
        {
            return;
        }

        if ( newMode == Mode::Scale )
        {
            if ( !m_options.IsFlagSet( Options::AllowScale ) )
            {
                EE_LOG_WARNING( LogCategory::Tools, "Gizmo", "Trying to switch a gizmo to scale mode but it is disabled!" );
                return;
            }
        }

        CoordinateSpace const coordinateSpace = GetCoordinateSystemSpace();
        m_mode = newMode;

        //-------------------------------------------------------------------------

        switch ( m_mode )
        {
            case Mode::Translation:
            {
                m_translationGizmo.Reset();
                m_translationGizmo.SetCoordinateSystemSpace( coordinateSpace );
            }
            break;

            case Mode::Rotation:
            {
                m_rotationGizmo.Reset();
                m_rotationGizmo.SetCoordinateSystemSpace( coordinateSpace );
            }
            break;

            case Mode::Scale:
            {
                m_scaleGizmo.Reset();
                m_scaleGizmo.SetCoordinateSystemSpace( coordinateSpace );
                m_scaleGizmo.SetNonUniformScaleAllowed( m_options.IsFlagSet( Options::AllowNonUniformScale ) );
            }
            break;
        }
    }

    bool Gizmo::IsManipulating() const
    {
        return m_translationGizmo.IsManipulating() || m_rotationGizmo.IsManipulating() || m_scaleGizmo.IsManipulating();
    }

    void Gizmo::SetCoordinateSystemSpace( CoordinateSpace space )
    {
        if ( !m_options.IsFlagSet( Options::AllowCoordinateSpaceSwitching ) )
        {
            EE_LOG_WARNING( LogCategory::Tools, "Gizmo", "Trying to switch a gizmo's coordinate system, but this is disabled!" );
            return;
        }

        //-------------------------------------------------------------------------

        if ( GetCoordinateSystemSpace() == space )
        {
            return;
        }

        //-------------------------------------------------------------------------

        m_translationGizmo.SetCoordinateSystemSpace( space );
        m_rotationGizmo.SetCoordinateSystemSpace( space );
        m_scaleGizmo.SetCoordinateSystemSpace( space );
    }

    Gizmo::Result Gizmo::Draw( Vector const& positionWS, Quaternion const& orientationWS, Viewport const& viewport, char const* pOptionalLabel )
    {
        Result result;
        result.m_delta = Vector::Zero;
        result.m_deltaType = ResultDeltaType::None;

        //-------------------------------------------------------------------------

        switch ( m_mode )
        {
            case Mode::Translation:
            {
                //m_translationGizmo.EnableDebug( true );
                result.m_state = m_translationGizmo.UpdateAndDraw( positionWS, orientationWS, viewport, pOptionalLabel );
                if ( m_translationGizmo.IsManipulating() )
                {
                    result.m_delta = m_translationGizmo.GetDelta();
                    result.m_deltaType = ResultDeltaType::Translation;
                }
            }
            break;

            case Mode::Rotation:
            {
                //m_rotationGizmo.EnableDebug( true );
                result.m_state = m_rotationGizmo.UpdateAndDraw( positionWS, orientationWS, viewport, pOptionalLabel );
                if ( m_rotationGizmo.IsManipulating() )
                {
                    result.m_delta = m_rotationGizmo.GetDelta().ToVector();
                    result.m_deltaType = ResultDeltaType::Rotation;
                    EE_ASSERT( !result.m_delta.IsZero4() );
                }
                else
                {
                    result.m_delta = Vector::UnitW;
                }
            }
            break;

            case Mode::Scale:
            {
                //m_scaleGizmo.EnableDebug( true );

                m_scaleGizmo.SetNonUniformScaleAllowed( m_options.IsFlagSet( Options::AllowNonUniformScale ) );
                result.m_state = m_scaleGizmo.UpdateAndDraw( positionWS, orientationWS, viewport, pOptionalLabel );
                if ( m_scaleGizmo.IsManipulating() )
                {
                    result.m_delta = m_scaleGizmo.GetDelta();
                    result.m_deltaType = ResultDeltaType::Scale;
                }
            }
            break;
        }

        return result;
    }
}
#endif