#pragma once

#include "System/Esoterica.h"

//-------------------------------------------------------------------------

namespace EE { class ToolsContext; }
namespace EE::Resource { class ResourceFilePicker; }

//-------------------------------------------------------------------------

namespace EE::TypeSystem
{
    class PropertyInfo;
    class PropertyEditor;

    PropertyEditor* CreatePropertyEditor( ToolsContext const* pToolsContext, Resource::ResourceFilePicker& resourcePicker, PropertyInfo const& propertyInfo, uint8_t* pPropertyInstance );
}