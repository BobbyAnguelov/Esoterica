#include "PropertyGridEditor.h"
#include "EngineTools/Core/ToolsContext.h"
#include "System/TypeSystem/PropertyInfo.h"
#include "System/ThirdParty/imgui/imgui.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem
{
    PropertyEditor::PropertyEditor( ToolsContext const* pToolsContext, PropertyInfo const& propertyInfo, uint8_t* m_pPropertyInstance )
        : m_pToolsContext( pToolsContext )
        , m_propertyInfo( propertyInfo )
        , m_pPropertyInstance( m_pPropertyInstance )
        , m_coreType( GetCoreType( propertyInfo.m_typeID ) )
    {
        EE_ASSERT( m_pPropertyInstance != nullptr );
    }

    bool PropertyEditor::UpdateAndDraw()
    {
        ImGui::PushID( m_pPropertyInstance );
        HandleExternalUpdate();
        bool const result = InternalUpdateAndDraw();
        ImGui::PopID();

        return result;
    }
}