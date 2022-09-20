#pragma once

#include "System/Esoterica.h"

//-------------------------------------------------------------------------

namespace EE { class ToolsContext; }
namespace EE::Resource { class ResourcePicker; }

//-------------------------------------------------------------------------

namespace EE::TypeSystem
{
    class PropertyInfo;
    class PropertyEditor;

    PropertyEditor* CreatePropertyEditor( ToolsContext const* pToolsContext, Resource::ResourcePicker& resourcePicker, PropertyInfo const& propertyInfo, uint8_t* pPropertyInstance );
}