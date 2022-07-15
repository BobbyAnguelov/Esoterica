#pragma once

#include "Engine/_Module/API.h"
#include "System/Math/Math.h"
#include "System/Types/Arrays.h"
#include "System/Systems.h"
#include "System/Types/HashMap.h"

//-------------------------------------------------------------------------
// Runtime Settings
//-------------------------------------------------------------------------
// These are runtime settings that can be set via console or IMGUI
// 
// Settings support adding description text
// Settings support nested categories via the '/' delimiter
//
// Usage:
// static RuntimeSettingInt g_setting( "Name", "Category/SubCategory/etc...", "Description", -1 );
//
//-------------------------------------------------------------------------

namespace EE
{
    class SettingsRegistry;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API RuntimeSetting
    {
        friend SettingsRegistry;

        constexpr static char const s_categorySeparatorToken = '/';
        static RuntimeSetting* s_pHead;
        static RuntimeSetting* s_pTail;

    public:

        enum class Type
        {
            Bool,
            Int,
            Float,
            String
        };

        inline Type GetType() const { return m_type; }
        inline char const* GetName() const { return m_name; }
        inline char const* GetCategory() const { return m_category; }
        inline char const* GetDescription() const { return m_description; }

    protected:

        RuntimeSetting( char const* pName, char const* pCategory, char const* pDescription, Type type );
        virtual ~RuntimeSetting();

    protected:

        RuntimeSetting*         m_pNext = nullptr;
        char const              m_name[100] = { 0 };
        char const              m_category[156] = { 0 };
        uint32_t                m_nameHash = 0; // The hash of the combined category and name strings
        Type                    m_type;
        char const              m_description[256] = { 0 };
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API RuntimeSettingBool : public RuntimeSetting
    {

    public:

        RuntimeSettingBool( char const* pName, char const* pCategory, char const* pDescription, bool initialValue );

        inline RuntimeSettingBool& operator=( bool value ) { m_value = value; return *this; }
        inline operator bool() const { return m_value; }

    private:

        bool                    m_value = false;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API RuntimeSettingInt : public RuntimeSetting
    {

    public:

        RuntimeSettingInt( char const* pName, char const* pCategory, char const* pDescription, int32_t initialValue );
        RuntimeSettingInt( char const* pName, char const* pCategory, char const* pDescription, int32_t initialValue, int32_t min, int32_t max );

        inline RuntimeSettingInt& operator=( int32_t value ) { m_value = Math::Clamp( value, m_min, m_max ); return *this; }
        inline operator int32_t() const { return m_value; }

        inline int32_t GetMin() const { return m_min; }
        inline int32_t GetMax() const { return m_max; }
        inline bool HasLimits() const { return m_min != INT_MIN || m_max != INT_MAX; }

    private:

        int32_t                 m_value = false;
        int32_t                 m_min = INT_MIN;
        int32_t                 m_max = INT_MAX;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API RuntimeSettingFloat : public RuntimeSetting
    {

    public:

        RuntimeSettingFloat( char const* pName, char const* pCategory, char const* pDescription, float initialValue );
        RuntimeSettingFloat( char const* pName, char const* pCategory, char const* pDescription, float initialValue, float min, float max );

        inline RuntimeSettingFloat& operator=( float value ) { m_value = Math::Clamp( value, m_min, m_max ); return *this; }
        inline operator float() const { return m_value; }

        inline float GetMin() const { return m_min; }
        inline float GetMax() const { return m_max; }
        inline bool HasLimits() const { return m_min != -FLT_MAX || m_max != FLT_MAX; }

    private:

        float                 m_value = false;
        float                 m_min = -FLT_MAX;
        float                 m_max = FLT_MAX;
    };
}

//-------------------------------------------------------------------------
// Runtime Settings Registry
//-------------------------------------------------------------------------
// TODO: support saving and loading from INI

namespace EE
{
    class EE_ENGINE_API SettingsRegistry : public ISystem
    {

    public:

        EE_SYSTEM_ID( SettingsRegistry );

    public:

        SettingsRegistry();

        THashMap<uint32_t, RuntimeSetting*> const& GetAllSettings() const { return m_settings; }
        RuntimeSetting const* TryGetSetting( char const* pCategoryName, char const* pSettingName ) const;


    private:

        void GenerateSettingsMap();

    private:

        THashMap<uint32_t, RuntimeSetting*>           m_settings;
    };
}