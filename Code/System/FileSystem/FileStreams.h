#pragma once

#include "FileSystemPath.h"
#include <fstream>
#include <iosfwd>

//-------------------------------------------------------------------------

namespace EE
{
    namespace FileSystem
    {
        class EE_SYSTEM_API InputFileStream
        {
        public:

            InputFileStream( Path const& filePath );
            inline bool IsValid() const { return m_filestream.is_open(); }
            inline std::ifstream& GetStream() { return m_filestream; }

            inline void Read( void* pData, size_t size )
            {
                EE_ASSERT( IsValid() );
                m_filestream.read( (char*) pData, size );
            }

            inline void Close()
            {
                EE_ASSERT( IsValid() );
                m_filestream.close();
            }

        private:

            InputFileStream();

        private:

            std::ifstream m_filestream;
        };

        //-------------------------------------------------------------------------
        
        class EE_SYSTEM_API OutputFileStream
        {
        public:

            OutputFileStream( Path const& filePath );
            inline std::ofstream& GetStream() { return m_filestream; }
            inline bool IsValid() const { return m_filestream.is_open(); }

            inline void Write( void* pData, size_t size )
            {
                EE_ASSERT( IsValid() );
                m_filestream.write( (char*) pData, size );
            }
            
            inline void Close()
            {
                EE_ASSERT( IsValid() );
                m_filestream.close();
            }

        private:

            OutputFileStream();

        private:

            std::ofstream m_filestream;
        };
    }
}