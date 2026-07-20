
#include "ResourceDescriptor_RenderMaterial.h"

#include "Base/Imgui/ImguiFilteredCombo.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/TypeSystem/TypeInfo.h"
#include "Engine/Render/RenderSystem.h"
#include "EngineTools/PropertyGrid/PropertyGridEditor.h"
#include "EngineTools/Core/ToolsContext.h"

namespace EE::Render
{
    class SurfaceShaderPicker final : public PG::PropertyEditor, public ImGuiX::ComboWithFilterWidget<StringID>
    {
    public:

        using PropertyEditor::PropertyEditor;

        SurfaceShaderPicker( PG::PropertyEditorContext const& context, TypeSystem::PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance )
            : PropertyEditor( context, propertyInfo, pTypeInstance, pPropertyInstance )
        {
            SurfaceShaderPicker::ResetWorkingCopy();
        }

    private:

        virtual void UpdatePropertyValue() override
        {
            StringID const newValue = HasValidSelection() ? GetSelectedOption()->m_value : StringID();
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
            SetSelectedOption( m_valueCached );
        }

        virtual void HandleExternalUpdate() override
        {
            StringID* pShaderID = static_cast<StringID*>( m_pPropertyInstance );
            if ( *pShaderID != m_valueCached )
            {
                m_valueCached = *pShaderID;
                SetSelectedOption( m_valueCached );
            }
        }

        virtual Result InternalUpdateAndDraw() override
        {
            return ImGuiX::ComboWithFilterWidget<StringID>::DrawAndUpdate() ? Result::ValueUpdatedAndGridNeedsRebuild : Result::None;
        }

        virtual void PopulateOptionsList() override
        {
            RenderSystem const* pRenderSystem = m_context.m_pToolsContext->m_pSystemRegistry->GetSystem<RenderSystem>();
            TVector<MaterialShader> const& materialShaders = pRenderSystem->GetMaterialShaders();

            m_options.reserve( materialShaders.size() );
            for ( MaterialShader const& shaderInstance : materialShaders )
            {
                if ( !shaderInstance.m_showInResourceEditor )
                {
                    continue;
                }

                Option& option = m_options.emplace_back();
                option.m_label = shaderInstance.m_shaderName.c_str();
                option.m_value = shaderInstance.m_shaderName;
            }
        }

    private:

        StringID m_valueCached;
    };

    //-------------------------------------------------------------------------

    EE_PROPERTY_GRID_CUSTOM_EDITOR( SurfaceShaderPicker, "SurfaceShaderPicker" );
}
