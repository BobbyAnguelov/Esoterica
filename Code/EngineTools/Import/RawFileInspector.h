#pragma once

#include "EngineTools/_Module/API.h"
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
        virtual InlineString GetDescription() const { return InlineString(); }

    public:

        ResourcePath                    m_sourceFile;
        StringID                        m_nameID;
    };

    //-------------------------------------------------------------------------
    // Importable Data
    //-------------------------------------------------------------------------
    // TODO: Make this extensible once we have a lot of different types

    struct ImportableMesh : public ImportableItem
    {
        EE_REFLECT_TYPE( ImportableMesh );

        virtual InlineString GetDescription() const override 
        {
            return InlineString( m_materialID.IsValid() ? m_materialID.c_str() : "No Material Set" );
        }

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

        virtual InlineString GetDescription() const override 
        {
            return InlineString( IsNullOrLocatorNode() ? "Null/Locator" : "Skeleton Node" );
        }

    public:

        TVector<StringID>               m_childSkeletonRoots; // Only filled if this is a null root
    };

    //-------------------------------------------------------------------------

    struct ImportableAnimation : public ImportableItem
    {
        EE_REFLECT_TYPE( ImportableAnimation );

        virtual InlineString GetDescription() const override 
        {
            float const numFrames = m_duration.ToFloat() * m_frameRate;
            return InlineString( InlineString::CtorSprintf(), "%.2f Frames, %.2fs, %.2f fps", numFrames, m_duration.ToFloat(), m_frameRate ); 
        }

    public:

        Seconds                         m_duration = 0.0f;
        float                           m_frameRate = 0.0f;
    };

    //-------------------------------------------------------------------------

    struct ImportableImage : public ImportableItem
    {
        EE_REFLECT_TYPE( ImportableImage );

        virtual InlineString GetDescription() const override 
        { 
            return InlineString( InlineString::CtorSprintf(), "%d x %d, channels: %d", m_dimensions.m_x, m_dimensions.m_y, m_numChannels );
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