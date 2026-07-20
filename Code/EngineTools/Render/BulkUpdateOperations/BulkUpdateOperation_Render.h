#pragma once
#include "EngineTools/Resource/ResourceBulkUpdateOperation.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    struct FixUpMaterialAssignments : public Resource::BulkUpdateOperation
    {
        EE_REFLECT_TYPE( FixUpMaterialAssignments );

    public:

        virtual TInlineVector<ResourceTypeID, 4> GetApplicableTypeIDs() const override;
        virtual bool PerformOperation( ToolsContext const* pToolsContext, Resource::IResource const* pResource, Resource::ResourceDescriptor &resourceDesc ) override;
    };

    //-------------------------------------------------------------------------

    struct FixUpTextureReferences : public Resource::BulkUpdateOperation
    {
        EE_REFLECT_TYPE( FixUpTextureReferences );

    public:

        virtual TInlineVector<ResourceTypeID, 4> GetApplicableTypeIDs() const override;
        virtual bool RequireResourceLoad() const override { return false; }
        virtual bool PerformOperation( ToolsContext const* pToolsContext, Resource::IResource const* pResource, Resource::ResourceDescriptor &resourceDesc ) override;
    };
}