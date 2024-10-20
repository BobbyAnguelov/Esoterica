#pragma once

#include "EngineTools/_Module/API.h"
#include "Engine/Entity/EntityDescriptors.h"
#include "Engine/Entity/EntityComponent.h"
#include "Engine/Physics/PhysicsCollision.h"
#include "Base/Types/Color.h"
#include "Base/TypeSystem/ReflectedType.h"
#include "Base/TypeSystem/TypeInstance.h"
#include "Base/Math/Transform.h"
#include "Base/Resource/ResourcePtr.h"
#include "Base/Time/Time.h"
#include "Base/Types/Percentage.h"
#include "Base/Math/NumericRange.h"
#include "Base/Math/FloatCurve.h"
#include "Base/Types/Tag.h"
#include "Base/Render/RenderShader.h"
#include "EngineTools/Physics/ResourceDescriptors/ResourceDescriptor_PhysicsMaterialDatabase.h"

//-------------------------------------------------------------------------

namespace EE::SerializationTest
{
    struct EE_ENGINETOOLS_API SimpleStruct : public IReflectedType
    {
        EE_REFLECT_TYPE( SimpleStruct );

        EE_REFLECT();
        bool                                        m_bool = false;

        EE_REFLECT();
        TVector<float>                              m_dynamicArray = { 1.0f, 2.0f, 3.0f };
    };

    struct EE_ENGINETOOLS_API BaseStruct : public IReflectedType
    {
        EE_REFLECT_TYPE( BaseStruct );

        EE_REFLECT();
        TVector<float>                              m_floats = { 0.3f, 5.0f, 7.0f };

        EE_REFLECT();
        TVector<SimpleStruct>                       m_dynamicArray = { SimpleStruct(), SimpleStruct() };
    };

    struct EE_ENGINETOOLS_API DerivedStruct : public BaseStruct
    {
        EE_REFLECT_TYPE( DerivedStruct );

        EE_REFLECT();
        int32_t                                     m_extraInt = 88;
    };

    struct EE_ENGINETOOLS_API ComplexStruct : public IReflectedType
    {
        EE_REFLECT_TYPE( ComplexStruct );

        EE_REFLECT( Category = "A", ReadOnly, Description = "This is a uint8" )
        uint8_t                                      m_uint8 = 8;

        // Ensure that we dont reflect the wrong things
        EE_REFLECT( Category = "B", Description = "This is a uint16" ) uint16_t m_uint16 = 16;
        uint16_t                                    m_shouldNotBeReflected;

        EE_REFLECT( Category = "C", ReadOnly, Description = "This is a uint32" )
        uint32_t                                    m_uint32 = 32;

        EE_REFLECT();
        UUID                                        m_UUID;

        EE_REFLECT( Category = "C", FriendlyName ="Blah", Description = "This is a uint64");
        uint64_t                                    m_U64 = 64;

        // Comment description "with quotes"
        EE_REFLECT();
        EulerAngles                                 m_eulerAngles = EulerAngles( 23, 45, 56 );

        // Comment description "with quotes" expected to be overwritten
        EE_REFLECT( ReadOnly     , Category =   "C",   Description = "meta description 'with quotes' and escape char" );
        TVector<DerivedStruct>                      m_dynamicArray = { DerivedStruct(), DerivedStruct() };
    };

    struct EE_ENGINETOOLS_API TypeInstanceContainerStruct : public IReflectedType
    {
        EE_REFLECT_TYPE( TypeInstanceContainerStruct );

        EE_REFLECT();
        TVector<TypeInstance>       m_types;

        EE_REFLECT();
        TTypeInstance<BaseStruct>   m_specificType;
    };

    enum class FlagEnum
    {
        EE_REFLECT_ENUM

        a = 1,
        B = 2,
        c = 3,
        Duplicate = 5,
        D = 4,
        A = 5,
    };

    // Dev Only Type
    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    struct DevOnlyStruct : public IReflectedType
    {
        EE_REFLECT_TYPE( DevOnlyStruct );

        EE_REFLECT() float m_float;
    };

    enum class DevOnlyEnum
    {
        EE_REFLECT_ENUM

        Moo, // Sound cows make
        // Animal
        Cow
    };
    #endif

    // Component
    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API TestComponent : public EntityComponent
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

        struct InternalEnumStructWrapper
        {
            enum class Enum : int16_t
            {
                EE_REFLECT_ENUM

                foo = -1,
                Bar
            };
        };

    public:

        TestComponent();
        virtual void Initialize() override;

    protected:

        // Core Types
        //-------------------------------------------------------------------------

        EE_REFLECT( Category = "Core Types" );
        bool                                                                m_bool = true;

        EE_REFLECT( Category = "Core Types" );
        uint8_t                                                             m_U8 = 8;

        EE_REFLECT( Category = "Core Types" );
        uint16_t                                                            m_U16 = 16;

        EE_REFLECT( Category = "Core Types" );
        uint32_t                                                            m_U32 = 32;

        EE_REFLECT( Category = "Core Types" );
        uint64_t                                                            m_U64 = 64;

        EE_REFLECT( Category = "Core Types" );
        int8_t                                                              m_S8 = -8;

        EE_REFLECT( Category = "Core Types" );
        int16_t                                                             m_S16 = -16;

        EE_REFLECT( Category = "Core Types" );
        int32_t                                                             m_S32 = -32;

        EE_REFLECT( Category = "Core Types" );
        int64_t                                                             m_S64 = -64;

        EE_REFLECT( Category = "Core Types" );
        float                                                               m_F32 = -343.23432432423f;

        EE_REFLECT( Category = "Core Types" );
        double                                                              m_F64 = 343.23432432423;

        EE_REFLECT( Category = "Core Types" );
        String                                                              m_string = "Default Test String";

        EE_REFLECT( Category = "Core Types" );
        Color                                                               m_color = Colors::Pink;

        // IDs
        //-------------------------------------------------------------------------

        EE_REFLECT( Category = "IDs" );
        UUID                                                                m_UUID;

        EE_REFLECT( Category = "IDs" );
        StringID                                                            m_stringID = StringID( "Default ID" );

        // Math
        //-------------------------------------------------------------------------

        EE_REFLECT( Category = "Math Types" );
        Float2                                                              m_float2 = Float2( 1.0f, 2.0f );

        EE_REFLECT( Category = "Math Types" );
        Float3                                                              m_float3 = Float3( 1.0f, 2.0f, 3.0f );

        EE_REFLECT( Category = "Math Types" );
        Float4                                                              m_float4 = Float4( 1.0f, 2.0f, 3.0f, 4.0f );

        EE_REFLECT( Category = "Math Types" );
        Vector                                                              m_vector = Vector( 1.0f, 2.0f, 3.0f, 4.0f );

        EE_REFLECT( Category = "Math Types" );
        Quaternion                                                          m_quaternion = Quaternion( AxisAngle( Vector::WorldRight, Degrees( 35 ) ) );

        EE_REFLECT( Category = "Math Types" );
        Matrix                                                              m_matrix;

        EE_REFLECT( Category = "Math Types" );
        Transform                                                           m_affineTransform;

        EE_REFLECT( Category = "Math Types" );
        Microseconds                                                        m_microseconds = 0;

        EE_REFLECT( Category = "Math Types" );
        Milliseconds                                                        m_milliseconds = 0;

        EE_REFLECT( Category = "Math Types" );
        Seconds                                                             m_seconds = 0;

        EE_REFLECT( Category = "Math Types" );
        Percentage                                                          m_percentage = Percentage( 1.0f );

        EE_REFLECT( Category = "Math Types" );
        Degrees                                                             m_degrees;

        EE_REFLECT( Category = "Math Types" );
        Radians                                                             m_radians;

        EE_REFLECT( Category = "Math Types" );
        EulerAngles                                                         m_eulerAngles = EulerAngles( 15, 25, 23 );

        EE_REFLECT( Category = "Math Types" );
        FloatCurve                                                          m_floatCurve;

        // Enums and Flags
        //-------------------------------------------------------------------------

        EE_REFLECT( Category = "Enums & Flags" );
        InternalEnum                                                        m_internalEnum = InternalEnum::Cow;

        EE_REFLECT( Category = "Enums & Flags" );
        InternalEnumStructWrapper::Enum                                     m_testInternalEnum = InternalEnumStructWrapper::Enum::Bar;

        EE_REFLECT( Category = "Enums & Flags" );
        BitFlags                                                            m_genericFlags;

        EE_REFLECT( Category = "Enums & Flags" );
        TBitFlags<FlagEnum>                                                 m_specificFlags;

        // Path
        //-------------------------------------------------------------------------

        EE_REFLECT( Category = "Data Paths" );
        DataPath                                                            m_dataPath = DataPath( "data://Default.txt" );

        EE_REFLECT( Category = "Data Paths" );
        TDataFilePath<Physics::PhysicsMaterialLibrary>                      m_dataFilePath = DataPath( "data://Default.pml" );

        // Resource
        //-------------------------------------------------------------------------

        EE_REFLECT( Category = "Resource Types" );
        ResourceTypeID                                                      m_resourceTypeID;

        EE_REFLECT( Category = "Resource Types" );
        ResourceID                                                          m_resourceID;

        EE_REFLECT( Category = "Resource Types" );
        TResourcePtr<EntityModel::EntityCollection>                         m_specificResourcePtr;

        // Tags
        //-------------------------------------------------------------------------

        EE_REFLECT( Category = "Ranges" );
        IntRange                                                            m_intRange;

        EE_REFLECT( Category = "Ranges" );
        FloatRange                                                          m_floatRange;

        // Tags
        //-------------------------------------------------------------------------

        EE_REFLECT( Category = "Tags" );
        Tag2                                                                m_tag2;

        EE_REFLECT( Category = "Tags" );
        Tag4                                                                m_tag4;

        EE_REFLECT( Category = "Tags" );
        TVector<Tag4>                                                       m_tags;

        // Structure Types
        //-------------------------------------------------------------------------

        // This is a simple struct
        EE_REFLECT( Category = "Structs" );
        SimpleStruct                                                        m_simpleStruct;

        // This is a complex struct
        EE_REFLECT( Category = "Structs" );
        ComplexStruct                                                       m_complexStruct;

        // This is an internal
        EE_REFLECT( Category = "Structs" );
        InternalStruct                                                      m_internalStruct;

        // Collision Settings (comment)
        EE_REFLECT( Category = "Custom Editors" ) // More Comments
        Physics::CollisionSettings                                          m_settings; // Even More!!!

        // Arrays
        //-------------------------------------------------------------------------

        EE_REFLECT( Category = "Arrays" );
        float                                                               m_staticArray[4];

        EE_REFLECT( Category = "Arrays" );
        StringID                                                            m_staticArrayOfStringIDs[4] = { StringID( "A" ), StringID( "B" ), StringID( "C" ), StringID( "D" ) };

        EE_REFLECT( Category = "Arrays" );
        InternalStruct                                                      m_staticArrayOfStructs[2];

        EE_REFLECT( Category = "Arrays" );
        InternalEnumStructWrapper::Enum                                     m_staticArrayOfEnums[6];

        EE_REFLECT( Category = "Arrays" );
        TVector<float>                                                      m_dynamicArray;

        EE_REFLECT( Category = "Arrays" );
        TInlineVector<float, 10>                                            m_inlineDynamicArray;

        EE_REFLECT( Category = "Arrays" );
        TVector<ComplexStruct>                                              m_dynamicArrayOfStructs = { ComplexStruct(), ComplexStruct(), ComplexStruct() };

        EE_REFLECT( Category = "Arrays" );
        TInlineVector<DerivedStruct, 5>                                     m_inlineDynamicArrayOfStructs = { DerivedStruct(), DerivedStruct(), DerivedStruct() };

        // Type Instances
        //-------------------------------------------------------------------------

        EE_REFLECT( Category = "Instances" );
        TypeInstance                                                        m_typeInstance;

        EE_REFLECT( Category = "Instances" );
        TTypeInstance<BaseStruct>                                           m_derivedTypeInstance;

        EE_REFLECT( Category = "Instances" );
        TVector<TypeInstance>                                               m_typeInstanceArray;

        // Tools Only
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        EE_REFLECT();
        TResourcePtr<EntityModel::EntityCollection>                         m_devOnlyResource;

        EE_REFLECT();
        TVector<TResourcePtr<EntityModel::EntityCollection>>                m_devOnlyResourcePtrs;

        EE_REFLECT();
        float                                                               m_devOnlyProperty;

        EE_REFLECT();
        TVector<ComplexStruct>                                              m_devOnlyDynamicArrayOfStructs = { ComplexStruct(), ComplexStruct(), ComplexStruct() };
        #endif

        // Meta / Clamps
        //-------------------------------------------------------------------------

        // User defined custom meta flags
        EE_REFLECT( Category = "Meta", CustomMeta = "moo", SizeLimit = 0.4f, SizeUpperLimit = 23 );
        bool                                                                m_userDefinedMetaFlags = false;

        EE_REFLECT( Category = "Meta", ReadOnly );
        bool                                                                m_readOnlyValue = false;

        EE_REFLECT( Category = "Meta", ShowAsStaticArray );
        TVector<int32_t>                                                    m_arrayWithNoSizeControls = { 1, 2, 3, 4, 5 };

        EE_REFLECT( Category = "Meta", DisableTypePicker );
        TTypeInstance<BaseStruct>                                           m_codeOnlyTypeInstance;

        EE_REFLECT( Category = "Meta", Min = -10, Max = 10 );
        int16_t                                                             m_clampedInt = 0;

        EE_REFLECT( Category = "Meta", Min = -100 );
        int32_t                                                             m_onlyMinClampedInt = 0;

        EE_REFLECT( Category = "Meta", Max = 100 );
        int32_t                                                             m_onlyMaxClampedInt = 0;

        // Bad min value on clamp is intentional to test grid editor
        EE_REFLECT( Category = "Meta", Min = -10, Max = 10 );
        uint16_t                                                            m_clampedUInt = 0;

        EE_REFLECT( Category = "Meta", Min = 24000, Max = 64000 );
        uint64_t                                                            m_clampedUint64 = 0;

        EE_REFLECT( Category = "Meta", Min = 0.0f, Max = 1.0f );
        float                                                               m_clampedFloat = 0;

        EE_REFLECT( Category = "Meta", Min = 0.0f );
        float                                                               m_onlyMinClampedFloat = 0;

        EE_REFLECT( Category = "Meta", Max = 1.0f );
        float                                                               m_onlyMaxClampedFloat = 0;
    };
}