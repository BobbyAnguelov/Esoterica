#pragma once
#include "Base/Resource/ResourceTypeID.h"
#include "Base/TypeSystem/TypeID.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Reflection
{
    struct ReflectedResourceType
    {
    public:

        constexpr static char const* const s_baseResourceFullTypeName = "EE::Resource::IResource";

    public:

        // Fill the resource type ID and the friendly name from the macro registration string
        bool TryParseResourceRegistrationMacroString( String const& registrationStr );

    public:

        TypeID                                          m_typeID;
        ResourceTypeID                                  m_resourceTypeID;
        String                                          m_friendlyName;
        StringID                                        m_headerID;
        String                                          m_className;
        String                                          m_namespace;
        TVector<TypeID>                                 m_parents;
        bool                                            m_isDevOnly = true;
    };
}