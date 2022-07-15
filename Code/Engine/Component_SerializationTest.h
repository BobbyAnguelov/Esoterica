#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Entity/EntityDescriptors.h"
#include "Engine/Entity/EntityComponent.h"
#include "System/Types/Color.h"
#include "System/TypeSystem/RegisteredType.h"
#include "System/Math/Transform.h"
#include "System/Resource/ResourcePtr.h"
#include "System/Time/Time.h"
#include "System/Types/Percentage.h"
#include "System/Math/NumericRange.h"
#include "System/Math/FloatCurve.h"
#include "System/Types/Tag.h"

//-------------------------------------------------------------------------

namespace EE
{
    struct EE_ENGINE_API ExternalTestSubSubStruct : public IRegisteredType
    {
        EE_REGISTER_TYPE( ExternalTestSubSubStruct );

        EE_EXPOSE TVector<float>                               m_dynamicArray = { 1.0f, 2.0f, 3.0f };
    };

    struct EE_ENGINE_API ExternalTestSubStruct : public IRegisteredType
    {
        EE_REGISTER_TYPE( ExternalTestSubStruct );

        EE_EXPOSE TVector<float>                               m_floats = { 0.3f, 5.0f, 7.0f };
        EE_EXPOSE TVector<ExternalTestSubSubStruct>            m_dynamicArray = { ExternalTestSubSubStruct(), ExternalTestSubSubStruct() };
    };

    struct EE_ENGINE_API ExternalTestStruct : public IRegisteredType
    {
        EE_REGISTER_TYPE( ExternalTestStruct );

        EE_EXPOSE uint8_t                                        m_uint8 = 8;
        EE_EXPOSE uint16_t                                       m_uint16 = 16;
        EE_EXPOSE uint32_t                                       m_uint32 = 32;
        EE_EXPOSE uint64_t                                       m_U64 = 64;
        EE_EXPOSE UUID                                         m_UUID;
        EE_EXPOSE EulerAngles                                  m_eulerAngles = EulerAngles( 23, 45, 56 );
        EE_EXPOSE TVector<ExternalTestSubStruct>               m_dynamicArray = { ExternalTestSubStruct(), ExternalTestSubStruct() };
    };

    #if EE_DEVELOPMENT_TOOLS
    struct DevOnlyStruct : public IRegisteredType
    {
        EE_REGISTER_TYPE( DevOnlyStruct );

        EE_EXPOSE float m_float;
    };
    #endif

    //-------------------------------------------------------------------------

    enum class TestFlags
    {
        EE_REGISTER_ENUM

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
        EE_REGISTER_ENUM

        Moo, // Sound cows make
        // Animal
        Cow
    };
    #endif

    //-------------------------------------------------------------------------

    class EE_ENGINE_API TestComponent : public EntityComponent
    {
        EE_REGISTER_ENTITY_COMPONENT( TestComponent );

    public:

        struct InternalStruct : public IRegisteredType
        {
            EE_REGISTER_TYPE( InternalStruct );

            EE_EXPOSE EulerAngles                                  m_eulerAngles;
            EE_EXPOSE ResourceID                                   m_resourceID;
        };

        enum class InternalEnum : uint8_t
        {
            EE_REGISTER_ENUM

            Moo = 54,// Sound cows make
             // Animal
            Cow = 75
        };

        struct InternalTest
        {
            enum class Enum : int16_t
            {
                EE_REGISTER_ENUM

                foo = -1,
                Bar
            };
        };

    public:

        TestComponent() = default;
        virtual void Initialize() override;

    protected:

        EE_EXPOSE bool                                                         m_bool = true;
        EE_EXPOSE uint8_t                                                        m_U8 = 8;
        EE_EXPOSE uint16_t                                                       m_U16 = 16;
        EE_EXPOSE uint32_t                                                       m_U32 = 32;
        EE_EXPOSE uint64_t                                                       m_U64 = 64;
        EE_EXPOSE int8_t                                                         m_S8 = -8;
        EE_EXPOSE int16_t                                                        m_S16 = -16;
        EE_EXPOSE int32_t                                                        m_S32 = -32;
        EE_EXPOSE int64_t                                                        m_S64 = -64;
        EE_EXPOSE float                                                        m_F32 = -343.23432432423f;
        EE_EXPOSE double                                                       m_F64 = 343.23432432423;
        EE_EXPOSE UUID                                                         m_UUID;
        EE_EXPOSE StringID                                                     m_StringID = StringID( "Default ID" );
        EE_EXPOSE String                                                       m_String = "Default Test String";
        EE_EXPOSE Color                                                        m_Color = Colors::Pink;
        EE_EXPOSE Float2                                                       m_Float2 = Float2( 1.0f, 2.0f );
        EE_EXPOSE Float3                                                       m_Float3 = Float3( 1.0f, 2.0f, 3.0f );
        EE_EXPOSE Float4                                                       m_Float4 = Float4( 1.0f, 2.0f, 3.0f, 4.0f );
        EE_EXPOSE Vector                                                       m_vector = Vector( 1.0f, 2.0f, 3.0f, 4.0f );
        EE_EXPOSE Quaternion                                                   m_quaternion = Quaternion( AxisAngle( Vector::WorldRight, Degrees( 35 ) ) );
        EE_EXPOSE Matrix                                                       m_matrix;
        EE_EXPOSE Transform                                                    m_affineTransform;
        EE_EXPOSE Microseconds                                                 m_us = 0;
        EE_EXPOSE Milliseconds                                                 m_ms = 0;
        EE_EXPOSE Seconds                                                      m_s = 0;
        EE_EXPOSE Percentage                                                   m_percentage = Percentage( 1.0f );
        EE_EXPOSE Degrees                                                      m_degrees;
        EE_EXPOSE Radians                                                      m_radians;
        EE_EXPOSE EulerAngles                                                  m_eulerAngles = EulerAngles( 15, 25, 23 );
        EE_EXPOSE ResourcePath                                                 m_resourcePath = ResourcePath( "data://Default.txt" );
        EE_EXPOSE BitFlags                                                     m_genericFlags;
        EE_EXPOSE TBitFlags<TestFlags>                                         m_specificFlags;
        EE_EXPOSE ResourceTypeID                                               m_resourceTypeID;
        EE_EXPOSE ResourceID                                                   m_resourceID;
        EE_EXPOSE TResourcePtr<EntityModel::EntityCollectionDescriptor>        m_specificResourcePtr;

        EE_EXPOSE IntRange                                                     m_intRange;
        EE_EXPOSE FloatRange                                                   m_floatRange;
        EE_EXPOSE FloatCurve                                                   m_floatCurve;

        #if EE_DEVELOPMENT_TOOLS
        EE_EXPOSE TResourcePtr<EntityModel::EntityCollectionDescriptor>            m_devOnlyResource;
        EE_EXPOSE TVector<TResourcePtr<EntityModel::EntityCollectionDescriptor>>   m_devOnlyResourcePtrs;
        EE_EXPOSE float                                                            m_devOnlyProperty;
        EE_EXPOSE TVector<ExternalTestStruct>                                      m_devOnlyDynamicArrayOfStructs = { ExternalTestStruct(), ExternalTestStruct(), ExternalTestStruct() };
        #endif

        // Tags
        EE_EXPOSE Tag                                                          m_tag;
        EE_EXPOSE TVector<Tag>                                                 m_tags;

        // Enums
        EE_EXPOSE InternalEnum                                                 m_internalEnum = InternalEnum::Cow;
        EE_EXPOSE InternalTest::Enum                                           m_testInternalEnum = InternalTest::Enum::Bar;

        // Types
        EE_EXPOSE ExternalTestStruct                                           m_struct0;
        EE_EXPOSE InternalStruct                                               m_struct1;

        // Arrays
        EE_EXPOSE float                                                        m_staticArray[4];
        EE_EXPOSE StringID                                                     m_staticArrayOfStringIDs[4] = { StringID( "A" ), StringID( "B" ), StringID( "C" ), StringID( "D" ) };
        EE_EXPOSE InternalStruct                                               m_staticArrayOfStructs[2];
        EE_EXPOSE InternalTest::Enum                                           m_staticArrayOfEnums[6];
        EE_EXPOSE TVector<float>                                               m_dynamicArray;
        EE_EXPOSE TVector<ExternalTestStruct>                                  m_dynamicArrayOfStructs = { ExternalTestStruct(), ExternalTestStruct(), ExternalTestStruct() };
    };
}