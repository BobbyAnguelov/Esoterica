#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "Engine/Animation/AnimationSkeleton.h"
#include "Engine/Render/Mesh/SkeletalMesh.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    struct EE_ENGINETOOLS_API SkeletonResourceDescriptor final : public Resource::ResourceDescriptor
    {
        EE_REFLECT_TYPE( SkeletonResourceDescriptor );
    
        virtual bool IsValid() const override { return m_skeletonPath.IsValid(); }
        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override{ return Skeleton::GetStaticResourceTypeID(); }

        virtual void GetCompileDependencies( TVector<ResourcePath>& outDependencies ) override
        {
            if ( m_skeletonPath.IsValid() )
            {
                outDependencies.emplace_back( m_skeletonPath );
            }
        }

        inline TInlineVector<StringID, 10> GetBoneMaskIDs() const
        {
            TInlineVector<StringID, 10> boneMaskIDs;
            for ( auto const& maskDefinition : m_boneMaskDefinitions )
            {
                boneMaskIDs.emplace_back( maskDefinition.m_ID );
            }
            return boneMaskIDs;
        }

        virtual void Clear() override
        {
            m_skeletonPath.Clear();
            m_skeletonRootBoneName.clear();
            m_boneMaskDefinitions.clear();
            m_highLODBones.clear();
            m_previewMesh.Clear();
            m_previewAttachmentSocketID.Clear();
        }

    public:

        EE_REFLECT();
        ResourcePath                                m_skeletonPath;

        // Optional value that specifies the name of the skeleton hierarchy to use, if it is unset, we use the first skeleton we find
        EE_REFLECT();
        String                                      m_skeletonRootBoneName;

        EE_REFLECT( "IsToolsReadOnly" : "True" );
        TVector<BoneMaskDefinition>                 m_boneMaskDefinitions;

        // The set of bones that only matter when working with this skeleton at high LOD
        EE_REFLECT( "IsToolsReadOnly" : "True" );
        TVector<StringID>                           m_highLODBones;

        // Preview
        //-------------------------------------------------------------------------

        // Preview Only: the mesh to use to preview
        EE_REFLECT( "Category" : "Preview" );
        TResourcePtr<Render::SkeletalMesh>          m_previewMesh;

        // Preview Only: The default attachment bone ID that we use when attaching to a parent skeleton
        EE_REFLECT( "Category" : "Preview" );
        StringID                                    m_previewAttachmentSocketID;
    };
}