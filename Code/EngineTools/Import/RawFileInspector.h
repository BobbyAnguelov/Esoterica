#pragma once

#include "EngineTools/_Module/API.h"
#include "Base/Imgui/MaterialDesignIcons.h"
#include "Base/FileSystem/FileSystemPath.h"
#include "Base/Memory/Pointers.h"
#include "Base/Types/Function.h"
#include "Base/Types/StringID.h"
#include "Base/TypeSystem/ReflectedType.h"
#include "Base/Resource/ResourcePath.h"
#include "Base/Time/Time.h"

//-------------------------------------------------------------------------

namespace EE
{
    class ToolsContext;
}

//-------------------------------------------------------------------------

namespace EE::Import
{
    //-------------------------------------------------------------------------
    // Describes an importable piece of data present in a raw file
    //-------------------------------------------------------------------------

    struct ImportableItem : public IReflectedType
    {
        EE_REFLECT_TYPE( ImportableItem );

        ImportableItem() = default;
        ImportableItem( ResourcePath const& path, StringID nameID ) : m_sourceFile ( path ), m_nameID( nameID ) {}

        inline bool IsValid() const { return m_sourceFile.IsValid() && m_nameID.IsValid(); }

        virtual char const* GetHeading() const { return ""; }
        virtual int32_t GetNumExtraInfoColumns() const { return 0; }
        virtual char const* GetExtraInfoColumnName( int32_t columnIdx ) const { return ""; }
        virtual InlineString GetExtraInfoColumnValue( int32_t columnIdx ) const { return ""; }

    public:

        ResourcePath                    m_sourceFile;
        StringID                        m_nameID;
    };

    //-------------------------------------------------------------------------
    // Importable Data
    //-------------------------------------------------------------------------

    struct ImportableMesh : public ImportableItem
    {
        EE_REFLECT_TYPE( ImportableMesh );

        virtual char const* GetHeading() const override { return EE_ICON_HOME_CITY" Meshes"; }
        virtual int32_t GetNumExtraInfoColumns() const override { return 1; }
        virtual char const* GetExtraInfoColumnName( int32_t columnIdx ) const override { return "Material"; }
        virtual InlineString GetExtraInfoColumnValue( int32_t columnIdx ) const override { return m_materialID.IsValid() ? m_materialID.c_str() : "No Material Set"; }

        StringID                        m_materialID;
        String                          m_extraInfo;
        bool                            m_isSkeletalMesh = false;
    };

    //-------------------------------------------------------------------------

    struct ImportableSkeleton : public ImportableItem
    {
        EE_REFLECT_TYPE( ImportableSkeleton );

        inline bool IsNullOrLocatorNode() const { return !m_childSkeletonRoots.empty(); }
        inline bool IsSkeletonNode() const { return m_childSkeletonRoots.empty(); }

        virtual char const* GetHeading() const override { return EE_ICON_SKULL" Skeletons"; }
        virtual int32_t GetNumExtraInfoColumns() const override { return 1; }
        virtual char const* GetExtraInfoColumnName( int32_t columnIdx ) const override { return "Type"; }
        virtual InlineString GetExtraInfoColumnValue( int32_t columnIdx ) const override { return IsNullOrLocatorNode() ? "Null/Locator" : "Skeleton Node"; }

    public:

        TVector<StringID>               m_childSkeletonRoots; // Only filled if this is a null root
    };

    //-------------------------------------------------------------------------

    struct ImportableAnimation : public ImportableItem
    {
        EE_REFLECT_TYPE( ImportableAnimation );

        virtual char const* GetHeading() const override { return EE_ICON_RUN" Animations"; }
        virtual int32_t GetNumExtraInfoColumns() const override { return 3; }

        virtual char const* GetExtraInfoColumnName( int32_t columnIdx ) const override 
        {
            if ( columnIdx == 0 )
            {
                return "Frames";
            }

            if ( columnIdx == 1 )
            {
                return "Duration";
            }

            if ( columnIdx == 2 )
            {
                return "FPS";
            }

            return "";
        }
        
        virtual InlineString GetExtraInfoColumnValue( int32_t columnIdx ) const override
        {
            InlineString str;

            if ( columnIdx == 0 )
            {
                float const numFrames = m_duration.ToFloat() * m_frameRate;
                return str.sprintf( "%.2f", numFrames, m_duration.ToFloat(), m_frameRate );
            }

            if ( columnIdx == 1 )
            {
                return str.sprintf( "%.2f", m_duration.ToFloat() );
            }

            if ( columnIdx == 2 )
            {
                return str.sprintf( "%.2f", m_frameRate );
            }

            return str;
        }

    public:

        Seconds                         m_duration = 0.0f;
        float                           m_frameRate = 0.0f;
    };

    //-------------------------------------------------------------------------

    struct ImportableImage : public ImportableItem
    {
        EE_REFLECT_TYPE( ImportableImage );

        virtual char const* GetHeading() const override { return EE_ICON_IMAGE" Images"; }
        virtual int32_t GetNumExtraInfoColumns() const override { return 2; }

        virtual char const* GetExtraInfoColumnName( int32_t columnIdx ) const override
        {
            if ( columnIdx == 0 )
            {
                return "Dimensions";
            }

            if ( columnIdx == 1 )
            {
                return "Channels";
            }

            return "";
        }

        virtual InlineString GetExtraInfoColumnValue( int32_t columnIdx ) const override
        {
            InlineString str;

            if ( columnIdx == 0 )
            {
                return str.sprintf( "%d x %d", m_dimensions.m_x, m_dimensions.m_y );
            }

            if ( columnIdx == 1 )
            {
                return str.sprintf( "%d", m_numChannels );
            }

            return str;
        }

    public:

        Int2                            m_dimensions = Int2::Zero;
        int32_t                         m_numChannels = 0;
    };

    //-------------------------------------------------------------------------
    // Inspector
    //-------------------------------------------------------------------------

    struct InspectorContext
    {
        inline bool IsValid() const{ return m_warningDelegate != nullptr && m_errorDelegate != nullptr && m_rawResourceDirectoryPath.IsValid(); }

    public:

        inline void LogWarning( char const* pFormat, ... ) const
        {
            va_list args;
            va_start( args, pFormat );
            InlineString str;
            str.sprintf_va_list( pFormat, args );
            m_warningDelegate( str.c_str() );
            va_end( args );
        }

        inline void LogError( char const* pFormat, ... ) const
        {
            va_list args;
            va_start( args, pFormat );
            InlineString str;
            str.sprintf_va_list( pFormat, args );
            m_errorDelegate( str.c_str() );
            va_end( args );
        }

    public:

        TFunction<void( char const* )>  m_warningDelegate;
        TFunction<void( char const* )>  m_errorDelegate;
        FileSystem::Path                m_rawResourceDirectoryPath;
    };

    //-------------------------------------------------------------------------

    enum class InspectionResult
    {
        Uninspected,
        Success,
        Failure,
        UnsupportedExtension
    };

    EE_ENGINETOOLS_API InspectionResult InspectFile( InspectorContext const& ctx, FileSystem::Path const& sourceFilePath, TVector<ImportableItem*>& outFileInfo );

    EE_ENGINETOOLS_API bool IsImportableFileType( TInlineString<6> const& extension );
}