
#include "ResourceDescriptor_RenderMaterial.h"
#include "Base/Imgui/ImguiInputs.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/TypeSystem/TypeInfo.h"
#include "Engine/Render/RenderSystem.h"
#include "EngineTools/PropertyGrid/PropertyGridEditor.h"
#include "EngineTools/Core/ToolsContext.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class SurfaceShaderPicker final : public PG::PropertyEditor
    {
    public:

        using PropertyEditor::PropertyEditor;

        SurfaceShaderPicker( PG::PropertyEditorContext const& context, TypeSystem::PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance )
            : PropertyEditor( context, propertyInfo, pTypeInstance, pPropertyInstance )
        {
            auto OptionsProviderFn = [this] ( TVector<ImGuiX::OptionData::Option>& options )
            {
                RenderSystem const* pRenderSystem = m_context.m_pToolsContext->m_pSystemRegistry->GetSystem<RenderSystem>();
                TVector<MaterialShader> const& materialShaders = pRenderSystem->GetMaterialShaders();

                options.reserve( materialShaders.size() );
                for ( MaterialShader const& shaderInstance : materialShaders )
                {
                    if ( !shaderInstance.m_showInResourceEditor )
                    {
                        continue;
                    }

                    options.emplace_back( shaderInstance.m_shaderName.c_str() );
                }

                eastl::sort( options.begin(), options.end() );
            };

            m_optionData.SetOptionProvider( OptionsProviderFn );

            //-------------------------------------------------------------------------

            SurfaceShaderPicker::ResetWorkingCopy();
        }

    private:

        virtual void UpdatePropertyValue() override
        {
            StringID newValue;
            auto pSelectedOption = m_optionData.TryGetOption( m_selectedOptionID );
            if ( pSelectedOption != nullptr )
            {
                newValue = StringID( pSelectedOption->m_text );
            }

            if ( m_valueCached == newValue )
            {
                return;
            }

            // Update the cached ID
            m_valueCached = newValue;
            *static_cast<StringID*>( m_pPropertyInstance ) = m_valueCached;

            // Update the instance
            if ( m_valueCached.IsValid() )
            {
                String shaderParameterTypeName;
                shaderParameterTypeName.sprintf( "EE::Render::Shaders::%sParameters", m_valueCached.c_str() );

                TypeSystem::TypeInfo const* pShaderParameterTypeInfo = m_context.m_pToolsContext->m_pTypeRegistry->GetTypeInfo( shaderParameterTypeName );
                EE_ASSERT( pShaderParameterTypeInfo != nullptr );
                Cast<MaterialResourceDescriptor>( m_pTypeInstance )->m_shaderParameters.CreateInstance( pShaderParameterTypeInfo );
            }
            else // Destroy instance
            {
                Cast<MaterialResourceDescriptor>( m_pTypeInstance )->m_shaderParameters.DestroyInstance();
            }
        }

        virtual void ResetWorkingCopy() override
        {
            m_valueCached = *static_cast<StringID*>( m_pPropertyInstance );
        }

        virtual void HandleExternalUpdate() override
        {
            StringID* pShaderID = static_cast<StringID*>( m_pPropertyInstance );
            if ( *pShaderID != m_valueCached )
            {
                m_valueCached = *pShaderID;
            }
        }

        virtual Result InternalUpdateAndDraw() override
        {
            m_selectedOptionID.Clear();
            if ( m_valueCached.IsValid() )
            {
                m_selectedOptionID = m_optionData.FindItemIDByText( m_valueCached.c_str() );
            }

            ImGui::SetNextItemWidth( -1 );
            return ImGuiX::ComboWithFilter( "SSP", &m_optionData, m_selectedOptionID ) ? Result::ValueUpdatedAndGridNeedsRebuild : Result::None;
        }

        SurfaceShaderPicker( SurfaceShaderPicker const& ) = delete;
        SurfaceShaderPicker( SurfaceShaderPicker&& ) = delete;
        SurfaceShaderPicker& operator=( SurfaceShaderPicker const& ) = delete;
        SurfaceShaderPicker& operator=( SurfaceShaderPicker&& ) = delete;

    private:

        StringID                    m_valueCached;
        ImGuiX::OptionData          m_optionData;
        UUID                        m_selectedOptionID;
    };

    //-------------------------------------------------------------------------

    EE_PROPERTY_GRID_CUSTOM_EDITOR( SurfaceShaderPicker, "SurfaceShaderPicker" );
}
