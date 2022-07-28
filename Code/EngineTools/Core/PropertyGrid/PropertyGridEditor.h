#pragma once

#include "System/TypeSystem/CoreTypeIDs.h"

//-------------------------------------------------------------------------

namespace EE { class ToolsContext; }

//-------------------------------------------------------------------------

namespace EE::TypeSystem
{
    class PropertyInfo;

    //-------------------------------------------------------------------------

    class PropertyEditor
    {

    public:

        PropertyEditor( ToolsContext const* pToolsContext, PropertyInfo const& propertyInfo, uint8_t* pPropertyInstance );
        virtual ~PropertyEditor() = default;

        // Returns true if the value was updated
        bool UpdateAndDraw();

        // Actually set the real property value from the working copy
        virtual void UpdatePropertyValue() = 0;

        // Called to reset the cached working copy value to that of the actual instance
        virtual void ResetWorkingCopy() = 0;

    protected:

        // Check if the external value is different from the cached working copy and if so updates the cached working copy
        virtual void HandleExternalUpdate() = 0;

        // Draw the editor widgets and handle the user input, returns true if the value has been updated
        virtual bool InternalUpdateAndDraw() = 0;

    protected:

        ToolsContext const*     m_pToolsContext;
        PropertyInfo const&     m_propertyInfo;
        uint8_t*                m_pPropertyInstance;
        CoreTypeID const        m_coreType;
    };
}