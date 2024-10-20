#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "Engine/Animation/AnimationSkeleton.h"
#include "Engine/Render/Mesh/SkeletalMesh.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    //-------------------------------------------------------------------------
    // Bone Mask Definition
    //-------------------------------------------------------------------------
    // Serialized data that describes a set of weights from which we can generate a bone mask

    struct EE_ENGINETOOLS_API BoneMaskDefinition final : public IReflectedType
    {
        friend class SkeletonEditor;

        EE_REFLECT_TYPE( BoneMaskDefinition );

    public:

        struct EE_ENGINETOOLS_API BoneWeight final : public IReflectedType
        {
            EE_REFLECT_TYPE( BoneWeight );

            BoneWeight() = default;

            BoneWeight( StringID const ID, float weight )
                : m_boneID( ID )
                , m_weight( weight )
            {
                EE_ASSERT( ID.IsValid() && m_weight >= 0.0f && m_weight <= 1.0f );
            }

            EE_REFLECT( ReadOnly );
            StringID            m_boneID;

            EE_REFLECT( ReadOnly );
            float               m_weight = 0.0f;
        };


    public:

        BoneMaskDefinition() = default;
        BoneMaskDefinition( StringID ID ) : m_ID( ID ) {}

        bool IsValid() const;

        void GenerateSerializedBoneMask( Skeleton const* pSkeleton, BoneMask::SerializedData& outData, TVector<StringID>* pOutOptionalListOfDetectedMissingBones = nullptr ) const;

    public:

        EE_REFLECT( ReadOnly );
        StringID                m_ID;

        EE_REFLECT( ReadOnly );
        TVector<BoneWeight>     m_weights;
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API SkeletonResourceDescriptor final : public Resource::ResourceDescriptor
    {
        EE_REFLECT_TYPE( SkeletonResourceDescriptor );
    
        virtual bool IsValid() const override { return m_skeletonPath.IsValid(); }
        virtual int32_t GetFileVersion() const override { return 0; }
        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override{ return Skeleton::GetStaticResourceTypeID(); }
        virtual FileSystem::Extension GetExtension() const override final { return Skeleton::GetStaticResourceTypeID().ToString(); }
        virtual char const* GetFriendlyName() const override final { return Skeleton::s_friendlyName; }

        virtual void GetCompileDependencies( TVector<DataPath>& outDependencies ) override
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
        DataPath                                    m_skeletonPath;

        // Optional value that specifies the name of the skeleton hierarchy to use, if it is unset, we use the first skeleton we find
        EE_REFLECT();
        String                                      m_skeletonRootBoneName;

        EE_REFLECT( Hidden );
        TVector<BoneMaskDefinition>                 m_boneMaskDefinitions;

        // The set of bones that only matter when working with this skeleton at high LOD
        EE_REFLECT( Hidden );
        TVector<StringID>                           m_highLODBones;

        // Preview
        //-------------------------------------------------------------------------

        // Preview Only: the mesh to use to preview
        EE_REFLECT( Category = "Preview" );
        TResourcePtr<Render::SkeletalMesh>          m_previewMesh;

        // Preview Only: The default attachment bone ID that we use when attaching to a parent skeleton
        EE_REFLECT( Category = "Preview" );
        StringID                                    m_previewAttachmentSocketID;
    };
}