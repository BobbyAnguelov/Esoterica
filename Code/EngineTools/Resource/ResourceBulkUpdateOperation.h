#pragma once
#include "EngineTools/_Module/API.h"
#include "Base/Resource/ResourceTypeID.h"
#include "Base/Types/Arrays.h"
#include "Base/TypeSystem/ReflectedType.h"

//-------------------------------------------------------------------------
//  Bulk Edit Operation
//-------------------------------------------------------------------------
// This is a tool for performing bulk resource descriptor edits that might rely on the loaded compiled resource

namespace EE { class ToolsContext; }

//-------------------------------------------------------------------------

namespace EE::Resource
{
    class IResource;
    struct ResourceDescriptor;

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API BulkUpdateOperation : public IReflectedType
    {
        EE_REFLECT_TYPE( BulkUpdateOperation );

    public:

        virtual ~BulkUpdateOperation() = default;

        // Do we need to load the resource to perform the operation?
        virtual bool RequireResourceLoad() const { return true; }

        // What types does this operate on?
        virtual TInlineVector<ResourceTypeID, 4> GetApplicableTypeIDs() const = 0;

        // Return true to signal that the descriptor was modified and requires saving
        virtual bool PerformOperation( ToolsContext const* pToolsContext, IResource const* pResource, ResourceDescriptor &resourceDesc ) = 0;
    };
}