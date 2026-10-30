#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Import/ImporterSource.h"
#include "Base/Memory/UniquePtr.h"
#include "Base/FileSystem/FileSystemPath.h"
#include "Base/Types/StringID.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    class EE_ENGINETOOLS_API File
    {
    public:

        File( Source const& source );
        virtual ~File() = default;

        virtual bool IsValid() const { return !m_fileData.empty() && !HasErrorOccurred(); }

        inline bool HasErrorOccurred() const { return !m_error.empty(); }
        inline String const& GetErrorMessage() const { return m_error; }

    private:

        File( File const& ) = delete;
        File( File&& ) = delete;
        File& operator=( File const& ) = delete;
        File& operator=( File&& ) = delete;

    protected:

        FileSystem::Path            m_sourcePath;
        Blob                        m_fileData;
        String                      m_error;
    };

    //-------------------------------------------------------------------------
    // Image file format
    //-------------------------------------------------------------------------

    class Image;

    class EE_ENGINETOOLS_API ImageFile : public File
    {
    public:

        ImageFile( Source const& source ) : File( source ) {}

        //-------------------------------------------------------------------------

        virtual TUniquePtr<Image> ReadImage() const = 0;
    };

    //-------------------------------------------------------------------------
    // 3D scene file format
    //-------------------------------------------------------------------------

    class Mesh;
    class Animation;
    class AnimationClip;
    class Skeleton;
    class Materials;

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API SceneFile : public File
    {
    public:

        enum class ItemType
        {
            Unknown,
            Material,
            StaticMesh,
            SkeletalMesh,
            Skeleton,
            Animation
        };

        struct Item
        {
            Item() = default;
            Item( ItemType type ) : m_type( type ) {}
            Item( ItemType type, StringID ID ) : m_type( type ), m_ID( ID ) {}

            inline bool IsValid() const { return m_type != ItemType::Unknown && m_ID.IsValid(); }
            inline void Clear() { m_type = ItemType::Unknown; m_ID.Clear(); }
            inline bool operator==( Item const& rhs ) const { return ( m_type == rhs.m_type ) && ( m_ID == rhs.m_ID ); }

        public:

            ItemType                                    m_type = ItemType::Unknown;
            StringID                                    m_ID;
        };

        template<ItemType T>
        struct TItem : public Item 
        {
            TItem() : Item( T ) {} 
            TItem( StringID ID ) : Item( T, ID ) {}
        };

        struct Contents
        {
            void Clear();

        public:

            TVector<TItem<ItemType::Material>>          m_materials;
            TVector<TItem<ItemType::StaticMesh>>        m_staticMeshes;
            TVector<TItem<ItemType::SkeletalMesh>>      m_skeletalMeshes;
            TVector<TItem<ItemType::Skeleton>>          m_skeletons;
            TVector<TItem<ItemType::Animation>>         m_animations;
        };

    public:

        SceneFile( Source const& source, FileSystem::Path const& sourceDataDirectoryPath );

        //-------------------------------------------------------------------------

        virtual void GetFileContents( Contents& outContents ) const = 0;
        virtual TUniquePtr<Skeleton> ReadSkeleton( String const& skeletonRootBoneName ) const = 0;
        virtual TUniquePtr<Animation> ReadAnimation( Skeleton const* pPrimarySkeleton, TVector<Import::Skeleton const*> const& secondarySkeletons, String const& animationName, float samplingFrameRate ) const = 0;
        virtual TUniquePtr<Mesh> ReadStaticMesh( TVector<String> const& meshesToInclude ) const = 0;
        virtual TUniquePtr<Mesh> ReadSkeletalMesh( TVector<String> const& meshesToInclude ) const = 0;
        virtual TUniquePtr<Materials> ReadMaterials() const = 0;

    protected:

        FileSystem::Path                                m_sourceDataDirectoryPath;
    };
}