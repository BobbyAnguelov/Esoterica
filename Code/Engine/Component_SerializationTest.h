#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Entity/EntityDescriptors.h"
#include "Engine/Entity/EntityComponent.h"
#include "Engine/Physics/PhysicsCollision.h"
#include "Base/Types/Color.h"
#include "Base/TypeSystem/ReflectedType.h"
#include "Base/Math/Transform.h"
#include "Base/Resource/ResourcePtr.h"
#include "Base/Time/Time.h"
#include "Base/Types/Percentage.h"
#include "Base/Math/NumericRange.h"
#include "Base/Math/FloatCurve.h"
#include "Base/Types/Tag.h"
#include "Base/Render/RenderShader.h"

//-------------------------------------------------------------------------

#define EE_PROPERTY( ... )

namespace EE
{
    struct EE_ENGINE_API ExternalTestSubSubStruct : public IReflectedType
    {
        EE_REFLECT_TYPE( ExternalTestSubSubStruct );
        EE_REFLECT() TVector<float>                               m_dynamicArray = { 1.0f, 2.0f, 3.0f };
    };

    struct EE_ENGINE_API ExternalTestSubStruct : public IReflectedType
    {
        EE_REFLECT_TYPE( ExternalTestSubStruct );

        EE_REFLECT() TVector<float>                               m_floats = { 0.3f, 5.0f, 7.0f };
        EE_REFLECT() TVector<ExternalTestSubSubStruct>            m_dynamicArray = { ExternalTestSubSubStruct(), ExternalTestSubSubStruct() };
    };

    struct EE_ENGINE_API ExternalTestStruct : public IReflectedType
    {
        EE_REFLECT_TYPE( ExternalTestStruct );

        EE_REFLECT( "Category" : "A", "IsToolsReadOnly" : true, "Description" : "This is a uint8" )
        uint8_t                                      m_uint8 = 8;

        EE_REFLECT( "Category" : "B", "IsToolsReadOnly" : false, "Description" : "This is a uint16" ) uint16_t m_uint16 = 16;
        uint16_t m_DONTREFLECTTHIS;

        EE_REFLECT( "Category" : "C", "IsToolsReadOnly" : true, "Description" : "This is a uint32" )
        uint32_t                                     m_uint32 = 32;

        EE_REFLECT();
        UUID                                         m_UUID;

        EE_REFLECT( "Category" : "C", "Description" : "This is a uint64" );
        uint64_t                                     m_U64 = 64;

        EE_REFLECT();
        EulerAngles                                  m_eulerAngles = EulerAngles( 23, 45, 56 );

        EE_REFLECT();
        TVector<ExternalTestSubStruct>               m_dynamicArray = { ExternalTestSubStruct(), ExternalTestSubStruct() };
    };

    #if EE_DEVELOPMENT_TOOLS
    struct DevOnlyStruct : public IReflectedType
    {
        EE_REFLECT_TYPE( DevOnlyStruct );

        EE_REFLECT() float m_float;
    };
    #endif

    //-------------------------------------------------------------------------

    enum class TestFlags
    {
        EE_REFLECT_ENUM

        a = 1,
        B = 2,
        c = 3,
        Duplicate = 5,
        D = 4,
        A = 5,
    };

    #if EE_DEVELOPMENT_TOOLS
    enum class DevOnlyEnum
    {
        EE_REFLECT_ENUM

        Moo, // Sound cows make
        // Animal
        Cow
    };
    #endif

    //-------------------------------------------------------------------------

    class EE_ENGINE_API TestComponent : public EntityComponent
    {
        EE_ENTITY_COMPONENT( TestComponent );

    public:

        struct InternalStruct : public IReflectedType
        {
            EE_REFLECT_TYPE( InternalStruct );

            EE_REFLECT() EulerAngles                                  m_eulerAngles;
            EE_REFLECT() ResourceID                                   m_resourceID;
        };

        enum class InternalEnum : uint8_t
        {
            EE_REFLECT_ENUM

            Moo = 54,// Sound cows make
             // Animal
            Cow = 75
        };

        struct InternalTest
        {
            enum class Enum : int16_t
            {
                EE_REFLECT_ENUM

                foo = -1,
                Bar
            };
        };

    public:

        TestComponent() = default;
        virtual void Initialize() override;

    protected:

        EE_REFLECT( "Category" : "Basic Types" );
        bool                                                                m_bool = true;

        EE_REFLECT( "Category" : "Basic Types" );
        uint8_t                                                             m_U8 = 8;

        EE_REFLECT( "Category" : "Basic Types" );
        uint16_t                                                            m_U16 = 16;

        EE_REFLECT( "Category" : "Basic Types" );
        uint32_t                                                            m_U32 = 32;

        EE_REFLECT( "Category" : "Basic Types" );
        uint64_t                                                            m_U64 = 64;

        EE_REFLECT( "Category" : "Basic Types" );
        int8_t                                                              m_S8 = -8;

        EE_REFLECT( "Category" : "Basic Types" );
        int16_t                                                             m_S16 = -16;

        EE_REFLECT( "Category" : "Basic Types" );
        int32_t                                                             m_S32 = -32;

        EE_REFLECT( "Category" : "Basic Types" );
        int64_t                                                             m_S64 = -64;

        EE_REFLECT( "Category" : "Basic Types" );
        float                                                               m_F32 = -343.23432432423f;

        EE_REFLECT( "Category" : "Basic Types" );
        double                                                              m_F64 = 343.23432432423;

        EE_REFLECT( "Category" : "IDs" );
        UUID                                                                m_UUID;

        EE_REFLECT( "Category" : "IDs" );
        StringID                                                            m_StringID = StringID( "Default ID" );

        EE_REFLECT();
        String                                                              m_String = "Default Test String";

        EE_REFLECT();
        Color                                                               m_Color = Colors::Pink;

        EE_REFLECT( "Category" : "Math Types" );
        Float2                                                              m_Float2 = Float2( 1.0f, 2.0f );

        EE_REFLECT( "Category" : "Math Types" );
        Float3                                                              m_Float3 = Float3( 1.0f, 2.0f, 3.0f );

        EE_REFLECT( "Category" : "Math Types" );
        Float4                                                              m_Float4 = Float4( 1.0f, 2.0f, 3.0f, 4.0f );
        EE_REFLECT( "Category" : "Math Types" );
        Vector                                                              m_vector = Vector( 1.0f, 2.0f, 3.0f, 4.0f );

        EE_REFLECT( "Category" : "Math Types" );
        Quaternion                                                          m_quaternion = Quaternion( AxisAngle( Vector::WorldRight, Degrees( 35 ) ) );

        EE_REFLECT( "Category" : "Math Types" );
        Matrix                                                              m_matrix;

        EE_REFLECT( "Category" : "Math Types" );
        Transform                                                           m_affineTransform;

        EE_REFLECT( "Category" : "Time Types" );
        Microseconds                                                        m_us = 0;

        EE_REFLECT( "Category" : "Time Types" );
        Milliseconds                                                        m_ms = 0;

        EE_REFLECT( "Category" : "Time Types" );
        Seconds                                                             m_s = 0;

        EE_REFLECT( "Category" : "Math Types" );
        Percentage                                                          m_percentage = Percentage( 1.0f );

        EE_REFLECT( "Category" : "Math Types" );
        Degrees                                                             m_degrees;

        EE_REFLECT( "Category" : "Math Types" );
        Radians                                                             m_radians;

        EE_REFLECT( "Category" : "Math Types" );
        EulerAngles                                                         m_eulerAngles = EulerAngles( 15, 25, 23 );

        EE_REFLECT( "Category" : "Math Types" );
        FloatCurve                                                          m_floatCurve;

        EE_REFLECT( "Category" : "Flags" );
        BitFlags                                                            m_genericFlags;
        EE_REFLECT( "Category" : "Flags" );
        TBitFlags<TestFlags>                                                m_specificFlags;

        EE_REFLECT( "Category" : "Resource Types" );
        ResourcePath                                                        m_resourcePath = ResourcePath( "data://Default.txt" );

        EE_REFLECT( "Category" : "Resource Types" );
        ResourceTypeID                                                      m_resourceTypeID;

        EE_REFLECT( "Category" : "Resource Types" );
        ResourceID                                                          m_resourceID;

        EE_REFLECT( "Category" : "Resource Types" );
        TResourcePtr<EntityModel::SerializedEntityCollection>               m_specificResourcePtr;

        EE_REFLECT( "Category" : "Ranges" );
        IntRange                                                            m_intRange;

        EE_REFLECT( "Category" : "Ranges" );
        FloatRange                                                          m_floatRange;

        // Tags
        EE_REFLECT( "Category" : "Tags" );
        Tag                                                                 m_tag;

        EE_REFLECT( "Category" : "Tags" );
        TVector<Tag>                                                        m_tags;

        // Meta
        //-------------------------------------------------------------------------

        EE_REFLECT( "Category" : "Meta", "IsToolsReadOnly" : true );
        float                                                               m_float5 = 5;

        EE_REFLECT( "Category" : "Meta" );
        float                                                               m_float10 = 10;

        // Enums
        //-------------------------------------------------------------------------

        EE_REFLECT( "Category" : "Enums" );
        InternalEnum                                                        m_internalEnum = InternalEnum::Cow;

        EE_REFLECT( "Category" : "Enums" );
        InternalTest::Enum                                                  m_testInternalEnum = InternalTest::Enum::Bar;

        // Structure Types
        //-------------------------------------------------------------------------

        // This is struct0
        EE_REFLECT( "Category" : "Structs" );
        ExternalTestStruct                                                  m_struct0;

        // This is struct1
        EE_REFLECT( "Category" : "Structs" );
        InternalStruct                                                      m_struct1;

        // Collision Settings
        EE_REFLECT( "Category" : "Custom Editors" ) // More Comments
        Physics::CollisionSettings                                          m_settings; // Even More!!!

        // Arrays
        //-------------------------------------------------------------------------

        EE_REFLECT( "Category" : "Arrays" );
        float                                                               m_staticArray[4];

        EE_REFLECT( "Category" : "Arrays" );
        StringID                                                            m_staticArrayOfStringIDs[4] = { StringID( "A" ), StringID( "B" ), StringID( "C" ), StringID( "D" ) };

        EE_REFLECT( "Category" : "Arrays" );
        InternalStruct                                                      m_staticArrayOfStructs[2];

        EE_REFLECT( "Category" : "Arrays" );
        InternalTest::Enum                                                  m_staticArrayOfEnums[6];

        EE_REFLECT( "Category" : "Arrays" );
        TVector<float>                                                      m_dynamicArray;

        EE_REFLECT( "Category" : "Arrays" );
        TVector<ExternalTestStruct>                                         m_dynamicArrayOfStructs = { ExternalTestStruct(), ExternalTestStruct(), ExternalTestStruct() };

        // Tools
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        EE_REFLECT();
        TResourcePtr<EntityModel::SerializedEntityCollection>               m_devOnlyResource;

        EE_REFLECT();
        TVector<TResourcePtr<EntityModel::SerializedEntityCollection>>      m_devOnlyResourcePtrs;

        EE_REFLECT();
        float                                                               m_devOnlyProperty;

        EE_REFLECT();
        TVector<ExternalTestStruct>                                         m_devOnlyDynamicArrayOfStructs = { ExternalTestStruct(), ExternalTestStruct(), ExternalTestStruct() };
        #endif
    };
}