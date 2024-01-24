#pragma once

#include "Base/Math/Math.h"
#include "Base/Types/StringID.h"
#include "Base/Types/Function.h"
#include "Base/Types/Containers_ForwardDecl.h"

//-------------------------------------------------------------------------

namespace EE::Input
{
    class InputDevice;

    //-------------------------------------------------------------------------

    class VirtualInput
    {
    public:

        enum class Type
        {
            Binary,
            Analog,
            Directional
        };

    public:

        VirtualInput( StringID ID ) : m_ID( ID ) {}
        virtual ~VirtualInput() = default;

        inline StringID GetID() const { return m_ID; }

        virtual Type GetType() const = 0;

        virtual void Update( TInlineVector<InputDevice const*, 5> const& connectedInputDevices ) = 0;

    protected:

        StringID m_ID;
    };

    //-------------------------------------------------------------------------

    class BinaryInput : public VirtualInput
    {
    public:

        using UpdateFunction = TFunction<bool( TInlineVector<InputDevice const*, 5> const& )>;

    public:

        BinaryInput( StringID ID, UpdateFunction&& updateFunc )
            : VirtualInput( ID )
            , m_updateFunction( updateFunc )
        {}

        inline bool GetValue() const { return m_value; }

        virtual Type GetType() const override { return VirtualInput::Type::Binary; }

        virtual void Update( TInlineVector<InputDevice const*, 5> const& connectedInputDevices ) override { m_value = m_updateFunction( connectedInputDevices ); }

    private:

        UpdateFunction      m_updateFunction;
        bool                m_value;
    };

    //-------------------------------------------------------------------------

    class AnalogInput : public VirtualInput
    {
    public:

        using UpdateFunction = TFunction<float( TInlineVector<InputDevice const*, 5> const& )>;

    public:

        AnalogInput( StringID ID, UpdateFunction&& updateFunc )
            : VirtualInput( ID )
            , m_updateFunction( updateFunc )
        {}

        inline float GetValue() const { return m_value; }

        virtual Type GetType() const override { return VirtualInput::Type::Binary; }

        virtual void Update( TInlineVector<InputDevice const*, 5> const& connectedInputDevices ) override { m_value = m_updateFunction( connectedInputDevices ); }

    private:

        UpdateFunction      m_updateFunction;
        float               m_value;
    };

    //-------------------------------------------------------------------------

    class DirectionalInput : public VirtualInput
    {
    public:

        using UpdateFunction = TFunction<Float2( TInlineVector<InputDevice const*, 5> const& )>;

    public:

        DirectionalInput( StringID ID, UpdateFunction&& updateFunc )
            : VirtualInput( ID )
            , m_updateFunction( updateFunc )
        {}

        inline Float2 GetValue() const { return m_value; }

        virtual Type GetType() const override { return VirtualInput::Type::Binary; }

        virtual void Update( TInlineVector<InputDevice const*, 5> const& connectedInputDevices ) override { m_value = m_updateFunction( connectedInputDevices ); }

    private:

        UpdateFunction      m_updateFunction;
        Float2              m_value;
    };
}