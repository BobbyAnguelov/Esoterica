#pragma once

#include "ReflectedSolution.h"
#include "ReflectedType.h"
#include "ReflectedResourceType.h"
#include "ReflectedDataFileType.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem { class PropertyPath; }

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Reflection
{
    class ReflectionDatabase
    {

    public:

        ReflectionDatabase( TVector<ReflectedProject> const& projects );

        inline bool HasErrors() const { return !m_errorMessage.empty(); }
        char const* GetErrorMessage() const { return m_errorMessage.c_str(); }

        inline bool HasWarnings() const { return !m_warningMessage.empty(); }
        char const* GetWarningMessage() const { return m_warningMessage.c_str(); }

        // Solution Info
        //-------------------------------------------------------------------------

        TVector<ReflectedProject> const& GetReflectedProjects() const { return m_reflectedProjects; }
        bool IsProjectRegistered( StringID projectID ) const;
        ReflectedProject const* GetProjectDesc( StringID projectID ) const;

        bool IsHeaderRegistered( StringID headerID ) const;
        ReflectedHeader const* GetReflectedHeader( StringID headerID ) const;

        // Type Info
        //-------------------------------------------------------------------------

        ReflectedType const* GetType( TypeID typeID ) const;
        ReflectedType* GetType( TypeID typeID ) { return const_cast<ReflectedType*>( const_cast<ReflectionDatabase const*>( this )->GetType( typeID ) ); }
        TVector<ReflectedType> const& GetAllTypes() const { return m_reflectedTypes; }
        bool IsTypeRegistered( TypeID typeID ) const;
        bool IsTypeDerivedFrom( TypeID typeID, TypeID parentTypeID ) const;
        void GetAllTypesForHeader( StringID headerID, TVector<ReflectedType>& types ) const;
        void GetAllTypesForProject( StringID projectID, TVector<ReflectedType>& types ) const;
        void RegisterType( ReflectedType const* pType );
        void UpdateRegisteredType( ReflectedType const* pType );

        ReflectedProperty const* GetPropertyInfo( TypeID typeID, PropertyPath const& pathID ) const;

        // Parse all reflected type json metadata
        bool ProcessAndValidateReflectedProperties();

        // Resource Info
        //-------------------------------------------------------------------------

        ReflectedResourceType const* GetResourceType( ResourceTypeID typeID ) const;
        ReflectedResourceType* GetResourceType( ResourceTypeID typeID ) { return const_cast<ReflectedResourceType*>( const_cast<ReflectionDatabase const*>( this )->GetResourceType( typeID ) ); }

        bool IsResourceRegistered( ResourceTypeID typeID ) const;
        bool IsResourceRegistered( TypeID typeID ) const;
        void RegisterResource( ReflectedResourceType const* pResource );
        void UpdateRegisteredResource( ReflectedResourceType const* pResource );
        TVector<ReflectedResourceType> const& GetAllRegisteredResourceTypes() const { return m_reflectedResourceTypes; }

        // Removes all irrelevant parents from the registered resource types
        void CleanupResourceHierarchy();

        // Data Info
        //-------------------------------------------------------------------------

        ReflectedDataFileType const* GetDataFileType( TypeID typeID ) const;
        ReflectedDataFileType* GetDataFileType( TypeID typeID ) { return const_cast<ReflectedDataFileType*>( const_cast<ReflectionDatabase const*>( this )->GetDataFileType( typeID ) ); }

        bool IsDataFileRegistered( TypeID typeID ) const;
        void RegisterDataFile( ReflectedDataFileType const* pDataFile );
        void UpdateRegisteredDataFile( ReflectedDataFileType const* pDataFile );
        TVector<ReflectedDataFileType> const& GetAllRegisteredDataFileTypes() const { return m_reflectedDataFileTypes; }

        bool ValidateDataFileRegistrations();

    private:

        bool LogError( char const* pFormat, ... ) const;
        void LogWarning( char const* pFormat, ... ) const;

    private:

        TVector<ReflectedProject>           m_reflectedProjects;
        ReflectedType                       m_reflectedTypeBase;
        TVector<ReflectedType>              m_reflectedTypes;
        TVector<ReflectedResourceType>      m_reflectedResourceTypes;
        TVector<ReflectedDataFileType>      m_reflectedDataFileTypes;
        mutable String                      m_warningMessage;
        mutable String                      m_errorMessage;
    };
}