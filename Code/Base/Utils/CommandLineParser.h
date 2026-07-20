#pragma once
#include "Base/Types/String.h"
#include "Base/Types/Arrays.h"
#include "Base/Types/StringID.h"
#include <iostream>

//-------------------------------------------------------------------------

namespace EE
{
    class CommandLineParser
    {
        template<typename T>
        struct Arg
        {
            T const& GetValue() const { return m_wasParsed ? m_value : m_default; }

            inline bool operator==( StringID ID ) const { return m_ID == ID; }
            inline bool operator!=( StringID ID ) const { return m_ID != ID; }
            inline bool operator==( char const* pName ) const { return m_ID == StringID( pName ); }
            inline bool operator!=( char const* pName ) const { return m_ID == StringID( pName ); }

        public:

            StringID    m_ID;
            String      m_description;
            bool        m_isRequired = false;
            bool        m_wasParsed = false;
            T           m_default;
            T           m_value;
        };

    public:

        CommandLineParser() = default;

        bool Parse( int32_t argc, char *argv[] );

        char const* GetErrorMessage() const { return m_errorMsg.c_str(); }

        void Clear()
        {
            m_boolArgs.clear();
            m_intArgs.clear();
            m_floatArgs.clear();
            m_stringArgs.clear();
        }

        void PrintHelp() const;

        void AddRequiredBoolArg( char const* pName, char const* pDescription = nullptr ) { AddArg<bool>( m_boolArgs, pName, pDescription, false, true ); }
        void AddRequiredIntArg( char const* pName, char const* pDescription = nullptr, int64_t defaultValue = 0 ) { AddArg<int64_t>( m_intArgs, pName, pDescription, defaultValue, true ); }
        void AddRequiredFloatArg( char const* pName, char const* pDescription = nullptr, float defaultValue = 0.0f ) { AddArg<float>( m_floatArgs, pName, pDescription, defaultValue, true ); }
        void AddRequiredStringArg( char const* pName, char const* pDescription = nullptr, char const* pDefaultValue = nullptr ) { AddArg<String>( m_stringArgs, pName, pDescription, String( pDefaultValue ? pDefaultValue : "" ), true ); }

        void AddOptionalBoolArg( char const* pName, char const* pDescription = nullptr, bool defaultValue = false ) { AddArg<bool>( m_boolArgs, pName, pDescription, defaultValue, false ); }
        void AddOptionalIntArg( char const* pName, char const* pDescription = nullptr, int64_t defaultValue = 0 ) { AddArg<int64_t>( m_intArgs, pName, pDescription, defaultValue, false ); }
        void AddOptionalFloatArg( char const* pName, char const* pDescription = nullptr, float defaultValue = 0.0f ) { AddArg<float>( m_floatArgs, pName, pDescription, defaultValue, false ); }
        void AddOptionalStringArg( char const* pName, char const* pDescription = nullptr, char const* pDefaultValue = nullptr ) { AddArg<String>( m_stringArgs, pName, pDescription, String( pDefaultValue ? pDefaultValue : "" ), false ); }

        // Accessors
        //-------------------------------------------------------------------------

        bool GetBoolArg( char const* pName ) const
        {
            auto pArg = FindArg<bool>( m_boolArgs, pName );
            EE_ASSERT( pArg != nullptr );
            return pArg->GetValue();
        }

        int64_t GetIntArg( char const* pName ) const
        {
            auto pArg = FindArg<int64_t>( m_intArgs, pName );
            EE_ASSERT( pArg != nullptr );
            return pArg->GetValue();
        }

        float GetFloatArg( char const* pName ) const
        {
            auto pArg = FindArg<float>( m_floatArgs, pName );
            EE_ASSERT( pArg != nullptr );
            return pArg->GetValue();
        }

        String const& GetStringArg( char const* pName ) const
        {
            auto pArg = FindArg<String>( m_stringArgs, pName );
            EE_ASSERT( pArg != nullptr );
            return pArg->GetValue();
        }

        inline bool HasBoolArg( char const* pName ) const
        {
            auto pArg = FindArg<bool>( m_boolArgs, pName );
            return pArg != nullptr && pArg->m_wasParsed;
        }

        inline bool HasIntArg( char const* pName ) const
        {
            auto pArg = FindArg<int64_t>( m_intArgs, pName );
            return pArg != nullptr && pArg->m_wasParsed;
        }

        inline bool HasFloatArg( char const* pName ) const
        {
            auto pArg = FindArg<float>( m_floatArgs, pName );
            return pArg != nullptr && pArg->m_wasParsed;
        }

        inline bool HasStringArg( char const* pName ) const
        {
            auto pArg = FindArg<String>( m_stringArgs, pName );
            return pArg != nullptr && pArg->m_wasParsed;
        }

    private:

        inline StringID MakeArgID( char const* pName ) const
        {
            EE_ASSERT( pName != nullptr );
            InlineString lowercaseName( pName );
            lowercaseName.make_lower();
            StringID const ID( lowercaseName.c_str() );
            EE_ASSERT( ID.IsValid() );
            return ID;
        }

        template<typename T> 
        Arg<T> const* FindArg( TVector<Arg<T>> const& args, char const* pName ) const
        {
            StringID const ID = MakeArgID( pName );

            for ( auto const& arg : args )
            {
                if ( arg.m_ID == ID )
                {
                    return &arg;
                }
            }

            return nullptr;
        }

        inline bool IsUniqueArgID( StringID ID ) const
        {
            if ( VectorContains( m_boolArgs, ID ) ) return false;
            if ( VectorContains( m_intArgs, ID ) ) return false;
            if ( VectorContains( m_floatArgs, ID ) ) return false;
            if ( VectorContains( m_stringArgs, ID ) ) return false;

            return true;
        }

        template<typename T>
        Arg<T>& AddArg( TVector<Arg<T>>& args, char const* pName, char const* pDescription, T const& defaultValue, bool isRequired )
        {
            StringID const ID = MakeArgID( pName );
            EE_ASSERT( IsUniqueArgID( ID ) );

            Arg<T>& arg = args.emplace_back();
            arg.m_ID = ID;
            arg.m_description = pDescription != nullptr ? pDescription : "";
            arg.m_isRequired = isRequired;
            arg.m_wasParsed = false;
            arg.m_default = defaultValue;
            arg.m_value = defaultValue;

            return arg;
        }

        template<typename T>
        bool AreAllRequiredArgsParsed( TVector<Arg<T>>& args, TInlineVector<StringID, 10>* pOutMissingArgs = nullptr ) const
        {
            bool missingArgsFound = false;

            for ( auto const& arg : args )
            {
                if ( arg.m_isRequired && !arg.m_wasParsed )
                {
                    if ( pOutMissingArgs != nullptr )
                    {
                        pOutMissingArgs->emplace_back( arg.m_ID );
                    }

                    missingArgsFound = true;
                }
            }

            return !missingArgsFound;
        }

        bool TryReadValueStrForArg( int32_t argc, char *argv[], int32_t& i, InlineString& outStrValue ) const
        {
            if ( i < ( argc - 1 ) )
            {
                i++;
                outStrValue = argv[i];
                return true;
            }
            else
            {
                outStrValue.clear();
                m_errorMsg.sprintf( "Malformed command line args, missing value for arg: %s", argv[i][1] );
                return false;
            }
        }

        template<typename T>
        void PrintHelp( TVector<Arg<T>> const& args ) const
        {
            for ( auto const& arg : args )
            {
                if ( arg.m_isRequired )
                {
                    std::cout << "[Required] ";
                }
                else
                {
                    std::cout << "[Optional] ";
                }

                //-------------------------------------------------------------------------

                std::cout << "Name: " << arg.m_ID.c_str();

                //-------------------------------------------------------------------------

                std::cout << ", Description: '";

                if ( !arg.m_description.empty() )
                {
                    std::cout << arg.m_description.c_str();
                }
                else
                {
                    std::cout << "'";
                }

                //-------------------------------------------------------------------------

                std::cout << ", Default: " << arg.m_default << std::endl;
            }
        }

    private:

        TVector<Arg<bool>>      m_boolArgs;
        TVector<Arg<int64_t>>   m_intArgs;
        TVector<Arg<float>>     m_floatArgs;
        TVector<Arg<String>>    m_stringArgs;
        mutable String          m_errorMsg;
    };

    std::ostream& operator<<( std::ostream & s, String const& val )
    {
        if ( !val.empty() )
        {
            std::cout.write( val.c_str(), std::min<size_t>( val.size(), val.length() ) );
        }
        else
        {
            std::cout.write( "''", 2 );
        }

        return s;
    }

    //-------------------------------------------------------------------------

    inline bool CommandLineParser::Parse( int32_t argc, char *argv[] )
    {
        InlineString strName;
        InlineString strValue;

        for ( int32_t i = 0; i < argc; i++ )
        {
            strName = argv[i];
            if ( strName.size() > 1 && strName[0] == '-' )
            {
                StringID const ID = MakeArgID( &strName[1] );

                //-------------------------------------------------------------------------

                int32_t const boolArgIdx = VectorFindIndex( m_boolArgs, ID );
                if ( boolArgIdx != InvalidIndex )
                {
                    m_boolArgs[boolArgIdx].m_value = true;
                    m_boolArgs[boolArgIdx].m_wasParsed = true;
                    continue;
                }

                //-------------------------------------------------------------------------

                int32_t const intArgIdx = VectorFindIndex( m_intArgs, ID );
                if ( intArgIdx != InvalidIndex )
                {
                    if ( !TryReadValueStrForArg( argc, argv, i, strValue ) )
                    {
                        return false;
                    }

                    errno = 0;
                    char* pEndPtr = nullptr;
                    m_intArgs[intArgIdx].m_value = std::strtol( strValue.c_str(), &pEndPtr, 0 );
                    m_intArgs[intArgIdx].m_wasParsed = true;

                    if ( errno != 0 || pEndPtr != strValue.end() )
                    {
                        m_errorMsg.sprintf( "Parsing int argument (%s) failed, supplied string: %s", ID.c_str(), strValue.c_str() );
                        return false;
                    }

                    continue;
                }

                //-------------------------------------------------------------------------

                int32_t const floatArgIdx = VectorFindIndex( m_floatArgs, ID );
                if ( floatArgIdx != InvalidIndex )
                {
                    if ( !TryReadValueStrForArg( argc, argv, i, strValue ) )
                    {
                        return false;
                    }

                    errno = 0;
                    char* pEndPtr = nullptr;
                    m_floatArgs[floatArgIdx].m_value = std::strtof( strValue.c_str(), &pEndPtr );
                    m_floatArgs[floatArgIdx].m_wasParsed = true;

                    if ( errno != 0 || pEndPtr != strValue.end() )
                    {
                        m_errorMsg.sprintf( "Parsing int argument (%s) failed, supplied string: %s", ID.c_str(), strValue.c_str() );
                        return false;
                    }

                    continue;
                }

                //-------------------------------------------------------------------------

                int32_t const stringArgIdx = VectorFindIndex( m_stringArgs, ID );
                if ( stringArgIdx != InvalidIndex )
                {
                    if ( !TryReadValueStrForArg( argc, argv, i, strValue ) )
                    {
                        return false;
                    }

                    m_stringArgs[stringArgIdx].m_value = strValue.c_str();
                    m_stringArgs[stringArgIdx].m_wasParsed = true;
                    continue;
                }
            }
        }

        //-------------------------------------------------------------------------

        TInlineVector<StringID, 10> missingArgs;
        bool succeeded = AreAllRequiredArgsParsed( m_boolArgs, &missingArgs );
        succeeded &= AreAllRequiredArgsParsed( m_intArgs, &missingArgs );
        succeeded &= AreAllRequiredArgsParsed( m_floatArgs, &missingArgs );
        succeeded &= AreAllRequiredArgsParsed( m_stringArgs, &missingArgs );

        if ( !succeeded )
        {
            m_errorMsg.sprintf( "Not all required arguments set, missing args: " );

            for ( int32_t i = 0; i < missingArgs.size(); i++ )
            {
                m_errorMsg.append( missingArgs[i].c_str() );

                if ( i < missingArgs.size() - 1 )
                {
                    m_errorMsg.append( ", " );
                }
            }
        }

        return succeeded;
    }

    void CommandLineParser::PrintHelp() const
    {
        PrintHelp( m_boolArgs );
        PrintHelp( m_intArgs );
        PrintHelp( m_floatArgs );
        PrintHelp( m_stringArgs );
    }
}