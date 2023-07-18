#pragma once
#include "Base/_Module/API.h"
#include "Base/Types/Containers_ForwardDecl.h"

//-------------------------------------------------------------------------

struct _dictionary_;

//-------------------------------------------------------------------------

namespace EE
{
    namespace FileSystem { class Path; }

    //-------------------------------------------------------------------------

    class EE_BASE_API IniFile
    {

    public:

        IniFile();
        IniFile( FileSystem::Path const& filePath );
        ~IniFile();

        inline bool IsValid() const { return m_pDictionary != nullptr; }
        void SaveToFile( FileSystem::Path const& filePath ) const;
        bool HasEntry( char const* key ) const;

        //-------------------------------------------------------------------------

        bool TryGetBool( char const* key, bool& outValue ) const;
        bool TryGetInt( char const* key, int32_t& outValue ) const;
        bool TryGetUInt( char const* key, uint32_t& outValue ) const;
        bool TryGetString( char const* key, String& outValue ) const;
        bool TryGetFloat( char const* key, float& outValue ) const;

        bool GetBoolOrDefault( char const* key, bool defaultValue ) const;
        int32_t GetIntOrDefault( char const* key, int32_t defaultValue ) const;
        uint32_t GetUIntOrDefault( char const* key, uint32_t defaultValue ) const;
        String GetStringOrDefault( char const* key, String const& defaultValue ) const;
        String GetStringOrDefault( char const* key, char const* pDefaultValue ) const;
        float GetFloatOrDefault( char const* key, float defaultValue ) const;

        //-------------------------------------------------------------------------

        void CreateSection( char const* section );
        void SetBool( char const* key, bool value );
        void SetInt( char const* key, int32_t value );
        void SetUInt( char const* key, uint32_t value );
        void SetFloat( char const* key, float value );
        void SetString( char const* key, char const* pString );
        void SetString( char const* key, String const& string );

    private:

        _dictionary_* m_pDictionary = nullptr;
    };
}