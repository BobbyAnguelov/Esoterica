#include "PropertyGridEditor.h"
#include "EngineTools/Core/ToolsContext.h"
#include "EngineTools/Widgets/Pickers.h"
#include "EngineTools/Widgets/CurveEditor.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Resource/ResourcePtr.h"
#include "Base/TypeSystem/PropertyInfo.h"
#include "Base/TypeSystem/EnumInfo.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/Types/String.h"
#include "Base/Types/Tag.h"
#include "Base/TypeSystem/ResourceInfo.h"
#include "Base/TypeSystem/TypeInstance.h"

//-------------------------------------------------------------------------

using namespace EE::TypeSystem;

//-------------------------------------------------------------------------
// Property Grid Editor Base
//-------------------------------------------------------------------------

namespace EE::PG
{
    PropertyEditor::PropertyEditor( PropertyEditorContext const& context, PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance )
        : m_context( context )
        , m_propertyInfo( propertyInfo )
        , m_pTypeInstance( pTypeInstance )
        , m_pPropertyInstance( pPropertyInstance )
        , m_coreType( GetCoreType( propertyInfo.m_typeID ) )
    {
        EE_ASSERT( m_pTypeInstance != nullptr );
        EE_ASSERT( m_pPropertyInstance != nullptr );
    }

    PropertyEditor::Result PropertyEditor::UpdateAndDraw()
    {
        ImGui::PushID( m_pPropertyInstance );
        HandleExternalUpdate();
        Result const result = InternalUpdateAndDraw();
        ImGui::PopID();

        return result;
    }
}

//-------------------------------------------------------------------------
// Core Property Grid Editors
//-------------------------------------------------------------------------

namespace EE::PG
{
    constexpr static float const g_iconButtonWidth = 30;

    //-------------------------------------------------------------------------
    // Core Editors
    //-------------------------------------------------------------------------

    class EnumEditor final : public PropertyEditor
    {
    public:

        EnumEditor( PropertyEditorContext const& context, PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance )
            : PropertyEditor( context, propertyInfo, pTypeInstance, pPropertyInstance )
            , m_pEnumInfo( GetTypeRegistry()->GetEnumInfo( m_propertyInfo.m_typeID ) )
        {
            EE_ASSERT( m_pEnumInfo != nullptr );
            EnumEditor::ResetWorkingCopy();
        }

        virtual Result InternalUpdateAndDraw() override
        {
            InlineString currentValueStr;

            StringID enumLabel;
            if ( m_pEnumInfo->TryGetConstantLabel( m_value_imgui, enumLabel ) )
            {
                currentValueStr = enumLabel.c_str();
            }
            else
            {
                currentValueStr.append_sprintf( "Invalid Value: %d", m_value_imgui );
            }

            //-------------------------------------------------------------------------

            ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x );
            if ( ImGui::BeginCombo( "##enumCombo", currentValueStr.c_str() ) )
            {
                for ( auto const& enumValue : m_pEnumInfo->m_constants )
                {
                    bool const isSelected = ( enumValue.m_value == m_value_imgui );
                    if ( ImGui::Selectable( enumValue.m_ID.c_str(), isSelected ) )
                    {
                        m_value_imgui = enumValue.m_value;
                    }

                    if ( !enumValue.m_description.empty() )
                    {
                        ImGuiX::ItemTooltip( enumValue.m_description.c_str() );
                    }

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if ( isSelected )
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }

                ImGui::EndCombo();
            }

            return ( m_value_cached != m_value_imgui ) ? Result::ValueUpdated : Result::None;
        }

        virtual void UpdatePropertyValue() override
        {
            m_value_cached = m_value_imgui;

            switch ( m_pEnumInfo->m_underlyingType )
            {
                case CoreTypeID::Uint8:
                {
                    *reinterpret_cast<uint8_t*>( m_pPropertyInstance ) = (uint8_t)m_value_cached;
                }
                break;

                case CoreTypeID::Int8:
                {
                    *reinterpret_cast<int8_t*>( m_pPropertyInstance ) = (int8_t)m_value_cached;
                }
                break;

                case CoreTypeID::Uint16:
                {
                    *reinterpret_cast<uint16_t*>( m_pPropertyInstance ) = (uint16_t)m_value_cached;
                }
                break;

                case CoreTypeID::Int16:
                {
                    *reinterpret_cast<int16_t*>( m_pPropertyInstance ) = (int16_t)m_value_cached;
                }
                break;

                case CoreTypeID::Uint32:
                {
                    *reinterpret_cast<uint32_t*>( m_pPropertyInstance ) = (uint32_t)m_value_cached;
                }
                break;

                case CoreTypeID::Int32:
                {
                    *reinterpret_cast<int32_t*>( m_pPropertyInstance ) = (int32_t)m_value_cached;
                }
                break;

                default:
                {
                    EE_UNREACHABLE_CODE();
                }
                break;
            }
        }

        virtual void ResetWorkingCopy() override
        {
            switch ( m_pEnumInfo->m_underlyingType )
            {
                case CoreTypeID::Uint8:
                {
                    m_value_cached = m_value_imgui = ( int64_t ) * reinterpret_cast<uint8_t const*>( m_pPropertyInstance );
                }
                break;

                case CoreTypeID::Int8:
                {
                    m_value_cached = m_value_imgui = ( int64_t ) * reinterpret_cast<int8_t const*>( m_pPropertyInstance );
                }
                break;

                case CoreTypeID::Uint16:
                {
                    m_value_cached = m_value_imgui = ( int64_t ) * reinterpret_cast<uint16_t const*>( m_pPropertyInstance );
                }
                break;

                case CoreTypeID::Int16:
                {
                    m_value_cached = m_value_imgui = ( int64_t ) * reinterpret_cast<int16_t const*>( m_pPropertyInstance );
                }
                break;

                case CoreTypeID::Uint32:
                {
                    m_value_cached = m_value_imgui = ( int64_t ) * reinterpret_cast<uint32_t const*>( m_pPropertyInstance );
                }
                break;

                case CoreTypeID::Int32:
                {
                    m_value_cached = m_value_imgui = ( int64_t ) * reinterpret_cast<int32_t const*>( m_pPropertyInstance );
                }
                break;

                default:
                {
                    EE_UNREACHABLE_CODE();
                }
                break;
            }
        }

        virtual void HandleExternalUpdate() override
        {
            switch ( m_pEnumInfo->m_underlyingType )
            {
                case CoreTypeID::Uint8:
                {
                    auto actualValue = ( int64_t ) * reinterpret_cast<uint8_t const*>( m_pPropertyInstance );
                    if ( actualValue != m_value_cached )
                    {
                        m_value_cached = m_value_imgui = actualValue;
                    }
                }
                break;

                case CoreTypeID::Int8:
                {
                    auto actualValue = ( int64_t ) * reinterpret_cast<int8_t const*>( m_pPropertyInstance );
                    if ( actualValue != m_value_cached )
                    {
                        m_value_cached = m_value_imgui = actualValue;
                    }
                }
                break;

                case CoreTypeID::Uint16:
                {
                    auto actualValue = ( int64_t ) * reinterpret_cast<uint16_t const*>( m_pPropertyInstance );
                    if ( actualValue != m_value_cached )
                    {
                        m_value_cached = m_value_imgui = actualValue;
                    }
                }
                break;

                case CoreTypeID::Int16:
                {
                    auto actualValue = ( int64_t ) * reinterpret_cast<int16_t const*>( m_pPropertyInstance );
                    if ( actualValue != m_value_cached )
                    {
                        m_value_cached = m_value_imgui = actualValue;
                    }
                }
                break;

                case CoreTypeID::Uint32:
                {
                    auto actualValue = ( int64_t ) * reinterpret_cast<uint32_t const*>( m_pPropertyInstance );
                    if ( actualValue != m_value_cached )
                    {
                        m_value_cached = m_value_imgui = actualValue;
                    }
                }
                break;

                case CoreTypeID::Int32:
                {
                    auto actualValue = ( int64_t ) * reinterpret_cast<int32_t const*>( m_pPropertyInstance );
                    if ( actualValue != m_value_cached )
                    {
                        m_value_cached = m_value_imgui = actualValue;
                    }
                }
                break;

                default:
                {
                    EE_UNREACHABLE_CODE();
                }
                break;
            }
        }

    private:

        EnumInfo const*         m_pEnumInfo = nullptr;
        int64_t                 m_value_cached;
        int64_t                 m_value_imgui;
    };

    //-------------------------------------------------------------------------

    class BoolEditor final : public PropertyEditor
    {
    public:

        BoolEditor( PropertyEditorContext const& context, PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance )
            : PropertyEditor( context, propertyInfo, pTypeInstance, pPropertyInstance )
        {
            BoolEditor::ResetWorkingCopy();
        }

        virtual Result InternalUpdateAndDraw() override
        {
            ImGui::Checkbox( "##be", &m_value );
            return ImGui::IsItemDeactivatedAfterEdit() ? Result::ValueUpdated : Result::None;
        }

        virtual void UpdatePropertyValue() override
        {
            *reinterpret_cast<bool*>( m_pPropertyInstance ) = m_value;
        }

        virtual void ResetWorkingCopy() override
        {
            m_value = *reinterpret_cast<bool*>( m_pPropertyInstance );
        }

        virtual void HandleExternalUpdate() override
        {
            ResetWorkingCopy();
        }

    private:

        bool m_value;
    };

    // Numeric Editors
    //-------------------------------------------------------------------------

    enum class ClampMode
    {
        None,
        Min,
        Max,
        Both
    };

    template<typename T>
    static ClampMode GetClampInfo( PropertyInfo const& propertyInfo, T& outMin, T& outMax )
    {
        ClampMode mode = ClampMode::None;

        bool const hasMinClamp = propertyInfo.m_metadata.TryGetNumericValue( PropertyMetadata::Min, std::numeric_limits<T>::lowest(), outMin );
        bool const hasMaxClamp = propertyInfo.m_metadata.TryGetNumericValue( PropertyMetadata::Max, std::numeric_limits<T>::max(), outMax );

        //-------------------------------------------------------------------------

        if ( hasMinClamp && hasMaxClamp )
        {
            mode = ClampMode::Both;
        }
        else if ( hasMinClamp )
        {
            mode = ClampMode::Min;
        }
        else if ( hasMaxClamp )
        {
            mode = ClampMode::Max;
        }

        //-------------------------------------------------------------------------

        if ( outMax < outMin )
        {
            outMin = std::numeric_limits<T>::lowest();
            outMax = std::numeric_limits<T>::max();
            mode = ClampMode::None;

            EE_LOG_WARNING( "Tools", "PropertyGrid", "Invalid Min/Max meta flags for property: %s", propertyInfo.m_ID.c_str() );
        }

        return mode;
    }

    template<typename T>
    static T ApplyClamp( ClampMode mode, T const value, T const min, T const max )
    {
        if ( mode == ClampMode::Both )
        {
            return Math::Clamp( value, min, max );
        }
        else if ( mode == ClampMode::Min )
        {
            return Math::Max( value, min );
        }
        else if ( mode == ClampMode::Max )
        {
            return Math::Min( value, max );
        }

        return value;
    }

    //-------------------------------------------------------------------------

    template<typename T, ImGuiDataType_ DT>
    class IntegerEditor final : public PropertyEditor
    {
    public:

        IntegerEditor( PropertyEditorContext const& context, PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance )
            : PropertyEditor( context, propertyInfo, pTypeInstance, pPropertyInstance )
        {
            IntegerEditor::ResetWorkingCopy();
            m_clampMode = GetClampInfo( m_propertyInfo, m_min, m_max );
        }

        virtual Result InternalUpdateAndDraw() override
        {
            ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x );

            if ( m_clampMode == ClampMode::Both )
            {
                ImGui::SliderScalar( "##scaed", DT, &m_value_imgui, &m_min, &m_max, nullptr, ImGuiSliderFlags_AlwaysClamp );
            }
            else
            {
                if ( ImGui::InputScalar( "##scaed", DT, &m_value_imgui ) )
                {
                    m_value_imgui = ApplyClamp( m_clampMode, m_value_imgui, m_min, m_max );
                }
            }

            return ImGui::IsItemDeactivatedAfterEdit() ? Result::ValueUpdated : Result::None;
        }

        virtual void UpdatePropertyValue() override
        {
            m_value_cached = m_value_imgui;
            *reinterpret_cast<T*>( m_pPropertyInstance ) = m_value_cached;
        }

        virtual void ResetWorkingCopy() override
        {
            m_value_cached = m_value_imgui = *reinterpret_cast<T*>( m_pPropertyInstance );
        }

        virtual void HandleExternalUpdate() override
        {
            T const actualValue = *reinterpret_cast<T*>( m_pPropertyInstance );
            if ( actualValue != m_value_cached )
            {
                m_value_cached = m_value_imgui = actualValue;
            }
        }

    private:

        T                   m_value_imgui;
        T                   m_value_cached;
        T                   m_min;
        T                   m_max;
        ClampMode           m_clampMode = ClampMode::None;
    };

    class FloatEditor final : public PropertyEditor
    {
    public:

        FloatEditor( PropertyEditorContext const& context, PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance )
            : PropertyEditor( context, propertyInfo, pTypeInstance, pPropertyInstance )
        {
            FloatEditor::ResetWorkingCopy();

            static constexpr float const imguiSliderMax = FLT_MAX / 2;
            m_clampMode = GetClampInfo( propertyInfo, m_min, m_max );
            m_min = Math::Clamp( m_min, -imguiSliderMax, imguiSliderMax );
            m_max = Math::Clamp( m_max, -imguiSliderMax, imguiSliderMax );
        }

        virtual Result InternalUpdateAndDraw() override
        {
            ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x );

            if ( m_clampMode == ClampMode::Both )
            {
                ImGui::SliderFloat( "##scaed", &m_value_imgui, m_min, m_max, "%.3f", ImGuiSliderFlags_AlwaysClamp );
            }
            else
            {
                if( ImGui::InputFloat( "##scaed", &m_value_imgui ) )
                {
                    m_value_imgui = ApplyClamp( m_clampMode, m_value_imgui, m_min, m_max );
                }
            }

            return ImGui::IsItemDeactivatedAfterEdit() ? Result::ValueUpdated : Result::None;
        }

        virtual void UpdatePropertyValue() override
        {
            m_value_cached = m_value_imgui;
            *reinterpret_cast<float*>( m_pPropertyInstance ) = m_value_cached;
        }

        virtual void ResetWorkingCopy() override
        {
            m_value_cached = m_value_imgui = *reinterpret_cast<float*>( m_pPropertyInstance );
        }

        virtual void HandleExternalUpdate() override
        {
            float const actualValue = *reinterpret_cast<float*>( m_pPropertyInstance );
            if ( actualValue != m_value_cached )
            {
                m_value_cached = m_value_imgui = actualValue;
            }
        }

    private:

        float               m_value_imgui;
        float               m_value_cached;
        float               m_min;
        float               m_max;
        ClampMode           m_clampMode = ClampMode::None;
    };

    class DoubleEditor final : public PropertyEditor
    {
    public:

        DoubleEditor( PropertyEditorContext const& context, PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance )
            : PropertyEditor( context, propertyInfo, pTypeInstance, pPropertyInstance )
        {
            DoubleEditor::ResetWorkingCopy();
            m_clampMode = GetClampInfo( propertyInfo, m_min, m_max );
        }

        virtual Result InternalUpdateAndDraw() override
        {
            ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x );

            if ( ImGui::InputDouble( "##scaed", (double*) &m_value_imgui ) )
            {
                m_value_imgui = ApplyClamp( m_clampMode, m_value_imgui, m_min, m_max );
            }

            return ImGui::IsItemDeactivatedAfterEdit() ? Result::ValueUpdated : Result::None;
        }

        virtual void UpdatePropertyValue() override
        {
            m_value_cached = m_value_imgui;
            *reinterpret_cast<double*>( m_pPropertyInstance ) = m_value_cached;
        }

        virtual void ResetWorkingCopy() override
        {
            m_value_cached = m_value_imgui = *reinterpret_cast<double*>( m_pPropertyInstance );
        }

        virtual void HandleExternalUpdate() override
        {
            double const actualValue = *reinterpret_cast<double*>( m_pPropertyInstance );
            if ( actualValue != m_value_cached )
            {
                m_value_cached = m_value_imgui = actualValue;
            }
        }

    private:

        double              m_value_imgui;
        double              m_value_cached;
        double              m_min;
        double              m_max;
        ClampMode           m_clampMode = ClampMode::None;
    };

    //-------------------------------------------------------------------------

    template<typename T>
    class FloatNEditor final : public PropertyEditor
    {
    public:

        FloatNEditor( PropertyEditorContext const& context, PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance )
            : PropertyEditor( context, propertyInfo, pTypeInstance, pPropertyInstance )
        {
            FloatNEditor::ResetWorkingCopy();
        }

        virtual Result InternalUpdateAndDraw() override
        {
            bool valueChanged = false;

            switch ( m_coreType )
            {
                case CoreTypeID::Float2:
                {
                    if ( ImGuiX::InputFloat2( this, (Float2&) m_value_imgui ) )
                    {
                        valueChanged = true;
                    }
                }
                break;

                case CoreTypeID::Float3:
                {
                    if ( ImGuiX::InputFloat3( this, (Float3&)m_value_imgui ) )
                    {
                        valueChanged = true;
                    }
                }
                break;

                case CoreTypeID::Float4:
                {
                    if ( ImGuiX::InputFloat4( this, (Float4&)m_value_imgui ) )
                    {
                        valueChanged = true;
                    }
                }
                break;

                default:
                {
                    EE_UNREACHABLE_CODE();
                }
                break;
            }

            return valueChanged ? Result::ValueUpdated : Result::None;
        }

        virtual void UpdatePropertyValue() override
        {
            m_value_cached = m_value_imgui;
            *reinterpret_cast<T*>( m_pPropertyInstance ) = m_value_cached;
        }

        virtual void ResetWorkingCopy() override
        {
            m_value_cached = m_value_imgui = *reinterpret_cast<T*>( m_pPropertyInstance );
        }

        virtual void HandleExternalUpdate() override
        {
            T const actualValue = *reinterpret_cast<T*>( m_pPropertyInstance );
            if ( actualValue != m_value_cached )
            {
                m_value_cached = m_value_imgui = actualValue;
            }
        }

    private:

        T                   m_value_imgui;
        T                   m_value_cached;
    };

    //-------------------------------------------------------------------------

    class VectorEditor final : public PropertyEditor
    {
    public:

        VectorEditor( PropertyEditorContext const& context, PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance )
            : PropertyEditor( context, propertyInfo, pTypeInstance, pPropertyInstance )
        {
            VectorEditor::ResetWorkingCopy();
        }

        virtual Result InternalUpdateAndDraw() override
        {
            bool valueChanged = false;
            if ( ImGuiX::InputFloat4( "VectorEd", m_value_imgui ) )
            {
                valueChanged = true;
            }
            return valueChanged ? Result::ValueUpdated : Result::None;
        }

        virtual void UpdatePropertyValue() override
        {
            m_value_cached = m_value_imgui;
            *reinterpret_cast<Vector*>( m_pPropertyInstance ) = Vector( m_value_cached );
        }

        virtual void ResetWorkingCopy() override
        {
            m_value_cached = m_value_imgui = reinterpret_cast<Vector*>( m_pPropertyInstance )->ToFloat4();
        }

        virtual void HandleExternalUpdate() override
        {
            Float4 const actualValue = reinterpret_cast<Vector*>( m_pPropertyInstance )->ToFloat4();
            if ( actualValue != m_value_cached )
            {
                m_value_cached = m_value_imgui = actualValue;
            }
        }

    private:

        Float4              m_value_imgui;
        Float4              m_value_cached;
    };

    //-------------------------------------------------------------------------

    class ColorEditor final : public PropertyEditor
    {
    public:

        ColorEditor( PropertyEditorContext const& context, PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance )
            : PropertyEditor( context, propertyInfo, pTypeInstance, pPropertyInstance )
        {
            ColorEditor::ResetWorkingCopy();
        }

        virtual Result InternalUpdateAndDraw() override
        {
            ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x );
            ImGui::ColorEdit4( "##ce", &m_value_imgui.x );
            return ImGui::IsItemDeactivatedAfterEdit() ? Result::ValueUpdated : Result::None;
        }

        virtual void UpdatePropertyValue() override
        {
            m_value_cached = m_value_imgui;
            *reinterpret_cast<Color*>( m_pPropertyInstance ) = Color( m_value_cached );
        }

        virtual void ResetWorkingCopy() override
        {
            m_value_cached = m_value_imgui = reinterpret_cast<Color*>( m_pPropertyInstance )->ToFloat4();
        }

        virtual void HandleExternalUpdate() override
        {
            Float4 const actualValue = reinterpret_cast<Color*>( m_pPropertyInstance )->ToFloat4();
            if ( !Vector( actualValue ).IsNearEqual4( m_value_cached ) )
            {
                m_value_cached = m_value_imgui = actualValue;
            }
        }

    private:

        ImVec4 m_value_imgui;
        ImVec4 m_value_cached;
    };

    //-------------------------------------------------------------------------

    class RotationEditor final : public PropertyEditor
    {
    public:

        RotationEditor( PropertyEditorContext const& context, PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance )
            : PropertyEditor( context, propertyInfo, pTypeInstance, pPropertyInstance )
        {
            RotationEditor::ResetWorkingCopy();
            EE_ASSERT( m_coreType == CoreTypeID::EulerAngles || m_coreType == CoreTypeID::Quaternion );
        }

        virtual Result InternalUpdateAndDraw() override
        {
            return ImGuiX::InputFloat3( "Rotation", m_anglesInDegrees_imgui ) ? Result::ValueUpdated : Result::None;
        }

        virtual void UpdatePropertyValue() override
        {
            m_anglesInDegrees_cached = m_anglesInDegrees_imgui;
            EulerAngles const newRotation( Degrees( m_anglesInDegrees_cached.m_x ).GetClamped180(), Degrees( m_anglesInDegrees_cached.m_y ).GetClamped180(), Degrees( m_anglesInDegrees_cached.m_z ).GetClamped180() );

            if ( m_coreType == CoreTypeID::EulerAngles )
            {
                *reinterpret_cast<EulerAngles*>( m_pPropertyInstance ) = newRotation;
            }
            else if ( m_coreType == CoreTypeID::Quaternion )
            {
                *reinterpret_cast<Quaternion*>( m_pPropertyInstance ) = Quaternion( newRotation );
            }
        }

        virtual void ResetWorkingCopy() override
        {
            if ( m_coreType == CoreTypeID::EulerAngles )
            {
                m_anglesInDegrees_cached = m_anglesInDegrees_imgui = reinterpret_cast<EulerAngles*>( m_pPropertyInstance )->GetAsDegrees();
            }
            else if ( m_coreType == CoreTypeID::Quaternion )
            {
                m_anglesInDegrees_cached = m_anglesInDegrees_imgui = reinterpret_cast<Quaternion*>( m_pPropertyInstance )->ToEulerAngles().GetAsDegrees();
            }
        }

        virtual void HandleExternalUpdate() override
        {
            if ( m_coreType == CoreTypeID::EulerAngles )
            {
                Float3 const actualValue = reinterpret_cast<EulerAngles*>( m_pPropertyInstance )->GetAsDegrees();
                if ( !Vector( actualValue ).IsNearEqual3( m_anglesInDegrees_cached ) )
                {
                    m_anglesInDegrees_cached = m_anglesInDegrees_imgui = actualValue;
                }
            }
            else if ( m_coreType == CoreTypeID::Quaternion )
            {
                // We need to check the delta between the working copy and the real value since there are multiple euler representations of a given quaternion
                Quaternion const& externalQuat = *reinterpret_cast<Quaternion*>( m_pPropertyInstance );
                Quaternion const cachedQuat = Quaternion( EulerAngles( m_anglesInDegrees_cached ) );
                Radians const angularDistance = Quaternion::Distance( externalQuat, cachedQuat );
                if ( angularDistance > Degrees( 0.5f ) )
                {
                    Float3 const actualValue = externalQuat.ToEulerAngles().GetAsDegrees();
                    m_anglesInDegrees_cached = m_anglesInDegrees_imgui = actualValue;
                }
            }
        }

    private:

        Float3                m_anglesInDegrees_imgui;
        Float3                m_anglesInDegrees_cached;
    };

    //-------------------------------------------------------------------------

    class AngleEditor final : public PropertyEditor
    {
    public:

        AngleEditor( PropertyEditorContext const& context, PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance )
            : PropertyEditor( context, propertyInfo, pTypeInstance, pPropertyInstance )
        {
            AngleEditor::ResetWorkingCopy();
            EE_ASSERT( m_coreType == CoreTypeID::Degrees || m_coreType == CoreTypeID::Radians );
        }

        virtual Result InternalUpdateAndDraw() override
        {
            float const textAreaWidth = ImGui::GetContentRegionAvail().x - ( g_iconButtonWidth * 2 ) - ( ImGui::GetStyle().ItemSpacing.x * 2 );

            ImGui::SetNextItemWidth( textAreaWidth );
            ImGui::InputFloat( "##ae", &m_valueInDegrees_imgui );
            bool valueChanged = ImGui::IsItemDeactivatedAfterEdit();

            //-------------------------------------------------------------------------

            ImGui::SameLine( 0, ImGui::GetStyle().ItemSpacing.x );
            if ( ImGui::Button( EE_ICON_ANGLE_ACUTE "##ClampShortest", ImVec2( g_iconButtonWidth, 0 ) ) )
            {
                m_valueInDegrees_imgui = Degrees( m_valueInDegrees_imgui ).GetClamped180().ToFloat();
                valueChanged = true;
            }
            ImGuiX::ItemTooltip( "Clamp to Shortest Rotation [-180 : 180]" );

            ImGui::SameLine( 0, ImGui::GetStyle().ItemSpacing.x );
            if ( ImGui::Button( EE_ICON_ANGLE_OBTUSE "##ClampFull", ImVec2( g_iconButtonWidth, 0 ) ) )
            {
                m_valueInDegrees_imgui = Degrees( m_valueInDegrees_imgui ).GetClamped360().ToFloat();
                valueChanged = true;
            }
            ImGuiX::ItemTooltip( "Clamp to full rotation [-360 : 360]" );

            return valueChanged ? Result::ValueUpdated : Result::None;
        }

        virtual void UpdatePropertyValue() override
        {
            m_valueInDegrees_cached = m_valueInDegrees_imgui;
            if ( m_coreType == CoreTypeID::Degrees )
            {
                *reinterpret_cast<Degrees*>( m_pPropertyInstance ) = Degrees( m_valueInDegrees_cached );
            }
            else if ( m_coreType == CoreTypeID::Radians )
            {
                *reinterpret_cast<Radians*>( m_pPropertyInstance ) = Degrees( m_valueInDegrees_cached );
            }
        }

        virtual void ResetWorkingCopy() override
        {
            if ( m_coreType == CoreTypeID::Degrees )
            {
                m_valueInDegrees_cached = m_valueInDegrees_imgui = reinterpret_cast<Degrees*>( m_pPropertyInstance )->ToFloat();
            }
            else if ( m_coreType == CoreTypeID::Radians )
            {
                m_valueInDegrees_cached = m_valueInDegrees_imgui = reinterpret_cast<Radians*>( m_pPropertyInstance )->ToDegrees().ToFloat();
            }
        }

        virtual void HandleExternalUpdate() override
        {
            float actualValue = 0;

            if ( m_coreType == CoreTypeID::Degrees )
            {
                actualValue = reinterpret_cast<Degrees*>( m_pPropertyInstance )->ToFloat();
            }
            else if ( m_coreType == CoreTypeID::Radians )
            {
                actualValue = reinterpret_cast<Radians*>( m_pPropertyInstance )->ToDegrees().ToFloat();
            }

            if ( actualValue != m_valueInDegrees_cached )
            {
                m_valueInDegrees_cached = m_valueInDegrees_imgui = actualValue;
            }
        }

    private:

        float                m_valueInDegrees_imgui;
        float                m_valueInDegrees_cached;
    };

    //-------------------------------------------------------------------------

    class UUIDEditor final : public PropertyEditor
    {
    public:

        UUIDEditor( PropertyEditorContext const& context, PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance )
            : PropertyEditor( context, propertyInfo, pTypeInstance, pPropertyInstance )
        {
            UUIDEditor::ResetWorkingCopy();
        }

        virtual Result InternalUpdateAndDraw() override
        {
            float const textAreaWidth = ImGui::GetContentRegionAvail().x - g_iconButtonWidth - ImGui::GetStyle().ItemSpacing.x;

            bool valueChanged = false;

            ImGui::SetNextItemWidth( textAreaWidth );
            ImGui::InputText( "##ue", m_stringValue.data(), m_stringValue.length(), ImGuiInputTextFlags_ReadOnly );

            ImGui::SameLine( 0, ImGui::GetStyle().ItemSpacing.x );
            if ( ImGui::Button( EE_ICON_COG"##Generate", ImVec2( g_iconButtonWidth, 0 ) ) )
            {
                m_value_imgui = UUID::GenerateID();
                m_stringValue = m_value_imgui.ToString();
                valueChanged = true;
            }
            ImGuiX::ItemTooltip( "Generate UUID" );

            return valueChanged ? Result::ValueUpdated : Result::None;
        }

        virtual void UpdatePropertyValue() override
        {
            m_value_cached = m_value_imgui;
            *reinterpret_cast<UUID*>( m_pPropertyInstance ) = m_value_cached;
        }

        virtual void ResetWorkingCopy() override
        {
            m_value_cached = m_value_imgui = *reinterpret_cast<UUID*>( m_pPropertyInstance );
            m_stringValue = m_value_imgui.ToString();
        }

        virtual void HandleExternalUpdate() override
        {
            auto& actualValue = *reinterpret_cast<UUID*>( m_pPropertyInstance );
            if ( actualValue != m_value_cached )
            {
                m_value_cached = m_value_imgui = actualValue;
                m_stringValue = m_value_imgui.ToString();
            }
        }

    private:

        UUID            m_value_imgui;
        UUID            m_value_cached;
        UUIDString      m_stringValue;
    };

    //-------------------------------------------------------------------------

    class TimeEditor final : public PropertyEditor
    {
    public:

        TimeEditor( PropertyEditorContext const& context, PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance )
            : PropertyEditor( context, propertyInfo, pTypeInstance, pPropertyInstance )
        {
            TimeEditor::ResetWorkingCopy();
            EE_ASSERT( m_coreType == CoreTypeID::Microseconds || m_coreType == CoreTypeID::Milliseconds || m_coreType == CoreTypeID::Seconds );
        }

        virtual Result InternalUpdateAndDraw() override
        {
            ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x );
            if ( m_coreType == CoreTypeID::Microseconds )
            {
                ImGui::InputFloat( "##teus", &m_value_imgui, 0.0f, 0.0f, "%.2fus" );
            }
            else if ( m_coreType == CoreTypeID::Milliseconds )
            {
                ImGui::InputFloat( "#tems", &m_value_imgui, 0.0f, 0.0f, "%.2fms" );
            }
            else if ( m_coreType == CoreTypeID::Seconds )
            {
                ImGui::InputFloat( "##tes", &m_value_imgui, 0.0f, 0.0f, "%.2fs" );
            }

            return ImGui::IsItemDeactivatedAfterEdit() ? Result::ValueUpdated : Result::None;
        }

        virtual void UpdatePropertyValue() override
        {
            m_value_cached = m_value_imgui;

            if ( m_coreType == CoreTypeID::Microseconds )
            {
                *reinterpret_cast<Microseconds*>( m_pPropertyInstance ) = Microseconds( m_value_cached );
            }
            else if ( m_coreType == CoreTypeID::Milliseconds )
            {
                *reinterpret_cast<Milliseconds*>( m_pPropertyInstance ) = Milliseconds( m_value_cached );
            }
            else if ( m_coreType == CoreTypeID::Seconds )
            {
                *reinterpret_cast<Seconds*>( m_pPropertyInstance ) = Seconds( m_value_cached );
            }
        }

        virtual void ResetWorkingCopy() override
        {
            if ( m_coreType == CoreTypeID::Microseconds )
            {
                m_value_cached = m_value_imgui = reinterpret_cast<Microseconds*>( m_pPropertyInstance )->ToFloat();
            }
            else if ( m_coreType == CoreTypeID::Milliseconds )
            {
                m_value_cached = m_value_imgui = reinterpret_cast<Milliseconds*>( m_pPropertyInstance )->ToFloat();
            }
            else if ( m_coreType == CoreTypeID::Seconds )
            {
                m_value_cached = m_value_imgui = reinterpret_cast<Seconds*>( m_pPropertyInstance )->ToFloat();
            }
        }

        virtual void HandleExternalUpdate() override
        {
            float actualValue = 0;

            if ( m_coreType == CoreTypeID::Microseconds )
            {
                actualValue = reinterpret_cast<Microseconds*>( m_pPropertyInstance )->ToFloat();
            }
            else if ( m_coreType == CoreTypeID::Milliseconds )
            {
                actualValue = reinterpret_cast<Milliseconds*>( m_pPropertyInstance )->ToFloat();
            }
            else if ( m_coreType == CoreTypeID::Seconds )
            {
                actualValue = reinterpret_cast<Seconds*>( m_pPropertyInstance )->ToFloat();
            }

            if ( actualValue != m_value_cached )
            {
                m_value_cached = m_value_imgui = actualValue;
            }
        }

    private:

        float                m_value_imgui;
        float                m_value_cached;
    };

    //-------------------------------------------------------------------------

    class PercentageEditor final : public PropertyEditor
    {
    public:

        PercentageEditor( PropertyEditorContext const& context, PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance )
            : PropertyEditor( context, propertyInfo, pTypeInstance, pPropertyInstance )
        {
            PercentageEditor::ResetWorkingCopy();
        }

        virtual Result InternalUpdateAndDraw() override
        {
            float const textAreaWidth = ImGui::GetContentRegionAvail().x - g_iconButtonWidth - ImGui::GetStyle().ItemSpacing.x;

            ImGui::SetNextItemWidth( textAreaWidth );
            ImGui::InputFloat( "##pe", &m_value_imgui, 0, 0, "%.2f%%" );
            bool valueChanged = ImGui::IsItemDeactivatedAfterEdit();

            //-------------------------------------------------------------------------

            ImGui::SameLine();
            if ( ImGui::Button( EE_ICON_PERCENT_BOX_OUTLINE"##ClampPercentage", ImVec2( g_iconButtonWidth, 0 ) ) )
            {
                m_value_imgui = ( Percentage( m_value_imgui / 100 ).GetClamped( true ) ).ToFloat() * 100;
                valueChanged = true;
            }
            ImGuiX::ItemTooltip( "Clamp [-100 : 100]" );

            return valueChanged ? Result::ValueUpdated : Result::None;
        }

        virtual void UpdatePropertyValue() override
        {
            m_value_cached = m_value_imgui;
            *reinterpret_cast<Percentage*>( m_pPropertyInstance ) = Percentage( m_value_cached / 100.0f );
        }

        virtual void ResetWorkingCopy() override
        {
            m_value_cached = m_value_imgui = reinterpret_cast<Percentage*>( m_pPropertyInstance )->ToFloat() * 100;
        }

        virtual void HandleExternalUpdate() override
        {
            auto actualValue = reinterpret_cast<Percentage*>( m_pPropertyInstance )->ToFloat() * 100;;
            if ( actualValue != m_value_cached )
            {
                m_value_cached = m_value_imgui = actualValue;
            }
        }

    private:

        float  m_value_imgui;
        float  m_value_cached;
    };

    //-------------------------------------------------------------------------

    class TransformEditor final : public PropertyEditor
    {
    public:

        TransformEditor( PropertyEditorContext const& context, PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance )
            : PropertyEditor( context, propertyInfo, pTypeInstance, pPropertyInstance )
        {
            TransformEditor::ResetWorkingCopy();
            EE_ASSERT( m_coreType == CoreTypeID::Transform || m_coreType == CoreTypeID::Matrix );
        }

        virtual Result InternalUpdateAndDraw() override
        {
            bool transformUpdated = false;
            constexpr float const headerWidth = 24;

            ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 0, 2 ) );
            if ( ImGui::BeginTable( "Transform", 2, ImGuiTableFlags_None ) )
            {
                ImGui::TableSetupColumn( "Header", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, headerWidth );
                ImGui::TableSetupColumn( "Values", ImGuiTableColumnFlags_NoHide );

                ImGui::TableNextRow();
                {
                    ImGui::TableNextColumn();
                    ImGui::AlignTextToFramePadding();
                    {
                        ImGuiX::ScopedFont const sf( ImGuiX::Font::Medium );
                        ImGui::Text( EE_ICON_ROTATE_360 );
                        ImGuiX::TextTooltip( "Rotation" );
                    }

                    ImGui::TableNextColumn();
                    if ( ImGuiX::InputFloat3( "R", m_rotation_imgui ) )
                    {
                        m_rotation_imgui.m_x = Degrees( m_rotation_imgui.m_x ).GetClamped180().ToFloat();
                        m_rotation_imgui.m_y = Degrees( m_rotation_imgui.m_y ).GetClamped180().ToFloat();
                        m_rotation_imgui.m_z = Degrees( m_rotation_imgui.m_z ).GetClamped180().ToFloat();
                        m_rotation_cached = m_rotation_imgui;
                        transformUpdated = true;
                    }
                }

                ImGui::TableNextRow();
                {
                    ImGui::TableNextColumn();
                    ImGui::AlignTextToFramePadding();
                    {
                        ImGuiX::ScopedFont const sf( ImGuiX::Font::Medium );
                        ImGui::Text( EE_ICON_AXIS_ARROW );
                        ImGuiX::TextTooltip( "Translation" );
                    }

                    ImGui::TableNextColumn();
                    if ( ImGuiX::InputFloat3( "T", m_translation_imgui ) )
                    {
                        m_translation_cached = m_translation_imgui;
                        transformUpdated = true;
                    }
                }

                ImGui::TableNextRow();
                {
                    ImGui::TableNextColumn();
                    ImGui::AlignTextToFramePadding();

                    ImGuiX::ScopedFont const sf( ImGuiX::Font::Medium );
                    ImGui::Text( EE_ICON_ARROW_TOP_RIGHT_BOTTOM_LEFT );
                    ImGuiX::TextTooltip( "Scale" );

                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth( -1 );
                    if ( ImGui::InputFloat( "##S", &m_scale_imgui ) )
                    {
                        m_scale_imgui = Math::IsNearZero( m_scale_imgui ) ? 0.01f : m_scale_imgui;
                        m_scale_cached = m_scale_imgui;
                        transformUpdated = true;
                    }
                }

                ImGui::EndTable();
            }
            ImGui::PopStyleVar();

            return transformUpdated ? Result::ValueUpdated : Result::None;
        }

        virtual void UpdatePropertyValue() override
        {
            EulerAngles const rot( m_rotation_cached );
            Quaternion const q( rot );

            if ( m_coreType == CoreTypeID::Transform )
            {
                *reinterpret_cast<Transform*>( m_pPropertyInstance ) = Transform( q, m_translation_cached, m_scale_cached );
            }
            else if ( m_coreType == CoreTypeID::Matrix )
            {
                *reinterpret_cast<Matrix*>( m_pPropertyInstance ) = Matrix( q, m_translation_cached, m_scale_cached );
            }
        }

        virtual void ResetWorkingCopy() override
        {
            if ( m_coreType == CoreTypeID::Transform )
            {
                auto const& transform = reinterpret_cast<Transform*>( m_pPropertyInstance );

                m_rotation_cached = m_rotation_imgui = transform->GetRotation().ToEulerAngles().GetAsDegrees();
                m_translation_cached = m_translation_imgui = transform->GetTranslation().ToFloat3();
                m_scale_cached = m_scale_imgui = transform->GetScale();
            }
            else if ( m_coreType == CoreTypeID::Matrix )
            {
                auto const& matrix = reinterpret_cast<Matrix*>( m_pPropertyInstance );

                Quaternion q;
                Vector t;
                Vector s;
                matrix->Decompose( q, t, s );

                m_rotation_cached = m_rotation_imgui = q.ToEulerAngles().GetAsDegrees();
                m_translation_cached = m_translation_imgui = t.ToFloat3();
                m_scale_cached = m_scale_imgui = s.ToFloat();
            }
        }

        virtual void HandleExternalUpdate() override
        {
            Quaternion actualQ;
            Vector actualTranslation;
            float actualScale = 1.0f;

            // Get actual transform values
            //-------------------------------------------------------------------------

            if ( m_coreType == CoreTypeID::Transform )
            {
                auto const& transform = reinterpret_cast<Transform*>( m_pPropertyInstance );
                actualQ = transform->GetRotation();
                actualTranslation = transform->GetTranslation();
                actualScale = transform->GetScale();
            }
            else if ( m_coreType == CoreTypeID::Matrix )
            {
                auto const& matrix = reinterpret_cast<Matrix*>( m_pPropertyInstance );
                Vector vScale;
                matrix->Decompose( actualQ, actualTranslation, vScale );
                actualScale = vScale.ToFloat();
            }

            // Update the cached (and the imgui transform) when the actual is sufficiently different
            //-------------------------------------------------------------------------

            EulerAngles const currentRotation( m_rotation_cached );
            Quaternion const currentQ( currentRotation );

            Radians const angularDistance = Quaternion::Distance( currentQ, actualQ );
            if ( angularDistance > Degrees( 0.5f ) )
            {
                m_rotation_cached = actualQ.ToEulerAngles().GetAsDegrees();
                m_rotation_imgui = m_rotation_cached;
            }

            if ( !actualTranslation.IsNearEqual3( m_translation_cached ) )
            {
                m_translation_cached = actualTranslation.ToFloat3();
                m_translation_imgui = m_translation_cached;
            }

            if ( !Math::IsNearEqual( actualScale, m_scale_cached ) )
            {
                m_scale_cached = actualScale;
                m_scale_imgui = m_scale_cached;
            }
        }

    private:

        Float3                m_rotation_imgui;
        Float3                m_translation_imgui;
        float                 m_scale_imgui;

        Float3                m_rotation_cached;
        Float3                m_translation_cached;
        float                 m_scale_cached;
    };

    //-------------------------------------------------------------------------

    class DataPathEditor final : public PropertyEditor
    {
    public:

        DataPathEditor( PropertyEditorContext const& context, PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance )
            : PropertyEditor( context, propertyInfo, pTypeInstance, pPropertyInstance )
            , m_picker( context.m_pToolsContext->GetSourceDataDirectory() )
        {
            DataPathEditor::ResetWorkingCopy();
            EE_ASSERT( m_coreType == CoreTypeID::DataPath );
        }

        virtual Result InternalUpdateAndDraw() override
        {
            return m_picker.UpdateAndDraw() ? Result::ValueUpdated : Result::None;
        }

        virtual void UpdatePropertyValue() override
        {
            m_value_cached = m_picker.GetPath();
            *reinterpret_cast<DataPath*>( m_pPropertyInstance ) = m_value_cached;
        }

        virtual void ResetWorkingCopy() override
        {
            m_value_cached = *reinterpret_cast<DataPath*>( m_pPropertyInstance );
            m_picker.SetPath( m_value_cached );
        }

        virtual void HandleExternalUpdate() override
        {
            DataPath const* pActualPath = nullptr;
            pActualPath = reinterpret_cast<DataPath*>( m_pPropertyInstance );
            if ( *pActualPath != m_value_cached )
            {
                m_value_cached = *pActualPath;
                m_picker.SetPath( m_value_cached );
            }
        }

    private:

        DataPathPicker                  m_picker;
        DataPath                        m_value_cached;
    };

    //-------------------------------------------------------------------------

    class DataFilePathEditor final : public PropertyEditor
    {
    public:

        DataFilePathEditor( PropertyEditorContext const& context, PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance )
            : PropertyEditor( context, propertyInfo, pTypeInstance, pPropertyInstance )
            , m_picker( *context.m_pToolsContext, propertyInfo.m_templateArgumentTypeID )
        {
            DataFilePathEditor::ResetWorkingCopy();
        }

        virtual Result InternalUpdateAndDraw() override
        {
            return m_picker.UpdateAndDraw() ? Result::ValueUpdated : Result::None;
        }

        virtual void UpdatePropertyValue() override
        {
            m_value_cached = m_picker.GetPath();
            *reinterpret_cast<DataPath*>( m_pPropertyInstance ) = m_value_cached.IsValid() ? m_value_cached : DataPath();
        }

        virtual void ResetWorkingCopy() override
        {
            m_value_cached = *reinterpret_cast<DataPath*>( m_pPropertyInstance );
            m_picker.SetPath( m_value_cached );
        }

        virtual void HandleExternalUpdate() override
        {
            DataPath value_external = *reinterpret_cast<DataPath*>( m_pPropertyInstance );
            if ( value_external != m_value_cached )
            {
                m_value_cached = value_external;
            }
            m_picker.SetPath( m_value_cached );
        }

    private:

        DataFilePathPicker                  m_picker;
        DataPath                            m_value_cached;
    };

    //-------------------------------------------------------------------------

    class ResourceIDEditor final : public PropertyEditor
    {
    public:

        ResourceIDEditor( PropertyEditorContext const& context, PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance )
            : PropertyEditor( context, propertyInfo, pTypeInstance, pPropertyInstance )
            , m_picker( *context.m_pToolsContext )
        {
            ResourceIDEditor::ResetWorkingCopy();
            EE_ASSERT( m_coreType == CoreTypeID::ResourceID || m_coreType == CoreTypeID::ResourcePtr || m_coreType == CoreTypeID::TResourcePtr );

            if ( m_coreType == CoreTypeID::TResourcePtr )
            {
               m_picker.SetRequiredResourceType( GetTypeRegistry()->GetResourceInfo( propertyInfo.m_templateArgumentTypeID )->m_resourceTypeID );
            }
        }

        virtual Result InternalUpdateAndDraw() override
        {
            return m_picker.UpdateAndDraw() ? Result::ValueUpdated : Result::None;
        }

        virtual void UpdatePropertyValue() override
        {
            m_value_cached = m_picker.GetResourceID();

            if ( m_coreType == CoreTypeID::ResourceID )
            {
                *reinterpret_cast<ResourceID*>( m_pPropertyInstance ) = m_value_cached.IsValid() ? ResourceID( m_value_cached ) : ResourceID();
            }
            else if ( m_coreType == CoreTypeID::ResourcePtr || m_coreType == CoreTypeID::TResourcePtr )
            {
                *reinterpret_cast<Resource::ResourcePtr*>( m_pPropertyInstance ) = m_value_cached.IsValid() ? ResourceID( m_value_cached ) : ResourceID();
            }
        }

        virtual void ResetWorkingCopy() override
        {
            if ( m_coreType == CoreTypeID::ResourceID )
            {
                m_value_cached = *reinterpret_cast<ResourceID*>( m_pPropertyInstance );
            }
            else if ( m_coreType == CoreTypeID::ResourcePtr || m_coreType == CoreTypeID::TResourcePtr )
            {
                m_value_cached = reinterpret_cast<Resource::ResourcePtr*>( m_pPropertyInstance )->GetResourceID();
            }

            m_picker.SetResourceID( m_value_cached );
        }

        virtual void HandleExternalUpdate() override
        {
            if ( m_coreType == CoreTypeID::ResourceID )
            {
                ResourceID* pResourceID = reinterpret_cast<ResourceID*>( m_pPropertyInstance );
                if ( *pResourceID != m_value_cached )
                {
                    m_value_cached = *pResourceID;
                }
            }
            else if ( m_coreType == CoreTypeID::ResourcePtr || m_coreType == CoreTypeID::TResourcePtr )
            {
                ResourceID resourceID = reinterpret_cast<Resource::ResourcePtr*>( m_pPropertyInstance )->GetResourceID();
                if ( resourceID != m_value_cached )
                {
                    m_value_cached = resourceID;
                }
            }

            m_picker.SetResourceID( m_value_cached );
        }

    private:

        ResourcePicker                          m_picker;
        ResourceID                              m_value_cached;
    };

    //-------------------------------------------------------------------------

    class ResourceTypeIDEditor final : public PropertyEditor
    {
    public:

        ResourceTypeIDEditor( PropertyEditorContext const& context, PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance )
            : PropertyEditor( context, propertyInfo, pTypeInstance, pPropertyInstance )
        {
            ResourceTypeIDEditor::ResetWorkingCopy();
        }

        virtual Result InternalUpdateAndDraw() override
        {
            bool valueChanged = false;

            //-------------------------------------------------------------------------

            ImGui::SetNextItemWidth( -1 );
            if ( ImGui::BeginCombo( "##resTypeID", m_resourceTypeFriendlyName.c_str() ) )
            {
                auto AddComboItem = [this, &valueChanged]( ResourceInfo const& resourceInfo )
                {
                    bool const isSelected = ( m_value_imgui.m_ID == resourceInfo.m_resourceTypeID );
                    if ( ImGui::Selectable( resourceInfo.m_friendlyName.empty() ? "None" : resourceInfo.m_friendlyName.c_str(), isSelected ) )
                    {
                        if ( resourceInfo.m_resourceTypeID != m_value_imgui )
                        {
                            m_value_imgui = resourceInfo.m_resourceTypeID;
                            m_resourceTypeFriendlyName = resourceInfo.m_friendlyName;
                            valueChanged = true;
                        }
                    }

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if ( isSelected )
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                };

                //-------------------------------------------------------------------------

                AddComboItem( ResourceInfo() );

                //-------------------------------------------------------------------------

                auto const& registeredResourceTypes = GetTypeRegistry()->GetRegisteredResourceTypes();
                for ( auto& pair : registeredResourceTypes )
                {
                    AddComboItem( *pair.second );
                }

                ImGui::EndCombo();
            }

            //-------------------------------------------------------------------------

            return valueChanged ? Result::ValueUpdated : Result::None;
        }

        virtual void UpdatePropertyValue() override
        {
            m_value_cached = m_value_imgui;
            *reinterpret_cast<ResourceTypeID*>( m_pPropertyInstance ) = m_value_cached;
        }

        virtual void ResetWorkingCopy() override
        {
            m_value_cached = m_value_imgui = *reinterpret_cast<ResourceTypeID*>( m_pPropertyInstance );

            if ( m_value_cached.IsValid() )
            {
                m_resourceTypeFriendlyName = GetTypeRegistry()->GetResourceInfo( m_value_cached )->m_friendlyName;
            }
            else
            {
                m_resourceTypeFriendlyName = "";
            }
        }

        virtual void HandleExternalUpdate() override
        {
            auto actualValue = *reinterpret_cast<ResourceTypeID*>( m_pPropertyInstance );
            if ( actualValue != m_value_cached )
            {
                ResetWorkingCopy();
            }
        }

    private:

        ResourceTypeID              m_value_imgui;
        ResourceTypeID              m_value_cached;
        String                      m_resourceTypeFriendlyName;
    };

    //-------------------------------------------------------------------------

    class TypeInstanceEditor final : public PropertyEditor
    {
    public:

        TypeInstanceEditor( PropertyEditorContext const& context, PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance )
            : PropertyEditor( context, propertyInfo, pTypeInstance, pPropertyInstance )
            , m_picker( *context.m_pToolsContext )
        {
            TypeInstanceEditor::ResetWorkingCopy();
            EE_ASSERT( m_coreType == CoreTypeID::TypeInstance || m_coreType == CoreTypeID::TTypeInstance );

            bool const isTypePickerDisabled = m_propertyInfo.m_metadata.HasFlag( PropertyMetadata::DisableTypePicker );
            m_picker.SetDisabled( isTypePickerDisabled );

            if ( m_coreType == CoreTypeID::TTypeInstance )
            {
                TypeInstance* pTypeInstanceContainer = reinterpret_cast<TypeInstance*>( m_pPropertyInstance );
                m_picker.SetRequiredBaseClass( pTypeInstanceContainer->GetAllowedBaseTypeInfo()->m_ID );
            }
        }

        virtual Result InternalUpdateAndDraw() override
        {
            return m_picker.UpdateAndDraw() ? Result::ValueUpdated : Result::None;
        }

        virtual void UpdatePropertyValue() override
        {
            TypeSystem::TypeInfo const* pSelectedType = m_picker.GetSelectedType();
            m_value_cached = ( pSelectedType == nullptr ) ? TypeSystem::TypeID() : pSelectedType->m_ID;

            // Update the container
            TypeInstance* pTypeInstanceContainer = reinterpret_cast<TypeInstance*>( m_pPropertyInstance );
            if ( pTypeInstanceContainer->GetInstanceTypeID() != m_value_cached )
            {
                pTypeInstanceContainer->DestroyInstance();

                if ( m_value_cached.IsValid() )
                {
                    pTypeInstanceContainer->CreateInstance( pSelectedType );
                }
            }
        }

        virtual void ResetWorkingCopy() override
        {
            TypeInstance* pTypeInstanceContainer = reinterpret_cast<TypeInstance*>( m_pPropertyInstance );
            TypeSystem::TypeID instanceTypeID = pTypeInstanceContainer->GetInstanceTypeID();
            m_value_cached = instanceTypeID;
            m_picker.SetSelectedType( m_value_cached );
        }

        virtual void HandleExternalUpdate() override
        {
            TypeInstance* pTypeInstanceContainer = reinterpret_cast<TypeInstance*>( m_pPropertyInstance );
            TypeSystem::TypeID instanceTypeID = pTypeInstanceContainer->GetInstanceTypeID();
            if ( instanceTypeID != m_value_cached )
            {
                ResetWorkingCopy();
            }
        }

    private:

        TypeInfoPicker                          m_picker;
        TypeSystem::TypeID                      m_value_cached;
    };

    //-------------------------------------------------------------------------

    class BitflagsEditor final : public PropertyEditor
    {
    public:

        BitflagsEditor( PropertyEditorContext const& context, PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance )
            : PropertyEditor( context, propertyInfo, pTypeInstance, pPropertyInstance )
        {
            BitflagsEditor::ResetWorkingCopy();
            EE_ASSERT( m_coreType == CoreTypeID::BitFlags || m_coreType == CoreTypeID::TBitFlags );
        }

        virtual Result InternalUpdateAndDraw() override
        {
            bool valueChanged = false;

            if ( m_coreType == CoreTypeID::BitFlags )
            {
                ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 4, 4 ) );
                if ( ImGui::BeginTable( "FlagsTable", 9, ImGuiTableFlags_SizingFixedFit ) )
                {
                    constexpr static char const* const rowLabels[4] = { "00-07", "08-15", "16-23", "24-31" };

                    for ( uint8_t i = 0u; i < 4; i++ )
                    {
                        ImGui::TableNextRow();

                        for ( uint8_t j = 0u; j < 8; j++ )
                        {
                            uint8_t const flagIdx = i * 8 + j;
                            ImGui::TableNextColumn();

                            ImGui::PushID( &m_values_imgui[flagIdx] );
                            if ( ImGui::Checkbox( "##flag", &m_values_imgui[flagIdx] ) )
                            {
                                valueChanged = true;
                            }
                            ImGui::PopID();
                        }

                        ImGui::TableNextColumn();
                        ImGui::Text( rowLabels[i] );
                    }
                    ImGui::EndTable();
                }
                ImGui::PopStyleVar();
            }
            else if ( m_coreType == CoreTypeID::TBitFlags )
            {
                // Get enum type for specific flags
                TypeID const enumTypeID = m_propertyInfo.m_templateArgumentTypeID;
                EnumInfo const* pEnumInfo = GetTypeRegistry()->GetEnumInfo( enumTypeID );
                EE_ASSERT( pEnumInfo != nullptr );

                //-------------------------------------------------------------------------

                ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 4, 4 ) );
                if ( ImGui::BeginTable( "FlagsTable", 2, ImGuiTableFlags_SizingFixedFit ) )
                {
                    int32_t flagCount = 0;

                    // For each label 
                    for ( auto const& constant : pEnumInfo->m_constants )
                    {
                        if ( ( flagCount % 2 ) == 0 )
                        {
                            ImGui::TableNextRow();
                        }

                        ImGui::TableNextColumn();
                        flagCount++;

                        //-------------------------------------------------------------------------

                        int64_t const flagValue = constant.m_value;
                        EE_ASSERT( flagValue >= 0 && flagValue <= 31 );
                        if ( ImGui::Checkbox( constant.m_ID.c_str(), &m_values_imgui[flagValue] ) )
                        {
                            valueChanged = true;
                        }
                    }
                    ImGui::EndTable();
                }
                ImGui::PopStyleVar();
            }

            //-------------------------------------------------------------------------

            return valueChanged ? Result::ValueUpdated : Result::None;
        }

        virtual void UpdatePropertyValue() override
        {
            auto pFlags = reinterpret_cast<BitFlags*>( m_pPropertyInstance );
            for ( uint8_t i = 0; i < 32; i++ )
            {
                pFlags->SetFlag( i, m_values_imgui[i] );
            }

            m_cachedFlags = pFlags->Get();
        }

        virtual void ResetWorkingCopy() override
        {
            auto pFlags = reinterpret_cast<BitFlags*>( m_pPropertyInstance );
            for ( uint8_t i = 0; i < 32; i++ )
            {
                m_values_imgui[i] = pFlags->IsFlagSet( i );
            }

            m_cachedFlags = pFlags->Get();
        }

        virtual void HandleExternalUpdate() override
        {
            auto pFlags = reinterpret_cast<BitFlags*>( m_pPropertyInstance );
            if ( pFlags->Get() != m_cachedFlags )
            {
                ResetWorkingCopy();
            }
        }

    private:

        bool        m_values_imgui[32];
        uint32_t    m_cachedFlags = 0;
    };

    //-------------------------------------------------------------------------

    class StringEditor final : public PropertyEditor
    {
    public:

        StringEditor( PropertyEditorContext const& context, PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance )
            : PropertyEditor( context, propertyInfo, pTypeInstance, pPropertyInstance )
        {
            StringEditor::ResetWorkingCopy();
        }

        virtual Result InternalUpdateAndDraw() override
        {
            ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x );
            ImGui::InputText( "##stringEd", m_buffer_imgui, 256 );
            return ImGui::IsItemDeactivatedAfterEdit() ? Result::ValueUpdated : Result::None;
        }

        virtual void UpdatePropertyValue() override
        {
            strcpy_s( m_buffer_cached, 256, m_buffer_imgui );
            *reinterpret_cast<String*>( m_pPropertyInstance ) = m_buffer_cached;
        }

        virtual void ResetWorkingCopy() override
        {
            String* pValue = reinterpret_cast<String*>( m_pPropertyInstance );
            strcpy_s( m_buffer_imgui, 256, pValue->c_str() );
            strcpy_s( m_buffer_cached, 256, pValue->c_str() );
        }

        virtual void HandleExternalUpdate() override
        {
            String* pActualValue = reinterpret_cast<String*>( m_pPropertyInstance );
            if ( strcmp( pActualValue->c_str(), m_buffer_cached ) != 0 )
            {
                strcpy_s( m_buffer_imgui, 256, pActualValue->c_str() );
                strcpy_s( m_buffer_cached, 256, pActualValue->c_str() );
            }
        }

    private:

        char    m_buffer_imgui[256];
        char    m_buffer_cached[256];
    };

    //-------------------------------------------------------------------------

    class StringIDEditor final : public PropertyEditor
    {
        constexpr static uint32_t const s_bufferSize = 256;

    public:

        StringIDEditor( PropertyEditorContext const& context, PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance )
            : PropertyEditor( context, propertyInfo, pTypeInstance, pPropertyInstance )
        {
            StringIDEditor::ResetWorkingCopy();
        }

        virtual Result InternalUpdateAndDraw() override
        {
            bool valueUpdated = false;

            //-------------------------------------------------------------------------

            ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x - 60 );
            if ( ImGui::InputText( "##StringInput", m_buffer_imgui, s_bufferSize, ImGuiInputTextFlags_EnterReturnsTrue ) )
            {
                valueUpdated = true;
            }
            
            if ( ImGui::IsItemDeactivatedAfterEdit() )
            {
                valueUpdated = true;
            }

            if ( valueUpdated )
            {
                StringUtils::StripTrailingWhitespace( m_buffer_imgui );
            }

            //-------------------------------------------------------------------------

            ImGui::SameLine();

            ImGui::SetNextItemWidth( -1 );
            {
                ImGuiX::ScopedFont const sf( ImGuiX::Font::Tiny );
                ImGui::AlignTextToFramePadding();
                ImGui::TextColored( Colors::LightGreen.ToFloat4(), m_IDString.data() );
            }

            return valueUpdated ? Result::ValueUpdated : Result::None;
        }

        virtual void UpdatePropertyValue() override
        {
            auto pValue = reinterpret_cast<StringID*>( m_pPropertyInstance );

            if ( strlen( m_buffer_imgui ) > 0 )
            {
                m_buffer_cached = m_buffer_imgui;
                *pValue = StringID( m_buffer_imgui );
                m_IDString.sprintf( "%u", pValue->ToUint() );
            }
            else
            {
                pValue->Clear();
                m_buffer_cached.clear();
                m_IDString = "Invalid";
            }
        }

        virtual void ResetWorkingCopy() override
        {
            auto pValue = reinterpret_cast<StringID*>( m_pPropertyInstance );
            if ( pValue->IsValid() )
            {
                strcpy_s( m_buffer_imgui, 256, pValue->c_str() );
                m_buffer_cached = pValue->c_str();
                m_IDString.sprintf( "%u", pValue->ToUint() );
            }
            else
            {
                Memory::MemsetZero( m_buffer_imgui, s_bufferSize );
                m_buffer_cached.clear();
                m_IDString = "Invalid";
            }
        }

        virtual void HandleExternalUpdate() override
        {
            StringID const* pActualValue = reinterpret_cast<StringID*>( m_pPropertyInstance );
            if ( pActualValue->IsValid() )
            {
                if ( m_buffer_cached != pActualValue->c_str() )
                {
                    ResetWorkingCopy();
                }
            }
            else // Invalid String ID
            {
                if ( !m_buffer_cached.empty() )
                {
                    ResetWorkingCopy();
                }
            }
        }

    private:

        char                             m_buffer_imgui[s_bufferSize];
        TInlineString<s_bufferSize>      m_buffer_cached;
        TInlineString<s_bufferSize>      m_IDString;
    };

    //-------------------------------------------------------------------------

    class Tag2Editor final : public PropertyEditor
    {
    public:

        Tag2Editor( PropertyEditorContext const& context, PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance )
            : PropertyEditor( context, propertyInfo, pTypeInstance, pPropertyInstance )
        {
            Tag2Editor::ResetWorkingCopy();
        }

        virtual Result InternalUpdateAndDraw() override
        {
            float const cellContentWidth = ImGui::GetContentRegionAvail().x;
            float const elementAreaWidth = ( cellContentWidth - ImGui::GetStyle().ItemSpacing.x ) / 4;
            int32_t const tagDepth = m_value_imgui.GetDepth();
            bool valueUpdated = false;

            //-------------------------------------------------------------------------

            ImGui::SetNextItemWidth( elementAreaWidth );
            ImGui::InputText( "##0", m_buffers[0], 255 );
            if ( ImGui::IsItemDeactivatedAfterEdit() )
            {
                UpdateWorkingValue();
                valueUpdated = true;
            }

            //-------------------------------------------------------------------------

            ImGui::SameLine();
            ImGui::BeginDisabled( tagDepth < 1 );
            ImGui::SetNextItemWidth( elementAreaWidth );
            ImGui::InputText( "##1", m_buffers[1], 255 );
            if ( ImGui::IsItemDeactivatedAfterEdit() )
            {
                UpdateWorkingValue();
                valueUpdated = true;
            }
            ImGui::EndDisabled();

            //-------------------------------------------------------------------------

            return valueUpdated ? Result::ValueUpdated : Result::None;
        }

        virtual void UpdatePropertyValue() override
        {
            m_value_cached = m_value_imgui;
            *reinterpret_cast<Tag2*>( m_pPropertyInstance ) = m_value_cached;
        }

        virtual void ResetWorkingCopy() override
        {
            m_value_cached = m_value_imgui = *reinterpret_cast<Tag2*>( m_pPropertyInstance );
            UpdateBuffers();
        }

        virtual void HandleExternalUpdate() override
        {
            Tag2& actualValue = *reinterpret_cast<Tag2*>( m_pPropertyInstance );
            if ( actualValue != m_value_cached )
            {
                m_value_cached = m_value_imgui = actualValue;
                UpdateBuffers();
            }
        }

    private:

        void UpdateBuffers()
        {
            int32_t const depth = m_value_imgui.GetDepth();
            for ( auto i = 0; i < 2; i++ )
            {
                if ( i < depth )
                {
                    Printf( m_buffers[i], 255, "%s", m_value_imgui.GetValueAtDepth( i ).c_str() );
                }
                else
                {
                    Memory::MemsetZero( m_buffers[i], 255 );
                }
            }
        }

        void UpdateWorkingValue()
        {
            StringID c[4] = { StringID( m_buffers[0] ), StringID( m_buffers[1] ) };

            // Ensure valid tag state
            for ( int32_t i = 0; i < 2; i++ )
            {
                if ( !c[i].IsValid() )
                {
                    for ( int32_t j = i + 1; j < 4; j++ )
                    {
                        c[j].Clear();
                    }
                    break;
                }
            }

            m_value_imgui = Tag2( c[0], c[1] );
            UpdateBuffers();
        }

    private:

        char    m_buffers[2][255] = { {0}, {0} };
        Tag2    m_value_imgui;
        Tag2    m_value_cached;
    };

    //-------------------------------------------------------------------------

    class Tag4Editor final : public PropertyEditor
    {
    public:

        Tag4Editor( PropertyEditorContext const& context, PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance )
            : PropertyEditor( context, propertyInfo, pTypeInstance, pPropertyInstance )
        {
            Tag4Editor::ResetWorkingCopy();
        }

        virtual Result InternalUpdateAndDraw() override
        {
            float const cellContentWidth = ImGui::GetContentRegionAvail().x;
            float const elementAreaWidth = ( cellContentWidth - ImGui::GetStyle().ItemSpacing.x * 3 ) / 4;
            int32_t const tagDepth = m_value_imgui.GetDepth();
            bool valueUpdated = false;

            //-------------------------------------------------------------------------

            ImGui::SetNextItemWidth( elementAreaWidth );
            ImGui::InputText( "##0", m_buffers[0], 255 );
            if ( ImGui::IsItemDeactivatedAfterEdit() )
            {
                UpdateWorkingValue();
                valueUpdated = true;
            }

            //-------------------------------------------------------------------------

            ImGui::SameLine();
            ImGui::BeginDisabled( tagDepth < 1 );
            ImGui::SetNextItemWidth( elementAreaWidth );
            ImGui::InputText( "##1", m_buffers[1], 255 );
            if ( ImGui::IsItemDeactivatedAfterEdit() )
            {
                UpdateWorkingValue();
                valueUpdated = true;
            }
            ImGui::EndDisabled();

            //-------------------------------------------------------------------------

            ImGui::SameLine();
            ImGui::BeginDisabled( tagDepth < 2 );
            ImGui::SetNextItemWidth( elementAreaWidth );
            ImGui::InputText( "##2", m_buffers[2], 255 );
            if ( ImGui::IsItemDeactivatedAfterEdit() )
            {
                UpdateWorkingValue();
                valueUpdated = true;
            }
            ImGui::EndDisabled();

            //-------------------------------------------------------------------------

            ImGui::SameLine();
            ImGui::BeginDisabled( tagDepth < 3 );
            ImGui::SetNextItemWidth( elementAreaWidth );
            ImGui::InputText( "##3", m_buffers[3], 255 );
            if ( ImGui::IsItemDeactivatedAfterEdit() )
            {
                UpdateWorkingValue();
                valueUpdated = true;
            }
            ImGui::EndDisabled();

            //-------------------------------------------------------------------------

            return valueUpdated ? Result::ValueUpdated : Result::None;
        }

        virtual void UpdatePropertyValue() override
        {
            m_value_cached = m_value_imgui;
            *reinterpret_cast<Tag4*>( m_pPropertyInstance ) = m_value_cached;
        }

        virtual void ResetWorkingCopy() override
        {
            m_value_cached = m_value_imgui = *reinterpret_cast<Tag4*>( m_pPropertyInstance );
            UpdateBuffers();
        }

        virtual void HandleExternalUpdate() override
        {
            Tag4& actualValue = *reinterpret_cast<Tag4*>( m_pPropertyInstance );
            if ( actualValue != m_value_cached )
            {
                m_value_cached = m_value_imgui = actualValue;
                UpdateBuffers();
            }
        }

    private:

        void UpdateBuffers()
        {
            int32_t const depth = m_value_imgui.GetDepth();
            for ( auto i = 0; i < 4; i++ )
            {
                if ( i < depth )
                {
                    Printf( m_buffers[i], 255, "%s", m_value_imgui.GetValueN( i ).c_str() );
                }
                else
                {
                    Memory::MemsetZero( m_buffers[i], 255 );
                }
            }
        }

        void UpdateWorkingValue()
        {
            StringID c[4] = { StringID( m_buffers[0] ), StringID( m_buffers[1] ), StringID( m_buffers[2] ), StringID( m_buffers[3] ) };

            // Ensure valid tag state
            for ( int32_t i = 0; i < 4; i++ )
            {
                if ( !c[i].IsValid() )
                {
                    for ( int32_t j = i + 1; j < 4; j++ )
                    {
                        c[j].Clear();
                    }
                    break;
                }
            }

            m_value_imgui = Tag4( c[0], c[1], c[2], c[3] );
            UpdateBuffers();
        }

    private:

        char    m_buffers[4][255] = { {0}, {0}, {0}, {0} };
        Tag4    m_value_imgui;
        Tag4    m_value_cached;
    };

    //-------------------------------------------------------------------------

    class IntRangeEditor final : public PropertyEditor
    {
    public:

        IntRangeEditor( PropertyEditorContext const& context, PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance )
            : PropertyEditor( context, propertyInfo, pTypeInstance, pPropertyInstance )
        {
            IntRangeEditor::ResetWorkingCopy();
        }

        virtual Result InternalUpdateAndDraw() override
        {
            bool valueUpdated = false;

            constexpr float const verticalOffset = 3.0f;
            float const columnWidth = ImGui::GetColumnWidth();
            float const cursorStartPosX = ImGui::GetCursorScreenPos().x;

            //-------------------------------------------------------------------------

            bool isSet = m_value_imgui.IsSet();
            ImGui::SetCursorPosY( ImGui::GetCursorPosY() + verticalOffset );
            ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 1, 1 ) );
            if ( ImGui::Checkbox( "##isRangeSet", &isSet ) )
            {
                if ( isSet )
                {
                    m_value_imgui.m_begin = 0;
                    m_value_imgui.m_end = 1;
                }
                else
                {
                    m_value_imgui.Clear();
                }

                valueUpdated = true;
            }
            ImGui::PopStyleVar();
            ImGui::SameLine();
            ImGui::SetCursorPosY( ImGui::GetCursorPosY() - verticalOffset );

            //-------------------------------------------------------------------------

            float const cursorEndPosX = ImGui::GetCursorScreenPos().x;
            float const inputWidth = ( columnWidth - ( cursorEndPosX - cursorStartPosX ) - ( ImGui::GetStyle().ItemSpacing.x * 2 ) ) / 2;
            int32_t tmpValue = 0;

            //-------------------------------------------------------------------------

            ImGui::BeginDisabled( !isSet );
            ImGui::SetNextItemWidth( inputWidth );
            ImGui::InputScalar( "##min", ImGuiDataType_S32, isSet ? &m_value_imgui.m_begin : &tmpValue, 0, 0 );
            if ( ImGui::IsItemDeactivatedAfterEdit() )
            {
                ValidateUserInput( true );
                valueUpdated = true;
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::SetCursorPosY( ImGui::GetCursorPosY() - verticalOffset );

            //-------------------------------------------------------------------------

            ImGui::BeginDisabled( !isSet );
            ImGui::SetNextItemWidth( inputWidth );
            ImGui::InputScalar( "##max", ImGuiDataType_S32, isSet ? &m_value_imgui.m_end : &tmpValue, 0, 0 );
            if ( ImGui::IsItemDeactivatedAfterEdit() )
            {
                ValidateUserInput( false );
                valueUpdated = true;
            }
            ImGui::EndDisabled();

            //-------------------------------------------------------------------------

            return valueUpdated ? Result::ValueUpdated : Result::None;
        }

        virtual void UpdatePropertyValue() override
        {
            m_value_cached = m_value_imgui;
            *reinterpret_cast<IntRange*>( m_pPropertyInstance ) = m_value_cached;
        }

        virtual void ResetWorkingCopy() override
        {
            m_value_cached = m_value_imgui = *reinterpret_cast<IntRange*>( m_pPropertyInstance );

            // Ensure we always have a valid range!
            if ( m_value_imgui.IsSet() )
            {
                if ( m_value_imgui.IsValid() )
                {
                    m_value_imgui.Clear();
                    m_value_cached = m_value_imgui;
                }
            }
        }

        virtual void HandleExternalUpdate() override
        {
            auto& actualValue = *reinterpret_cast<IntRange*>( m_pPropertyInstance );
            if ( actualValue != m_value_cached )
            {
                m_value_cached = m_value_imgui = actualValue;
            }
        }

    private:

        // Ensure users cant input an invalid range
        void ValidateUserInput( bool wasRangeStartEdited )
        {
            if ( wasRangeStartEdited )
            {
                if ( m_value_imgui.m_begin > m_value_imgui.m_end )
                {
                    m_value_imgui.m_begin = m_value_imgui.m_end;
                }
            }
            else
            {
                if ( m_value_imgui.m_end < m_value_imgui.m_begin )
                {
                    m_value_imgui.m_end = m_value_imgui.m_begin;
                }
            }
        }

    private:

        IntRange m_value_imgui;
        IntRange m_value_cached;
    };

    //-------------------------------------------------------------------------

    class FloatRangeEditor final : public PropertyEditor
    {
    public:

        FloatRangeEditor( PropertyEditorContext const& context, PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance )
            : PropertyEditor( context, propertyInfo, pTypeInstance, pPropertyInstance )
        {
            FloatRangeEditor::ResetWorkingCopy();
        }

        virtual Result InternalUpdateAndDraw() override
        {
            bool valueUpdated = false;

            constexpr float const verticalOffset = 4.0f;
            float const columnWidth = ImGui::GetColumnWidth();
            float const cursorStartPosX = ImGui::GetCursorScreenPos().x;

            //-------------------------------------------------------------------------

            bool isSet = m_value_imgui.IsSet();
            ImGui::SetCursorPosY( ImGui::GetCursorPosY() + verticalOffset );
            ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 1, 1 ) );
            if ( ImGui::Checkbox( "##isRangeSet", &isSet ) )
            {
                if ( isSet )
                {
                    m_value_imgui.m_begin = 0;
                    m_value_imgui.m_end = 1;
                }
                else
                {
                    m_value_imgui.Clear();
                }

                valueUpdated = true;
            }
            ImGui::PopStyleVar();
            ImGui::SameLine();
            ImGui::SetCursorPosY( ImGui::GetCursorPosY() - verticalOffset );

            //-------------------------------------------------------------------------

            static constexpr char const separatorText[] = "to";
            float const cursorEndPosX = ImGui::GetCursorScreenPos().x;
            float const inputWidth = ( columnWidth - ImGui::CalcTextSize( separatorText ).x - ( cursorEndPosX - cursorStartPosX ) - ( ImGui::GetStyle().ItemSpacing.x * 3 ) ) / 2;
            float tmpValue = 0;

            //-------------------------------------------------------------------------

            ImGui::BeginDisabled( !isSet );
            ImGui::SetNextItemWidth( inputWidth );
            ImGui::InputFloat( "##min", isSet ? &m_value_imgui.m_begin : &tmpValue, 0, 0, "%.3f", 0 );
            if ( ImGui::IsItemDeactivatedAfterEdit() )
            {
                ValidateUserInput( true );
                valueUpdated = true;
            }
            ImGui::EndDisabled();

            //-------------------------------------------------------------------------

            ImGui::SameLine();
            ImGui::SetCursorPosY( ImGui::GetCursorPosY() - verticalOffset );
            ImGui::Text( separatorText );

            //-------------------------------------------------------------------------

            ImGui::SameLine( 0, ImGui::GetStyle().ItemSpacing.x );
            ImGui::SetCursorPosY( ImGui::GetCursorPosY() - verticalOffset );
            ImGui::BeginDisabled( !isSet );
            ImGui::SetNextItemWidth( inputWidth );
            ImGui::InputFloat( "##max", isSet ? &m_value_imgui.m_end : &tmpValue, 0, 0, "%.3f", 0 );
            if ( ImGui::IsItemDeactivatedAfterEdit() )
            {
                ValidateUserInput( false );
                valueUpdated = true;
            }
            ImGui::EndDisabled();

            //-------------------------------------------------------------------------

            return valueUpdated ? Result::ValueUpdated : Result::None;
        }

        virtual void UpdatePropertyValue() override
        {
            m_value_cached = m_value_imgui;
            *reinterpret_cast<FloatRange*>( m_pPropertyInstance ) = m_value_cached;
        }

        virtual void ResetWorkingCopy() override
        {
            m_value_cached = m_value_imgui = *reinterpret_cast<FloatRange*>( m_pPropertyInstance );

            // Ensure we always have a valid range!
            if ( m_value_imgui.IsSet() )
            {
                if ( m_value_imgui.IsValid() )
                {
                    m_value_imgui.Clear();
                    m_value_cached = m_value_imgui;
                }
            }
        }

        virtual void HandleExternalUpdate() override
        {
            auto& actualValue = *reinterpret_cast<FloatRange*>( m_pPropertyInstance );
            if ( actualValue != m_value_cached )
            {
                m_value_cached = m_value_imgui = actualValue;
            }
        }

    private:

        // Ensure users cant input an invalid range
        void ValidateUserInput( bool wasRangeStartEdited )
        {
            if ( wasRangeStartEdited )
            {
                if ( m_value_imgui.m_begin > m_value_imgui.m_end )
                {
                    m_value_imgui.m_begin = m_value_imgui.m_end;
                }
            }
            else
            {
                if ( m_value_imgui.m_end < m_value_imgui.m_begin )
                {
                    m_value_imgui.m_end = m_value_imgui.m_begin;
                }
            }
        }

    private:

        FloatRange m_value_imgui;
        FloatRange m_value_cached;
    };

    //-------------------------------------------------------------------------

    class FloatCurveEditor final : public PropertyEditor
    {
    public:

        FloatCurveEditor( PropertyEditorContext const& context, PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance )
            : PropertyEditor( context, propertyInfo, pTypeInstance, pPropertyInstance )
            , m_editor( m_value_imgui )
        {
            FloatCurveEditor::ResetWorkingCopy();
        }

        virtual Result InternalUpdateAndDraw() override
        {
            bool valueChanged = false;
            if ( ImGui::BeginChild( "##Preview", ImVec2( ImGui::GetContentRegionAvail().x, 140 ) ) )
            {
                valueChanged = m_editor.UpdateAndDraw( true );
            }
            ImGui::EndChild();
            return valueChanged ? Result::ValueUpdated : Result::None;
        }

        virtual void UpdatePropertyValue() override
        {
            m_value_cached = m_value_imgui;
            *reinterpret_cast<FloatCurve*>( m_pPropertyInstance ) = m_value_cached;
        }

        virtual void ResetWorkingCopy() override
        {
            auto const& originalCurve = *reinterpret_cast<FloatCurve*>( m_pPropertyInstance );
            m_value_cached = m_value_imgui = originalCurve;

            m_editor.OnCurveExternallyUpdated();
            m_editor.ResetView();
        }

        virtual void HandleExternalUpdate() override
        {
            auto& actualValue = *reinterpret_cast<FloatCurve*>( m_pPropertyInstance );
            if ( actualValue != m_value_cached )
            {
                m_value_cached = m_value_imgui = actualValue;
                m_editor.OnCurveExternallyUpdated();
            }
        }

    private:

        FloatCurve      m_value_imgui;
        FloatCurve      m_value_cached;
        CurveEditor     m_editor;
    };

    //-------------------------------------------------------------------------
    // Factory
    //-------------------------------------------------------------------------

    PropertyEditor* PropertyGridEditorFactory::TryCreateEditor( PropertyEditorContext const& context, TypeSystem::PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance )
    {
        EE_ASSERT( propertyInfo.IsValid() );
        auto typeID = propertyInfo.m_typeID;
        EE_ASSERT( typeID.IsValid() );

        // Check if we have a custom editor for this type
        //-------------------------------------------------------------------------

        auto pCurrentFactory = s_pHead;
        while ( pCurrentFactory != nullptr )
        {
            if ( pCurrentFactory->SupportsTypeID( typeID ) )
            {
                return pCurrentFactory->TryCreateEditorInternal( context, propertyInfo, pTypeInstance, pPropertyInstance );
            }

            StringID const customEditorNameID( propertyInfo.m_metadata.GetValue( PropertyMetadata::CustomEditor ) );
            if ( customEditorNameID.IsValid() && pCurrentFactory->SupportsCustomEditorID( customEditorNameID ) )
            {
                return pCurrentFactory->TryCreateEditorInternal( context, propertyInfo, pTypeInstance, pPropertyInstance );
            }

            pCurrentFactory = pCurrentFactory->GetNextItem();
        }

        // Create core type editors
        //-------------------------------------------------------------------------

        if ( propertyInfo.IsEnumProperty() )
        {
            return EE::New<EnumEditor>( context, propertyInfo, pTypeInstance, pPropertyInstance );
        }

        if ( TypeSystem::IsCoreType( typeID ) )
        {
            CoreTypeID const coreType = GetCoreType( propertyInfo.m_typeID );
            switch ( coreType )
            {
                case CoreTypeID::Bool:
                {
                    return EE::New<BoolEditor>( context, propertyInfo, pTypeInstance, pPropertyInstance );
                }
                break;

                case CoreTypeID::Int8:
                {
                    return EE::New<IntegerEditor<int8_t, ImGuiDataType_S8>>( context, propertyInfo, pTypeInstance, pPropertyInstance );
                }
                break;

                case CoreTypeID::Int16:
                {
                    return EE::New<IntegerEditor<int16_t, ImGuiDataType_S16>>( context, propertyInfo, pTypeInstance, pPropertyInstance );
                }
                break;

                case CoreTypeID::Int32:
                {
                    return EE::New<IntegerEditor<int32_t, ImGuiDataType_S32>>( context, propertyInfo, pTypeInstance, pPropertyInstance );
                }
                break;

                case CoreTypeID::Int64:
                {
                    return EE::New<IntegerEditor<int64_t, ImGuiDataType_S64>>( context, propertyInfo, pTypeInstance, pPropertyInstance );
                }
                break;

                case CoreTypeID::Uint8:
                {
                    return EE::New<IntegerEditor<uint8_t, ImGuiDataType_U8>>( context, propertyInfo, pTypeInstance, pPropertyInstance );
                }
                break;

                case CoreTypeID::Uint16:
                {
                    return EE::New<IntegerEditor<uint16_t, ImGuiDataType_U16>>( context, propertyInfo, pTypeInstance, pPropertyInstance );
                }
                break;

                case CoreTypeID::Uint32:
                {
                    return EE::New<IntegerEditor<uint32_t, ImGuiDataType_U32>>( context, propertyInfo, pTypeInstance, pPropertyInstance );
                }
                break;

                case CoreTypeID::Uint64:
                {
                    return EE::New<IntegerEditor<uint64_t, ImGuiDataType_U64>>( context, propertyInfo, pTypeInstance, pPropertyInstance );
                }
                break;

                case CoreTypeID::Float:
                {
                    return EE::New<FloatEditor>( context, propertyInfo, pTypeInstance, pPropertyInstance );
                }
                break;

                case CoreTypeID::Double:
                {
                    return EE::New<DoubleEditor>( context, propertyInfo, pTypeInstance, pPropertyInstance );
                }
                break;

                case CoreTypeID::Color:
                {
                    return EE::New<ColorEditor>( context, propertyInfo, pTypeInstance, pPropertyInstance );
                }
                break;

                case CoreTypeID::EulerAngles:
                case CoreTypeID::Quaternion:
                {
                    return EE::New<RotationEditor>( context, propertyInfo, pTypeInstance, pPropertyInstance );
                }
                break;

                case CoreTypeID::UUID:
                {
                    return EE::New<UUIDEditor>( context, propertyInfo, pTypeInstance, pPropertyInstance );
                }
                break;

                case CoreTypeID::Microseconds:
                case CoreTypeID::Milliseconds:
                case CoreTypeID::Seconds:
                {
                    return EE::New<TimeEditor>( context, propertyInfo, pTypeInstance, pPropertyInstance );
                }
                break;

                case CoreTypeID::Degrees:
                case CoreTypeID::Radians:
                {
                    return EE::New<AngleEditor>( context, propertyInfo, pTypeInstance, pPropertyInstance );
                }
                break;

                case CoreTypeID::Matrix:
                case CoreTypeID::Transform:
                {
                    return EE::New<TransformEditor>( context, propertyInfo, pTypeInstance, pPropertyInstance );
                }
                break;

                case CoreTypeID::DataPath:
                {
                    return EE::New<DataPathEditor>( context, propertyInfo, pTypeInstance, pPropertyInstance );
                }
                break;

                case CoreTypeID::TDataFilePath:
                {
                    return EE::New<DataFilePathEditor>( context, propertyInfo, pTypeInstance, pPropertyInstance );
                }
                break;

                case CoreTypeID::ResourceID:
                case CoreTypeID::ResourcePtr:
                case CoreTypeID::TResourcePtr:
                {
                    return EE::New<ResourceIDEditor>( context, propertyInfo, pTypeInstance, pPropertyInstance );
                }
                break;

                case CoreTypeID::ResourceTypeID:
                {
                    return EE::New<ResourceTypeIDEditor>( context, propertyInfo, pTypeInstance, pPropertyInstance );
                }
                break;

                case CoreTypeID::TypeInstance:
                case CoreTypeID::TTypeInstance:
                {
                    return EE::New<TypeInstanceEditor>( context, propertyInfo, pTypeInstance, pPropertyInstance );
                }
                break;

                case CoreTypeID::BitFlags:
                case CoreTypeID::TBitFlags:
                {
                    return EE::New<BitflagsEditor>( context, propertyInfo, pTypeInstance, pPropertyInstance );
                }
                break;

                case CoreTypeID::StringID:
                {
                    return EE::New<StringIDEditor>( context, propertyInfo, pTypeInstance, pPropertyInstance );
                }
                break;

                case CoreTypeID::Tag2:
                {
                    return EE::New<Tag2Editor>( context, propertyInfo, pTypeInstance, pPropertyInstance );
                }
                break;

                case CoreTypeID::Tag4:
                {
                    return EE::New<Tag4Editor>( context, propertyInfo, pTypeInstance, pPropertyInstance );
                }
                break;

                case CoreTypeID::String:
                {
                    return EE::New<StringEditor>( context, propertyInfo, pTypeInstance, pPropertyInstance );
                }
                break;

                case CoreTypeID::Float2:
                {
                    return EE::New<FloatNEditor<Float2>>( context, propertyInfo, pTypeInstance, pPropertyInstance );
                }
                break;

                case CoreTypeID::Float3:
                {
                    return EE::New<FloatNEditor<Float3>>( context, propertyInfo, pTypeInstance, pPropertyInstance );
                }
                break;

                case CoreTypeID::Float4:
                {
                    return EE::New<FloatNEditor<Float4>>( context, propertyInfo, pTypeInstance, pPropertyInstance );
                }
                break;

                case CoreTypeID::Vector:
                {
                    return EE::New<VectorEditor>( context, propertyInfo, pTypeInstance, pPropertyInstance );
                }
                break;

                case CoreTypeID::Percentage:
                {
                    return EE::New<PercentageEditor>( context, propertyInfo, pTypeInstance, pPropertyInstance );
                }
                break;

                case CoreTypeID::IntRange:
                {
                    return EE::New<IntRangeEditor>( context, propertyInfo, pTypeInstance, pPropertyInstance );
                }
                break;

                case CoreTypeID::FloatRange:
                {
                    return EE::New<FloatRangeEditor>( context, propertyInfo, pTypeInstance, pPropertyInstance );
                }
                break;

                case CoreTypeID::FloatCurve:
                {
                    return EE::New<FloatCurveEditor>( context, propertyInfo, pTypeInstance, pPropertyInstance );
                }
                break;

                default:
                {
                    return nullptr;
                }
                break;
            }
        }

        return nullptr;
    }
}