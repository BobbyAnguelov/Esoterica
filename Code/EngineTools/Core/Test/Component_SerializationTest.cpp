#include "Component_SerializationTest.h"

//-------------------------------------------------------------------------

namespace EE::SerializationTest
{
    TestComponent::TestComponent()
    {
        m_codeOnlyTypeInstance.CreateInstance<DerivedStruct>();
    }

    void TestComponent::Initialize()
    {
        EntityComponent::Initialize();
    }
}