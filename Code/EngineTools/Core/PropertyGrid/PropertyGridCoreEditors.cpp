#include "PropertyGridCoreEditors.h"
#include "PropertyGridEditor.h"
#include "EngineTools/Core/ToolsContext.h"
#include "EngineTools/Resource/ResourcePicker.h"
#include "EngineTools/Resource/ResourceDatabase.h"
#include "EngineTools/Core/Widgets/CurveEditor.h"
#include "System/Imgui/ImguiX.h"
#include "System/Resource/ResourcePtr.h"
#include "System/TypeSystem/PropertyInfo.h"
#include "System/TypeSystem/EnumInfo.h"
#include "System/TypeSystem/TypeRegistry.h"
#include "System/Types/String.h"
#include "System/Types/Tag.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem
{
    constexpr static float const g_iconButtonWidth = 26;

    //-------------------------------------------------------------------------
    // Core Editors
    //-------------------------------------------------------------------------

    class EnumEditor final : public PropertyEditor
    {
    public:

        EnumEditor( ToolsContext const* pToolsContext, PropertyInfo const& propertyInfo, uint8_t* m_pPropertyInstance )
            : PropertyEditor( pToolsContext, propertyInfo, m_pPropertyInstance )
            , m_pEnumInfo( pToolsContext->m_pTypeRegistry->GetEnumInfo( m_propertyInfo.m_typeID ) )
        {
            EE_ASSERT( m_pEnumInfo != nullptr );
            EnumEditor::ResetWorkingCopy();
        }

        virtual bool InternalUpdateAndDraw() override
        {
            ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x );
            if ( ImGui::BeginCombo( "##enumCombo", m_pEnumInfo->GetConstantLabel( m_value_imgui ).c_str() ) )
            {
                for ( auto const& enumValue : m_pEnumInfo->m_constants )
                {
                    bool const isSelected = ( enumValue.second.m_value == m_value_imgui );
                    if ( ImGui::Selectable( enumValue.first.c_str(), isSelected ) )
                    {
                        m_value_imgui = enumValue.second.m_value;
                    }

                    String const* pDescription = m_pEnumInfo->TryGetConstantDescription( enumValue.first );
                    if ( pDescription != nullptr && !pDescription->empty() )
                    {
                        ImGuiX::ItemTooltip( pDescription->c_str() );
                    }

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if ( isSelected )
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }

                ImGui::EndCombo();
            }

            return  m_value_cached != m_value_imgui;
        }

        virtual void UpdatePropertyValue() override
        {
            m_value_cached = m_value_imgui;

            switch ( m_pEnumInfo->m_underlyingType )
            {
                case CoreTypeID::Uint8:
                {
                    *reinterpret_cast<uint8_t*>( m_pPropertyInstance ) = (uint8_t) m_value_cached;
                }
                break;

                case CoreTypeID::Int8:
                {
                    *reinterpret_cast<int8_t*>( m_pPropertyInstance ) = (int8_t) m_value_cached;
                }
                break;

                case CoreTypeID::Uint16:
                {
                    *reinterpret_cast<uint16_t*>( m_pPropertyInstance ) = (uint16_t) m_value_cached;
                }
                break;

                case CoreTypeID::Int16:
                {
                    *reinterpret_cast<int16_t*>( m_pPropertyInstance ) = (int16_t) m_value_cached;
                }
                break;

                case CoreTypeID::Uint32:
                {
                    *reinterpret_cast<uint32_t*>( m_pPropertyInstance ) = (uint32_t) m_value_cached;
                }
                break;

                case CoreTypeID::Int32:
                {
                    *reinterpret_cast<int32_t*>( m_pPropertyInstance ) = (int32_t) m_value_cached;
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
                    m_value_cached = m_value_imgui = ( int64_t ) *reinterpret_cast<uint8_t const*>( m_pPropertyInstance );
                }
                break;

                case CoreTypeID::Int8:
                {
                    m_value_cached = m_value_imgui = ( int64_t ) *reinterpret_cast<int8_t const*>( m_pPropertyInstance );
                }
                break;

                case CoreTypeID::Uint16:
                {
                    m_value_cached = m_value_imgui = ( int64_t ) *reinterpret_cast<uint16_t const*>( m_pPropertyInstance );
                }
                break;

                case CoreTypeID::Int16:
                {
                    m_value_cached = m_value_imgui = ( int64_t ) *reinterpret_cast<int16_t const*>( m_pPropertyInstance );
                }
                break;

                case CoreTypeID::Uint32:
                {
                    m_value_cached = m_value_imgui = ( int64_t ) *reinterpret_cast<uint32_t const*>( m_pPropertyInstance );
                }
                break;

                case CoreTypeID::Int32:
                {
                    m_value_cached = m_value_imgui = ( int64_t ) *reinterpret_cast<int32_t const*>( m_pPropertyInstance );
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

        EnumInfo const*     m_pEnumInfo = nullptr;
        int64_t               m_value_cached;
        int64_t               m_value_imgui;
    };

    //-------------------------------------------------------------------------

    class BoolEditor final : public PropertyEditor
    {
    public:

        BoolEditor( ToolsContext const* pToolsContext, PropertyInfo const& propertyInfo, uint8_t* m_pPropertyInstance )
            : PropertyEditor( pToolsContext,  propertyInfo, m_pPropertyInstance )
        {
            BoolEditor::ResetWorkingCopy();
        }

        virtual bool InternalUpdateAndDraw() override
        {
            ImGui::Checkbox( "##be", &m_value );
            return ImGui::IsItemDeactivatedAfterEdit();
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

    //-------------------------------------------------------------------------

    template<typename T>
    class ScalarEditor final : public PropertyEditor
    {
    public:

        ScalarEditor( ToolsContext const* pToolsContext, PropertyInfo const& propertyInfo, uint8_t* m_pPropertyInstance )
            : PropertyEditor( pToolsContext, propertyInfo, m_pPropertyInstance )
        {
            ScalarEditor::ResetWorkingCopy();
        }

        virtual bool InternalUpdateAndDraw() override
        {
            ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x );

            switch ( m_coreType )
            {
                case CoreTypeID::Int8:
                {
                    ImGui::InputScalar( "##scaed", ImGuiDataType_S8, &m_value_imgui );
                }
                break;

                case CoreTypeID::Int16:
                {
                    ImGui::InputScalar( "##scaed", ImGuiDataType_S16, &m_value_imgui );
                }
                break;

                case CoreTypeID::Int32:
                {
                    ImGui::InputScalar( "##scaed", ImGuiDataType_S32, &m_value_imgui );
                }
                break;

                case CoreTypeID::Int64:
                {
                    ImGui::InputScalar( "##scaed", ImGuiDataType_S64, &m_value_imgui );
                }
                break;

                case CoreTypeID::Uint8:
                {
                    ImGui::InputScalar( "##scaed", ImGuiDataType_U8, &m_value_imgui );
                }
                break;

                case CoreTypeID::Uint16:
                {
                    ImGui::InputScalar( "##scaed", ImGuiDataType_U16, &m_value_imgui );
                }
                break;

                case CoreTypeID::Uint32:
                {
                    ImGui::InputScalar( "##scaed", ImGuiDataType_U32, &m_value_imgui );
                }
                break;

                case CoreTypeID::Uint64:
                {
                    ImGui::InputScalar( "##scaed", ImGuiDataType_U64, &m_value_imgui );
                }
                break;

                case CoreTypeID::Float:
                {
                    ImGui::InputFloat( "##scaed", (float*) &m_value_imgui );
                }
                break;

                case CoreTypeID::Double:
                {
                    ImGui::InputDouble( "##scaed", (double*) &m_value_imgui );
                }
                break;

                default:
                {
                    EE_UNREACHABLE_CODE();
                }
                break;
            }

            return ImGui::IsItemDeactivatedAfterEdit();
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

    template<typename T>
    class VectorEditor final : public PropertyEditor
    {
    public:

        VectorEditor( ToolsContext const* pToolsContext, PropertyInfo const& propertyInfo, uint8_t* m_pPropertyInstance )
            : PropertyEditor( pToolsContext, propertyInfo, m_pPropertyInstance )
        {
            VectorEditor::ResetWorkingCopy();
        }

        virtual bool InternalUpdateAndDraw() override
        {
            bool valueChanged = false;

            switch ( m_coreType )
            {
                case CoreTypeID::Float2:
                {
                    if ( ImGuiX::InputFloat2( "F2Ed", (Float2&) m_value_imgui ) )
                    {
                        valueChanged = true;
                    }
                }
                break;

                case CoreTypeID::Float3:
                {
                    if ( ImGuiX::InputFloat3( "F3Ed", (Float3&) m_value_imgui ) )
                    {
                        valueChanged = true;
                    }
                }
                break;

                case CoreTypeID::Float4:
                {
                    if ( ImGuiX::InputFloat4( "F4Ed", (Float4&) m_value_imgui ) )
                    {
                        valueChanged = true;
                    }
                }
                break;

                case CoreTypeID::Vector:
                {
                    if ( ImGuiX::InputFloat4( "VectorEd", (Vector&) m_value_imgui ) )
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

            return valueChanged;
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

    class ColorEditor final : public PropertyEditor
    {
    public:

        ColorEditor( ToolsContext const* pToolsContext, PropertyInfo const& propertyInfo, uint8_t* m_pPropertyInstance )
            : PropertyEditor( pToolsContext, propertyInfo, m_pPropertyInstance )
        {
            ColorEditor::ResetWorkingCopy();
        }

        virtual bool InternalUpdateAndDraw() override
        {
            ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x );
            ImGui::ColorEdit4( "##ce", &m_value_imgui.x );
            return ImGui::IsItemDeactivatedAfterEdit();
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

        RotationEditor( ToolsContext const* pToolsContext, PropertyInfo const& propertyInfo, uint8_t* m_pPropertyInstance )
            : PropertyEditor( pToolsContext, propertyInfo, m_pPropertyInstance )
        {
            RotationEditor::ResetWorkingCopy();
            EE_ASSERT( m_coreType == CoreTypeID::EulerAngles || m_coreType == CoreTypeID::Quaternion );
        }

        virtual bool InternalUpdateAndDraw() override
        {
            return ImGuiX::InputFloat3( "Rotation", m_anglesInDegrees_imgui );
        }

        virtual void UpdatePropertyValue() override
        {
            m_anglesInDegrees_cached = m_anglesInDegrees_imgui;
            EulerAngles const newRotation( Degrees( m_anglesInDegrees_cached.m_x ).GetClamped180(), Degrees( m_anglesInDegrees_cached.m_y ).GetClamped180(), Degrees( m_anglesInDegrees_cached.m_z ).GetClamped180() );

            if ( m_coreType == CoreTypeID::EulerAngles )
            {
                *reinterpret_cast<EulerAngles*>( m_pPropertyInstance ) = newRotation;
            }
            else if( m_coreType == CoreTypeID::Quaternion )
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
            else if( m_coreType == CoreTypeID::Quaternion )
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

        AngleEditor( ToolsContext const* pToolsContext, PropertyInfo const& propertyInfo, uint8_t* m_pPropertyInstance )
            : PropertyEditor( pToolsContext, propertyInfo, m_pPropertyInstance )
        {
            AngleEditor::ResetWorkingCopy();
            EE_ASSERT( m_coreType == CoreTypeID::Degrees || m_coreType == CoreTypeID::Radians );
        }

        virtual bool InternalUpdateAndDraw() override
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

            return valueChanged;
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

        UUIDEditor( ToolsContext const* pToolsContext, PropertyInfo const& propertyInfo, uint8_t* m_pPropertyInstance )
            : PropertyEditor( pToolsContext, propertyInfo, m_pPropertyInstance )
        {
            UUIDEditor::ResetWorkingCopy();
        }

        virtual bool InternalUpdateAndDraw() override
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

            return valueChanged;
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

        TimeEditor( ToolsContext const* pToolsContext, PropertyInfo const& propertyInfo, uint8_t* m_pPropertyInstance )
            : PropertyEditor( pToolsContext, propertyInfo, m_pPropertyInstance )
        {
            TimeEditor::ResetWorkingCopy();
            EE_ASSERT( m_coreType == CoreTypeID::Microseconds || m_coreType == CoreTypeID::Milliseconds || m_coreType == CoreTypeID::Seconds );
        }

        virtual bool InternalUpdateAndDraw() override
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
            
            return ImGui::IsItemDeactivatedAfterEdit();
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

        PercentageEditor( ToolsContext const* pToolsContext, PropertyInfo const& propertyInfo, uint8_t* m_pPropertyInstance )
            : PropertyEditor( pToolsContext, propertyInfo, m_pPropertyInstance )
        {
            PercentageEditor::ResetWorkingCopy();
        }

        virtual bool InternalUpdateAndDraw() override
        {
            float const textAreaWidth = ImGui::GetContentRegionAvail().x - g_iconButtonWidth - ImGui::GetStyle().ItemSpacing.x;

            ImGui::SetNextItemWidth( textAreaWidth );
            ImGui::InputFloat( "##pe", &m_value_imgui, 0, 0, "%.2f%%" );
            bool valueChanged = ImGui::IsItemDeactivatedAfterEdit();

            //-------------------------------------------------------------------------

            ImGui::SameLine( 0, ImGui::GetStyle().ItemSpacing.x );
            if ( ImGui::Button( EE_ICON_PERCENT_BOX_OUTLINE"##ClampPercentage", ImVec2( g_iconButtonWidth, 0 ) ) )
            {
                m_value_imgui = ( Percentage( m_value_imgui / 100 ).GetClamped( true ) ).ToFloat() * 100;
                valueChanged = true;
            }
            ImGuiX::ItemTooltip( "Clamp [-100 : 100]" );

            return valueChanged;
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

        TransformEditor( ToolsContext const* pToolsContext, PropertyInfo const& propertyInfo, uint8_t* m_pPropertyInstance )
            : PropertyEditor( pToolsContext, propertyInfo, m_pPropertyInstance )
        {
            TransformEditor::ResetWorkingCopy();
            EE_ASSERT( m_coreType == CoreTypeID::Transform || m_coreType == CoreTypeID::Matrix );
        }

        virtual bool InternalUpdateAndDraw() override
        {
            bool transformUpdated = false;

            ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 0, 2 ) );
            if ( ImGui::BeginTable( "Transform", 2, ImGuiTableFlags_None ) )
            {
                ImGui::TableSetupColumn( "Header", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 24 );
                ImGui::TableSetupColumn( "Values", ImGuiTableColumnFlags_NoHide );

                ImGui::TableNextRow();
                {
                    ImGui::TableNextColumn();
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text( "Rot" );

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
                    ImGui::Text( "Pos" );

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
                    ImGui::Text( "Scl" );

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

            return transformUpdated;
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
                float s;
                matrix->Decompose( q, t, s );

                m_rotation_cached = m_rotation_imgui = q.ToEulerAngles().GetAsDegrees();
                m_translation_cached = m_translation_imgui = t.ToFloat3();
                m_scale_cached = m_scale_imgui = s;
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
                matrix->Decompose( actualQ, actualTranslation, actualScale );
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

    class ResourcePathEditor final : public PropertyEditor
    {
    public:

        ResourcePathEditor( ToolsContext const* pToolsContext, Resource::ResourcePicker& resourcePicker, PropertyInfo const& propertyInfo, uint8_t* m_pPropertyInstance )
            : PropertyEditor( pToolsContext, propertyInfo, m_pPropertyInstance )
            , m_resourceFilePicker( resourcePicker )
        {
            ResourcePathEditor::ResetWorkingCopy();
            EE_ASSERT( m_coreType == CoreTypeID::ResourcePath || m_coreType == CoreTypeID::ResourceID || m_coreType == CoreTypeID::ResourcePtr || m_coreType == CoreTypeID::TResourcePtr );

            //-------------------------------------------------------------------------

            if ( m_coreType == CoreTypeID::TResourcePtr )
            {
                m_resourceTypeID = m_pToolsContext->m_pTypeRegistry->GetResourceInfoForType( propertyInfo.m_templateArgumentTypeID )->m_resourceTypeID;
            }
        }

        virtual bool InternalUpdateAndDraw() override
        {
            bool valueChanged = false;
            ResourcePath newPath;

            if ( m_coreType == CoreTypeID::ResourcePath )
            {
                if ( m_resourceFilePicker.DrawPicker( m_value_imgui, newPath ) )
                {
                    m_value_imgui = newPath;
                    valueChanged = true;
                }
            }
            else if ( m_coreType == CoreTypeID::ResourceID || m_coreType == CoreTypeID::ResourcePtr || m_coreType == CoreTypeID::TResourcePtr )
            {
                if ( m_resourceFilePicker.DrawPicker( m_tempResourceID, newPath, m_resourceTypeID ) )
                {
                    m_tempResourceID = newPath.IsValid() ? newPath : ResourceID();
                    m_value_imgui = m_tempResourceID.GetResourcePath();
                    valueChanged = true;
                }
            }

            //-------------------------------------------------------------------------

            return valueChanged;
        }

        virtual void UpdatePropertyValue() override
        {
            m_value_cached = m_value_imgui;

            if ( m_coreType == CoreTypeID::ResourcePath )
            {
                *reinterpret_cast<ResourcePath*>( m_pPropertyInstance ) = m_value_cached;
            }
            else if ( m_coreType == CoreTypeID::ResourceID )
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
            if ( m_coreType == CoreTypeID::ResourcePath )
            {
                m_value_cached = m_value_imgui = *reinterpret_cast<ResourcePath*>( m_pPropertyInstance );
                m_tempResourceID.Clear();
            }
            else if ( m_coreType == CoreTypeID::ResourceID )
            {
                m_tempResourceID = *reinterpret_cast<ResourceID*>( m_pPropertyInstance );
                m_value_cached = m_value_imgui = m_tempResourceID.GetResourcePath();
            }
            else if ( m_coreType == CoreTypeID::ResourcePtr || m_coreType == CoreTypeID::TResourcePtr )
            {
                Resource::ResourcePtr* pResourcePtr = reinterpret_cast<Resource::ResourcePtr*>( m_pPropertyInstance );
                m_value_cached = m_value_imgui = pResourcePtr->GetResourcePath();
                if ( m_value_imgui.IsValid() )
                {
                    m_tempResourceID = m_value_imgui;
                }
                else
                {
                    m_tempResourceID.Clear();
                }
            }
        }

        virtual void HandleExternalUpdate() override
        {
            ResourcePath const* pActualPath = nullptr;

            if ( m_coreType == CoreTypeID::ResourcePath )
            {
                pActualPath = reinterpret_cast<ResourcePath*>( m_pPropertyInstance );
            }
            else if ( m_coreType == CoreTypeID::ResourceID )
            {
                ResourceID* pResourceID = reinterpret_cast<ResourceID*>( m_pPropertyInstance );
                pActualPath = &pResourceID->GetResourcePath();
            }
            else if ( m_coreType == CoreTypeID::ResourcePtr || m_coreType == CoreTypeID::TResourcePtr )
            {
                Resource::ResourcePtr* pResourcePtr = reinterpret_cast<Resource::ResourcePtr*>( m_pPropertyInstance );
                pActualPath = &pResourcePtr->GetResourcePath();
            }

            if ( *pActualPath != m_value_cached )
            {
                m_value_cached = m_value_imgui = *pActualPath;

                if ( m_coreType == CoreTypeID::ResourceID || m_coreType == CoreTypeID::ResourcePtr || m_coreType == CoreTypeID::TResourcePtr )
                {
                    if ( m_value_imgui.IsValid() )
                    {
                        m_tempResourceID = m_value_imgui;
                    }
                    else
                    {
                        m_tempResourceID.Clear();
                    }
                }
            }
        }

    private:

        Resource::ResourcePicker&               m_resourceFilePicker;
        ResourceTypeID                          m_resourceTypeID;
        ResourceID                              m_tempResourceID;
        ResourcePath                            m_value_imgui;
        ResourcePath                            m_value_cached;
    };

    //-------------------------------------------------------------------------

    class ResourceTypeIDEditor final : public PropertyEditor
    {
    public:

        ResourceTypeIDEditor( ToolsContext const* pToolsContext, PropertyInfo const& propertyInfo, uint8_t* m_pPropertyInstance )
            : PropertyEditor( pToolsContext, propertyInfo, m_pPropertyInstance )
        {
            ResourceTypeIDEditor::ResetWorkingCopy();
        }

        virtual bool InternalUpdateAndDraw() override
        {
            bool valueChanged = false;

            //-------------------------------------------------------------------------

            ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x );
            if ( ImGui::BeginCombo( "##resTypeID", m_resourceTypeFriendlyName.c_str()) )
            {
                auto AddComboItem = [this, &valueChanged] ( ResourceInfo const& resourceInfo )
                {
                    bool const isSelected = ( m_value_imgui.m_ID == resourceInfo.m_resourceTypeID );
                    if ( ImGui::Selectable( resourceInfo.m_friendlyName.empty() ? "None" :  resourceInfo.m_friendlyName.c_str(), isSelected) )
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

                auto const& registeredResourceTypes = m_pToolsContext->m_pTypeRegistry->GetRegisteredResourceTypes();
                for ( auto& pair : registeredResourceTypes )
                {
                    AddComboItem( pair.second );
                }

                ImGui::EndCombo();
            }

            //-------------------------------------------------------------------------

            return valueChanged;
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
                m_resourceTypeFriendlyName = m_pToolsContext->m_pTypeRegistry->GetResourceInfoForResourceType( m_value_cached )->m_friendlyName;
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

    class BitflagsEditor final : public PropertyEditor
    {
    public:

        BitflagsEditor( ToolsContext const* pToolsContext, PropertyInfo const& propertyInfo, uint8_t* m_pPropertyInstance )
            : PropertyEditor( pToolsContext, propertyInfo, m_pPropertyInstance )
        {
            BitflagsEditor::ResetWorkingCopy();
            EE_ASSERT( m_coreType == CoreTypeID::BitFlags || m_coreType == CoreTypeID::TBitFlags );
        }

        virtual bool InternalUpdateAndDraw() override
        {
            bool valueChanged = false;

            if ( ImGui::BeginTable( "FlagsTable", 9, ImGuiTableFlags_SizingFixedFit ) )
            {
                if ( m_coreType == CoreTypeID::BitFlags )
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
                            if ( ImGui::Checkbox( "##flag", &m_values_imgui[flagIdx]) )
                            {
                                valueChanged = true;
                            }
                            ImGui::PopID();
                        }

                        ImGui::TableNextColumn();
                        ImGui::Text( rowLabels[i] );
                    }
                }
                else if ( m_coreType == CoreTypeID::TBitFlags )
                {
                    // Get enum type for specific flags
                    TypeID const enumTypeID = m_propertyInfo.m_templateArgumentTypeID;
                    EnumInfo const* pEnumInfo = m_pToolsContext->m_pTypeRegistry->GetEnumInfo( enumTypeID );
                    EE_ASSERT( pEnumInfo != nullptr );

                    //-------------------------------------------------------------------------

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

                        int64_t const flagValue = constant.second.m_value;
                        EE_ASSERT( flagValue >= 0 && flagValue <= 31 );
                        if ( ImGui::Checkbox( constant.first.c_str(), &m_values_imgui[flagValue] ) )
                        {
                            valueChanged = true;
                        }
                    }
                }

                ImGui::EndTable();
            }

            //-------------------------------------------------------------------------

            return valueChanged;
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
        uint32_t      m_cachedFlags = 0;
    };

    //-------------------------------------------------------------------------

    class StringEditor final : public PropertyEditor
    {
    public:

        StringEditor( ToolsContext const* pToolsContext, PropertyInfo const& propertyInfo, uint8_t* m_pPropertyInstance )
            : PropertyEditor( pToolsContext, propertyInfo, m_pPropertyInstance )
        {
            StringEditor::ResetWorkingCopy();
        }

        virtual bool InternalUpdateAndDraw() override
        {
            ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x );
            ImGui::InputText( "##stringEd", m_buffer_imgui, 256);
            return ImGui::IsItemDeactivatedAfterEdit();
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

        StringIDEditor( ToolsContext const* pToolsContext, PropertyInfo const& propertyInfo, uint8_t* m_pPropertyInstance )
            : PropertyEditor( pToolsContext, propertyInfo, m_pPropertyInstance )
        {
            StringIDEditor::ResetWorkingCopy();
        }

        virtual bool InternalUpdateAndDraw() override
        {
            ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x - 60 );
            ImGui::InputText( "##StringInput", m_buffer_imgui, s_bufferSize );
            
            bool const itemEdited = ImGui::IsItemDeactivatedAfterEdit();
            if ( itemEdited )
            {
                StringUtils::StripTrailingWhitespace( m_buffer_imgui );
            }

            ImGui::SameLine();

            ImGui::SetNextItemWidth( -1 );
            {
                ImGuiX::ScopedFont const sf( ImGuiX::Font::Tiny );
                ImGui::AlignTextToFramePadding();
                ImGui::TextColored( Colors::LightGreen.ToFloat4(), m_IDString.data() );
            }

            return itemEdited;
        }

        virtual void UpdatePropertyValue() override
        {
            auto pValue = reinterpret_cast<StringID*>( m_pPropertyInstance );

            if ( strlen( m_buffer_imgui ) > 0 )
            {
                m_buffer_cached = m_buffer_imgui;
                *pValue = StringID( m_buffer_imgui );
                m_IDString.sprintf( "%u", pValue->GetID() );
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
                m_IDString.sprintf( "%u", pValue->GetID() );
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

    class TagEditor final : public PropertyEditor
    {
    public:

        TagEditor( ToolsContext const* pToolsContext, PropertyInfo const& propertyInfo, uint8_t* m_pPropertyInstance )
            : PropertyEditor( pToolsContext, propertyInfo, m_pPropertyInstance )
        {
            TagEditor::ResetWorkingCopy();
        }

        virtual bool InternalUpdateAndDraw() override
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

            return valueUpdated;
        }

        virtual void UpdatePropertyValue() override
        {
            m_value_cached = m_value_imgui;
            *reinterpret_cast<Tag*>( m_pPropertyInstance ) = m_value_cached;
        }

        virtual void ResetWorkingCopy() override
        {
            m_value_cached = m_value_imgui = *reinterpret_cast<Tag*>( m_pPropertyInstance );
            UpdateBuffers();
        }

        virtual void HandleExternalUpdate() override
        {
            Tag& actualValue = *reinterpret_cast<Tag*>( m_pPropertyInstance );
            if ( actualValue != m_value_cached )
            {
                m_value_cached = m_value_imgui = actualValue;
                UpdateBuffers();
            }
        }

    private:

        void UpdateBuffers()
        {
            int32_t const tagDepth = m_value_imgui.GetDepth();
            for ( auto i = 0; i < 4; i++ )
            {
                if ( i < tagDepth )
                {
                    Printf( m_buffers[i], 255, "%s", m_value_imgui.GetValueAtLevel( i ).c_str() );
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

            m_value_imgui = Tag( c[0], c[1], c[2], c[3] );
            UpdateBuffers();
        }

    private:

        char    m_buffers[4][255] = { {0}, {0}, {0}, {0} };
        Tag     m_value_imgui;
        Tag     m_value_cached;
    };

    //-------------------------------------------------------------------------

    class IntRangeEditor final : public PropertyEditor
    {
    public:

        IntRangeEditor( ToolsContext const* pToolsContext, PropertyInfo const& propertyInfo, uint8_t* m_pPropertyInstance )
            : PropertyEditor( pToolsContext, propertyInfo, m_pPropertyInstance )
        {
            IntRangeEditor::ResetWorkingCopy();
        }

        virtual bool InternalUpdateAndDraw() override
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

            return valueUpdated;
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

        FloatRangeEditor( ToolsContext const* pToolsContext, PropertyInfo const& propertyInfo, uint8_t* m_pPropertyInstance )
            : PropertyEditor( pToolsContext, propertyInfo, m_pPropertyInstance )
        {
            FloatRangeEditor::ResetWorkingCopy();
        }

        virtual bool InternalUpdateAndDraw() override
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
            ImGui::SameLine( 0, ImGui::GetStyle().ItemSpacing.x );
            ImGui::SetCursorPosY( ImGui::GetCursorPosY() - verticalOffset );

            //-------------------------------------------------------------------------

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

            return valueUpdated;
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

        FloatCurveEditor( ToolsContext const* pToolsContext, PropertyInfo const& propertyInfo, uint8_t* m_pPropertyInstance )
            : PropertyEditor( pToolsContext, propertyInfo, m_pPropertyInstance )
            , m_editor( m_value_imgui )
        {
            FloatCurveEditor::ResetWorkingCopy();
        }

        virtual bool InternalUpdateAndDraw() override
        {
            bool valueChanged = false;
            if ( ImGui::BeginChild( "##Preview", ImVec2( ImGui::GetContentRegionAvail().x, 140 ) ) )
            {
                valueChanged = m_editor.UpdateAndDraw( true );
            }
            ImGui::EndChild();
            return valueChanged;
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
    // Factory Method
    //-------------------------------------------------------------------------

    PropertyEditor* CreatePropertyEditor( ToolsContext const* pToolsContext, Resource::ResourcePicker& resourcePicker, PropertyInfo const& propertyInfo, uint8_t* m_pPropertyInstance )
    {
        if ( propertyInfo.IsEnumProperty() )
        {
            return EE::New<EnumEditor>( pToolsContext, propertyInfo, m_pPropertyInstance );
        }
        else
        {
            CoreTypeID const coreType = GetCoreType( propertyInfo.m_typeID );
            switch ( coreType )
            {
                case CoreTypeID::Bool:
                {
                    return EE::New<BoolEditor>( pToolsContext, propertyInfo, m_pPropertyInstance );
                }
                break;

                case CoreTypeID::Int8:
                {
                    return EE::New<ScalarEditor<int8_t>>( pToolsContext, propertyInfo, m_pPropertyInstance );
                }
                break;

                case CoreTypeID::Int16:
                {
                    return EE::New<ScalarEditor<int16_t>>( pToolsContext, propertyInfo, m_pPropertyInstance );
                }
                break;

                case CoreTypeID::Int32:
                {
                    return EE::New<ScalarEditor<int32_t>>( pToolsContext, propertyInfo, m_pPropertyInstance );
                }
                break;

                case CoreTypeID::Int64:
                {
                    return EE::New<ScalarEditor<int64_t>>( pToolsContext, propertyInfo, m_pPropertyInstance );
                }
                break;
                
                case CoreTypeID::Uint8:
                {
                    return EE::New<ScalarEditor<uint8_t>>( pToolsContext, propertyInfo, m_pPropertyInstance );
                }
                break;

                case CoreTypeID::Uint16:
                {
                    return EE::New<ScalarEditor<uint16_t>>( pToolsContext, propertyInfo, m_pPropertyInstance );
                }
                break;

                case CoreTypeID::Uint32:
                {
                    return EE::New<ScalarEditor<uint32_t>>( pToolsContext, propertyInfo, m_pPropertyInstance );
                }
                break;

                case CoreTypeID::Uint64:
                {
                    return EE::New<ScalarEditor<uint64_t>>( pToolsContext, propertyInfo, m_pPropertyInstance );
                }
                break;
                
                case CoreTypeID::Float:
                {
                    return EE::New<ScalarEditor<float>>( pToolsContext, propertyInfo, m_pPropertyInstance );
                }
                break;

                case CoreTypeID::Double:
                {
                    return EE::New<ScalarEditor<double>>( pToolsContext, propertyInfo, m_pPropertyInstance );
                }
                break;

                case CoreTypeID::Color:
                {
                    return EE::New<ColorEditor>( pToolsContext, propertyInfo, m_pPropertyInstance );
                }
                break;

                case CoreTypeID::EulerAngles:
                case CoreTypeID::Quaternion:
                {
                    return EE::New<RotationEditor>( pToolsContext, propertyInfo, m_pPropertyInstance );
                }
                break;

                case CoreTypeID::UUID:
                {
                    return EE::New<UUIDEditor>( pToolsContext, propertyInfo, m_pPropertyInstance );
                }
                break;

                case CoreTypeID::Microseconds:
                case CoreTypeID::Milliseconds:
                case CoreTypeID::Seconds:
                {
                    return EE::New<TimeEditor>( pToolsContext, propertyInfo, m_pPropertyInstance );
                }
                break;

                case CoreTypeID::Degrees:
                case CoreTypeID::Radians:
                {
                    return EE::New<AngleEditor>( pToolsContext, propertyInfo, m_pPropertyInstance );
                }
                break;

                case CoreTypeID::Matrix:
                case CoreTypeID::Transform:
                {
                    return EE::New<TransformEditor>( pToolsContext, propertyInfo, m_pPropertyInstance );
                }
                break;

                case CoreTypeID::ResourcePath:
                case CoreTypeID::ResourceID:
                case CoreTypeID::ResourcePtr:
                case CoreTypeID::TResourcePtr:
                {
                    return EE::New<ResourcePathEditor>( pToolsContext, resourcePicker, propertyInfo, m_pPropertyInstance );
                }
                break;

                case CoreTypeID::ResourceTypeID:
                {
                    return EE::New<ResourceTypeIDEditor>( pToolsContext, propertyInfo, m_pPropertyInstance );
                }
                break;

                case CoreTypeID::BitFlags:
                case CoreTypeID::TBitFlags:
                {
                    return EE::New<BitflagsEditor>( pToolsContext, propertyInfo, m_pPropertyInstance );
                }
                break;

                case CoreTypeID::StringID:
                {
                    return EE::New<StringIDEditor>( pToolsContext, propertyInfo, m_pPropertyInstance );
                }
                break;

                case CoreTypeID::Tag:
                {
                    return EE::New<TagEditor>( pToolsContext, propertyInfo, m_pPropertyInstance );
                }
                break;

                case CoreTypeID::String:
                {
                    return EE::New<StringEditor>( pToolsContext, propertyInfo, m_pPropertyInstance );
                }
                break;

                case CoreTypeID::Float2:
                {
                    return EE::New<VectorEditor<Float2>>( pToolsContext, propertyInfo, m_pPropertyInstance );
                }
                break;

                case CoreTypeID::Float3:
                {
                    return EE::New<VectorEditor<Float3>>( pToolsContext, propertyInfo, m_pPropertyInstance );
                }
                break;

                case CoreTypeID::Float4:
                {
                    return EE::New<VectorEditor<Float4>>( pToolsContext, propertyInfo, m_pPropertyInstance );
                }
                break;

                case CoreTypeID::Vector:
                {
                    return EE::New<VectorEditor<Vector>>( pToolsContext, propertyInfo, m_pPropertyInstance );
                }
                break;

                case CoreTypeID::Percentage:
                {
                    return EE::New<PercentageEditor>( pToolsContext, propertyInfo, m_pPropertyInstance );
                }
                break;

                case CoreTypeID::IntRange:
                {
                    return EE::New<IntRangeEditor>( pToolsContext, propertyInfo, m_pPropertyInstance );
                }
                break;

                case CoreTypeID::FloatRange:
                {
                    return EE::New<FloatRangeEditor>( pToolsContext, propertyInfo, m_pPropertyInstance );
                }
                break;

                case CoreTypeID::FloatCurve:
                {
                    return EE::New<FloatCurveEditor>( pToolsContext, propertyInfo, m_pPropertyInstance );
                }
                break;

                default:
                EE_UNREACHABLE_CODE();
                break;
            }
        }

        return nullptr;
    }
}