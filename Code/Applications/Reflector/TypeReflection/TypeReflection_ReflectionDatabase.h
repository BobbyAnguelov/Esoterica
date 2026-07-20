#pragma once

#include "TypeReflection_ReflectedType.h"
#include "TypeReflection_ReflectedResourceType.h"
#include "TypeReflection_ReflectedDataFileType.h"
#include "Applications/Reflector/ReflectedProject.h"
#include "Base/Logging/Log.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem { class PropertyPath; }

//-------------------------------------------------------------------------

namespace EE::Reflection
{
    class ReflectionDatabase : public Log
    {

    public:

        ReflectionDatabase( TVector<ReflectedProject> const& projects );

        // Solution Info
        //-------------------------------------------------------------------------

        TVector<ReflectedProject> const& GetReflectedProjects() const { return m_reflectedProjects; }
        bool IsProjectRegistered( StringID projectID ) const;
        ReflectedProject const* GetProjectDesc( StringID projectID ) const;

        bool IsHeaderRegistered( StringID headerID ) const;
        ReflectedHeader const* GetReflectedHeader( StringID headerID ) const;

        // Type Info
        //-------------------------------------------------------------------------

        ReflectedType const* GetType( TypeSystem::TypeID typeID ) const;
        ReflectedType* GetType( TypeSystem::TypeID typeID ) { return const_cast<ReflectedType*>( const_cast<ReflectionDatabase const*>( this )->GetType( typeID ) ); }
        TVector<ReflectedType> const& GetAllTypes() const { return m_reflectedTypes; }
        bool IsTypeRegistered( TypeSystem::TypeID typeID ) const;
        bool IsTypeDerivedFrom( TypeSystem::TypeID typeID, TypeSystem::TypeID parentTypeID ) const;
        void GetAllTypesForHeader( StringID headerID, TVector<ReflectedType>& types ) const;
        void GetAllTypesForProject( StringID projectID, TVector<ReflectedType>& types ) const;
        void RegisterType( ReflectedType const* pType );
        void UpdateRegisteredType( ReflectedType const* pType );

        ReflectedProperty const* GetPropertyInfo( TypeSystem::TypeID typeID, TypeSystem::PropertyPath const& pathID ) const;

        // Parse all reflected type json metadata
        bool ProcessAndValidateReflectedProperties();

        // Resource Info
        //-------------------------------------------------------------------------

        ReflectedResourceType const* GetResourceType( ResourceTypeID typeID ) const;
        ReflectedResourceType* GetResourceType( ResourceTypeID typeID ) { return const_cast<ReflectedResourceType*>( const_cast<ReflectionDatabase const*>( this )->GetResourceType( typeID ) ); }

        bool IsResourceRegistered( ResourceTypeID typeID ) const;
        bool IsResourceRegistered( TypeSystem::TypeID typeID ) const;
        void RegisterResource( ReflectedResourceType const* pResource );
        void UpdateRegisteredResource( ReflectedResourceType const* pResource );
        TVector<ReflectedResourceType> const& GetAllRegisteredResourceTypes() const { return m_reflectedResourceTypes; }

        // Removes all irrelevant parents from the registered resource types
        void CleanupResourceHierarchy();

        // Data Info
        //-------------------------------------------------------------------------

        ReflectedDataFileType const* GetDataFileType( TypeSystem::TypeID typeID ) const;
        ReflectedDataFileType* GetDataFileType( TypeSystem::TypeID typeID ) { return const_cast<ReflectedDataFileType*>( const_cast<ReflectionDatabase const*>( this )->GetDataFileType( typeID ) ); }

        bool IsDataFileRegistered( TypeSystem::TypeID typeID ) const;
        void RegisterDataFile( ReflectedDataFileType const* pDataFile );
        void UpdateRegisteredDataFile( ReflectedDataFileType const* pDataFile );
        TVector<ReflectedDataFileType> const& GetAllRegisteredDataFileTypes() const { return m_reflectedDataFileTypes; }

        bool ValidateDataFileRegistrations();

    private:

        TVector<ReflectedProject>           m_reflectedProjects;
        ReflectedType                       m_reflectedTypeBase;
        TVector<ReflectedType>              m_reflectedTypes;
        TVector<ReflectedResourceType>      m_reflectedResourceTypes;
        TVector<ReflectedDataFileType>      m_reflectedDataFileTypes;
    };
}