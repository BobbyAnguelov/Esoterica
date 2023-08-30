#pragma once

#include "Base/Esoterica.h"
#include "Base/Serialization/BinarySerialization.h"
#include <type_traits>

//-------------------------------------------------------------------------
//  Bit Flags
//-------------------------------------------------------------------------
// Generic flag flags type

namespace EE
{
    class BitFlags
    {
        EE_SERIALIZE( m_flags );

    public:

        constexpr static uint8_t const MaxFlags = 32;
        EE_FORCE_INLINE static uint32_t GetFlagMask( uint8_t flag ) { return (uint32_t) ( 1u << flag ); }

    public:

        inline BitFlags() = default;
        inline explicit BitFlags( uint32_t flags ) : m_flags( flags ) {}

        //-------------------------------------------------------------------------

        EE_FORCE_INLINE uint32_t Get() const { return m_flags; }
        EE_FORCE_INLINE void Set( uint32_t flags ) { m_flags = flags; }
        inline operator uint32_t() const { return m_flags; }

        EE_FORCE_INLINE bool IsAnyFlagSet() const { return m_flags != 0; }

        //-------------------------------------------------------------------------

        EE_FORCE_INLINE bool IsFlagSet( uint8_t flag ) const
        {
            EE_ASSERT( flag < MaxFlags );
            return ( m_flags & GetFlagMask( flag ) ) > 0;
        }

        template<typename T, typename = std::enable_if_t<std::is_enum<T>::value>>
        EE_FORCE_INLINE bool IsFlagSet( T enumValue )
        {
            return IsFlagSet( (uint8_t) enumValue );
        }

        EE_FORCE_INLINE void SetFlag( uint8_t flag )
        {
            EE_ASSERT( flag >= 0 && flag < MaxFlags );
            m_flags |= GetFlagMask( flag );
        }

        template<typename T, typename = std::enable_if_t<std::is_enum<T>::value>>
        EE_FORCE_INLINE void SetFlag( T enumValue )
        {
            SetFlag( (uint8_t)enumValue );
        }

        EE_FORCE_INLINE void SetFlag( uint8_t flag, bool value )
        {
            EE_ASSERT( flag < MaxFlags );
            value ? SetFlag( flag ) : ClearFlag( flag );
        }

        template<typename T, typename = std::enable_if_t<std::is_enum<T>::value>>
        EE_FORCE_INLINE void SetFlag( T enumValue, bool value )
        {
            SetFlag( (uint8_t)enumValue, value );
        }

        EE_FORCE_INLINE void SetAllFlags()
        {
            m_flags = 0xFFFFFFFF;
        }

        //-------------------------------------------------------------------------

        EE_FORCE_INLINE bool IsFlagCleared( uint8_t flag ) const
        {
            EE_ASSERT( flag < MaxFlags );
            return ( m_flags & GetFlagMask( flag ) ) == 0;
        }

        template<typename T, typename = std::enable_if_t<std::is_enum<T>::value>>
        EE_FORCE_INLINE bool IsFlagCleared( T enumValue )
        {
            return IsFlagCleared( (uint8_t)enumValue );
        }

        EE_FORCE_INLINE void ClearFlag( uint8_t flag )
        {
            EE_ASSERT( flag < MaxFlags );
            m_flags &= ~GetFlagMask( flag );
        }

        template<typename T>
        EE_FORCE_INLINE void ClearFlag( T enumValue )
        {
            ClearFlag( (uint8_t)enumValue );
        }

        EE_FORCE_INLINE void ClearAllFlags()
        {
            m_flags = 0;
        }

        //-------------------------------------------------------------------------

        EE_FORCE_INLINE void FlipFlag( uint8_t flag )
        {
            EE_ASSERT( flag >= 0 && flag < MaxFlags );
            m_flags ^= GetFlagMask( flag );
        }

        template<typename T, typename = std::enable_if_t<std::is_enum<T>::value>>
        EE_FORCE_INLINE void FlipFlag( T enumValue )
        {
            FlipFlag( (uint8_t)enumValue );
        }


        EE_FORCE_INLINE void FlipAllFlags()
        {
            m_flags = ~m_flags;
        }

        //-------------------------------------------------------------------------

        EE_FORCE_INLINE BitFlags& operator| ( uint8_t flag )
        {
            EE_ASSERT( flag < MaxFlags );
            m_flags |= GetFlagMask( flag );
            return *this;
        }

        EE_FORCE_INLINE BitFlags& operator& ( uint8_t flag )
        {
            EE_ASSERT( flag < MaxFlags );
            m_flags &= GetFlagMask( flag );
            return *this;
        }

    protected:

        uint32_t m_flags = 0;
    };
}

//-------------------------------------------------------------------------
//  Templatized Bit Flags
//-------------------------------------------------------------------------
// Helper to create flag flags variables from a specific enum type

namespace EE
{
    template<typename T>
    class TBitFlags : public BitFlags
    {
        static_assert( std::is_enum<T>::value, "TBitFlags only supports enum types" );

    public:

        using BitFlags::BitFlags;

        inline explicit TBitFlags( T value ) 
            : BitFlags( GetFlagMask( (uint8_t) value ) )
        {
            EE_ASSERT( (uint32_t) value < MaxFlags );
        }

        inline TBitFlags( TBitFlags<T> const& flags )
            : BitFlags( flags.m_flags )
        {}

        template<typename... Args, class Enable = std::enable_if_t<( ... && std::is_convertible_v<Args, T> )>>
        TBitFlags( Args&&... args )
        {
            ( ( m_flags |= 1u << (uint8_t) args ), ... );
        }

        TBitFlags& operator=( TBitFlags const& rhs ) = default;

        //-------------------------------------------------------------------------

        EE_FORCE_INLINE bool IsFlagSet( T flag ) const { return BitFlags::IsFlagSet( (uint8_t) flag ); }
        EE_FORCE_INLINE bool IsFlagCleared( T flag ) const { return BitFlags::IsFlagCleared( (uint8_t) flag ); }
        EE_FORCE_INLINE void SetFlag( T flag ) { BitFlags::SetFlag( (uint8_t) flag ); }
        EE_FORCE_INLINE void SetFlag( T flag, bool value ) { BitFlags::SetFlag( (uint8_t) flag, value ); }
        EE_FORCE_INLINE void FlipFlag( T flag ) { BitFlags::FlipFlag( (uint8_t) flag ); }
        EE_FORCE_INLINE void ClearFlag( T flag ) { BitFlags::ClearFlag( (uint8_t) flag ); }

        //-------------------------------------------------------------------------

        template<typename... Args>
        inline void SetMultipleFlags( Args&&... args )
        {
            ( ( m_flags |= 1u << (uint8_t) args ), ... );
        }

        template<typename... Args>
        inline bool AreAnyFlagsSet( Args&&... args ) const
        {
            uint32_t mask = 0;
            ( ( mask |= 1u << (uint8_t) args ), ... );
            return ( m_flags & mask ) != 0;
        }

        //-------------------------------------------------------------------------

        EE_FORCE_INLINE TBitFlags& operator| ( T flag )
        {
            EE_ASSERT( (uint8_t) flag < MaxFlags );
            m_flags |= GetFlagMask( flag );
            return *this;
        }

        EE_FORCE_INLINE TBitFlags& operator& ( T flag )
        {
            EE_ASSERT( (uint8_t) flag < MaxFlags );
            m_flags &= GetFlagMask( flag );
            return *this;
        }
    };

    //-------------------------------------------------------------------------

    static_assert( sizeof( TBitFlags<enum class Temp> ) == sizeof( BitFlags ), "TBitFlags is purely syntactic sugar for easy conversion of enums to flags. It must not contain any members!" );
}
