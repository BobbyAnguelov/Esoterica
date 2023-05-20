#pragma once

#include "ReflectionDataTypes.h"
#include "System/Resource/ResourceTypeID.h"
#include "System/FileSystem/FileSystemPath.h"
#include "System/TypeSystem/PropertyPath.h"
#include <sqlite3.h>

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Reflection
{
    class ReflectionDatabase
    {
        static uint32_t const constexpr s_defaultStatementBufferSize = 8096;

    public:

        ReflectionDatabase();
        ~ReflectionDatabase();

        // Database functions
        //-------------------------------------------------------------------------

        bool IsConnected() const { return m_pDatabase != nullptr; }
        inline bool HasErrorOccurred() const { return !m_errorMessage.empty(); }
        inline String const& GetError() const { return m_errorMessage; }

        bool ReadDatabase( FileSystem::Path const& databasePath );
        bool WriteDatabase( FileSystem::Path const& databasePath );

        // Module functions
        //-------------------------------------------------------------------------

        TVector<ProjectInfo> const& GetAllRegisteredProjects() const { return m_reflectedProjects; }
        bool IsProjectRegistered( ProjectID projectID ) const;
        ProjectInfo const* GetProjectDesc( ProjectID projectID ) const;
        void UpdateProjectList( TVector<ProjectInfo> const& registeredProjects );

        bool IsHeaderRegistered( HeaderID headerID ) const;
        HeaderInfo const* GetHeaderDesc( HeaderID headerID ) const;
        void UpdateHeaderRecord( HeaderInfo const& header );

        // Type functions
        //-------------------------------------------------------------------------

        ReflectedType const* GetType( TypeID typeID ) const;
        ReflectedType* GetType( TypeID typeID ) { return const_cast<ReflectedType*>( const_cast<ReflectionDatabase const*>( this )->GetType( typeID ) ); }
        TVector<ReflectedType> const& GetAllTypes() const { return m_reflectedTypes; }
        bool IsTypeRegistered( TypeID typeID ) const;
        bool IsTypeDerivedFrom( TypeID typeID, TypeID parentTypeID ) const;
        void GetAllTypesForHeader( HeaderID headerID, TVector<ReflectedType>& types ) const;
        void GetAllTypesForProject( ProjectID projectID, TVector<ReflectedType>& types ) const;
        void RegisterType( ReflectedType const* pType, bool onlyUpdateDevFlag );

        // Property functions
        //-------------------------------------------------------------------------

        ReflectedProperty const* GetPropertyTypeDescriptor( TypeID typeID, PropertyPath const& pathID ) const;

        // Resource functions
        //-------------------------------------------------------------------------

        bool IsResourceRegistered( ResourceTypeID typeID ) const;
        void RegisterResource( ReflectedResourceType const* pDesc );
        TVector<ReflectedResourceType> const& GetAllRegisteredResourceTypes() const { return m_reflectedResourceTypes; }

        // Removes all irrelevant parents from the registered resource types
        void CleanupResourceHierarchy();

        // Cleaning
        //-------------------------------------------------------------------------

        void DeleteTypesForHeader( HeaderID headerID );
        void DeleteObseleteHeadersAndTypes( TVector<HeaderID> const& registeredHeaders );
        void DeleteObseleteProjects( TVector<ProjectInfo> const& registeredProjects );

    private:

        // Data
        //-------------------------------------------------------------------------

        bool CreateTables();
        bool DropTables();

        bool ReadAdditionalTypeData( ReflectedType& type );
        bool ReadAdditionalEnumData( ReflectedType& type );
        bool ReadAdditionalResourceTypeData( ReflectedResourceType& type );

        bool WriteAdditionalTypeData( ReflectedType const& type );
        bool WriteAdditionalEnumData( ReflectedType const& type );
        bool WriteAdditionalResourceTypeData( ReflectedResourceType const& type );

        // SQLite
        //-------------------------------------------------------------------------

        bool Connect( FileSystem::Path const& databasePath, bool readOnlyAccess = false, bool useMutex = false );
        bool Disconnect();

        bool IsValidSQLiteResult( int result, char const* pErrorMessage = nullptr ) const;

        void FillStatementBuffer( char const* pFormat, ... ) const;
        bool ExecuteSimpleQuery( char const* pFormat, ... ) const;

        bool BeginTransaction() const;
        bool EndTransaction() const;

    private:

        sqlite3*                            m_pDatabase = nullptr;
        mutable String                      m_errorMessage;
        mutable char                        m_statementBuffer[s_defaultStatementBufferSize] = { 0 };

        ReflectedType                       m_reflectedTypeBase;
        TVector<ReflectedType>              m_reflectedTypes;
        TVector<HeaderInfo>                 m_reflectedHeaders;
        TVector<ProjectInfo>                m_reflectedProjects;
        TVector<ReflectedResourceType>      m_reflectedResourceTypes;
    };
}