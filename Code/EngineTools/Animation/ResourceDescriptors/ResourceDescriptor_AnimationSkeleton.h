#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "Engine/Animation/AnimationSkeleton.h"
#include "Engine/Render/RenderMesh.h"

//-------------------------------------------------------------------------

namespace EE::Import { class Skeleton; }

//-------------------------------------------------------------------------

namespace EE::Animation
{
    struct EE_ENGINETOOLS_API SkeletonResourceDescriptor final : public Resource::ResourceDescriptor
    {
        EE_REFLECT_TYPE( SkeletonResourceDescriptor );
    
        struct EE_ENGINETOOLS_API SecondarySkeleton  final : public IReflectedType
        {
            EE_REFLECT_TYPE( SecondarySkeleton );

            // The secondary skeleton
            EE_REFLECT();
            TResourcePtr<Skeleton>  m_skeleton;

            // The bone that we expect this skeleton to be attached to
            EE_REFLECT();
            StringID                m_attachmentBoneID;
        };

    public:

        virtual bool IsValid() const override { return m_skeletonPath.IsValid(); }
        virtual int32_t GetFileVersion() const override { return 0; }
        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override{ return Skeleton::GetStaticResourceTypeID(); }
        virtual FileSystem::Extension GetExtension() const override final { return Skeleton::GetStaticResourceTypeID().ToString(); }
        virtual char const* GetFriendlyName() const override final { return Skeleton::s_friendlyName; }
        virtual void PostLoad( TypeSystem::TypeRegistry const& typeRegistry ) override;

        virtual void GetCompileDependencies( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& sourceResourceDirectoryPath, String const& subResourceName, TVector<Resource::CompileDependency>& outDependencies ) const override
        {
            if ( m_skeletonPath.IsValid() )
            {
                outDependencies.emplace_back( m_skeletonPath, false );
            }
        }

        virtual void Clear() override
        {
            m_skeletonPath.Clear();
            m_skeletonRootBoneName.clear();
            m_boneMaskSetDefinitions.clear();
            m_highLODBones.clear();
            m_previewMesh.Clear();
            m_secondarySkeletons.clear();
        }

        //-------------------------------------------------------------------------

        inline bool IsValidBoneMaskSetID( StringID maskSetID ) const
        {
            for ( auto const& maskDefinition : m_boneMaskSetDefinitions )
            {
                if ( maskDefinition.m_ID == maskSetID )
                {
                    return true;
                }
            }

            return false;
        }

        inline TInlineVector<StringID, 10> GetBoneMaskSetIDs() const
        {
            TInlineVector<StringID, 10> boneMaskSetIDs;
            for ( auto const& maskDefinition : m_boneMaskSetDefinitions )
            {
                boneMaskSetIDs.emplace_back( maskDefinition.m_ID );
            }
            return boneMaskSetIDs;
        }

        bool ValidateData( Log* pLog ) const;

        StringID GetUniqueBoneMaskSetID( String const& desiredID ) const;

        StringID GetUniqueFloatChannelSetID( String const& desiredID ) const;

        // Get the hash of the relevant data that resources will depend on
        uint64_t GetClipDependencyCompileHash() const
        {
            String compileHashStr = m_skeletonPath.GetString();
            compileHashStr.append( m_skeletonRootBoneName );
            return Hash::GetHash64( compileHashStr );
        }

    public:

        EE_REFLECT();
        DataPath                                    m_skeletonPath;

        // Optional value that specifies the name of the skeleton hierarchy to use, if it is unset, we use the first skeleton we find
        EE_REFLECT();
        String                                      m_skeletonRootBoneName;

        // The list of secondary skeleton we could expect to be attached to this skeleton
        EE_REFLECT();
        TVector<SecondarySkeleton>                  m_secondarySkeletons;

        EE_REFLECT( Hidden );
        TVector<BoneMaskSetDefinition>              m_boneMaskSetDefinitions;

        // The set of bones that only matter when working with this skeleton at high LOD
        EE_REFLECT( Hidden );
        TVector<StringID>                           m_highLODBones;

        // The set of float channels this skeleton can try to sample
        EE_REFLECT( Hidden );
        TVector<FloatChannelSet>                    m_floatChannelSets;

        // Preview
        //-------------------------------------------------------------------------

        // Preview Only: the mesh to use to preview
        EE_REFLECT( Category = "Preview" );
        TResourcePtr<Render::SkeletalMesh>          m_previewMesh;
    };
}