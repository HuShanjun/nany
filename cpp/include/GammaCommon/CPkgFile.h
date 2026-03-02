//=====================================================================
// CPkgFile.h 
// 定义兼容从文件包读取文件以及直接读取系统文件
// 柯达昭
// 2007-09-15
//=======================================================================

#ifndef __PACKAGE_FILE_H__
#define __PACKAGE_FILE_H__

#include "GammaCommon/IGammaFileMgr.h"
#include "GammaCommon/GammaCodeCvs.h"
#include <string.h>
#include <fstream>
#include <iosfwd>
#ifndef _WIN32
#include "GammaCommon/GammaCodeCvs.h"
#endif

namespace Gamma
{
    class CBufFile
    {
    protected:
        const tbyte*    m_pBuf;
		uint32          m_nCurPos;
		uint32          m_nMaxSize;
		uint32          m_nMinSize;

        friend class CPkgFile;
    public:
        CBufFile( const void* pBuf, uint32 nMaxSize, uint32 nMinSize = 0 ) 
			: m_pBuf( (const tbyte*)pBuf )
			, m_nMaxSize( nMaxSize )
			, m_nMinSize( nMinSize )
			, m_nCurPos(0)
		{}

		bool			IsEOF() const				{ return m_nCurPos >= m_nMaxSize; }
		const tbyte*	GetBuf() const				{ return m_pBuf; }
        uint32			GetBufSize() const			{ return m_nMaxSize; }
        uint32			GetPos()  const				{ return m_nCurPos; }
		void            Seek( uint32 nPos )			{ m_nCurPos = nPos; }
		void            SeekFromCur( int32 nDel )	{ m_nCurPos += nDel; }
		const tbyte*	GetBufByCur() const			{ return m_pBuf + m_nCurPos; }	//请检查 by hqli

        void Read( void* pBuf, uint32 nLen )
		{ 
			if( m_nCurPos < m_nMinSize || m_nCurPos + nLen > m_nMaxSize )
			{
				GammaErr << "Read Data Error," << m_nCurPos << "," 
					<< m_nMaxSize << "," << nLen << std::endl;
				return;
			}
            memcpy( pBuf, m_pBuf + m_nCurPos, nLen );
            m_nCurPos += nLen;
        }

		int32 SafeRead( void* pBuf, uint32 nLen )
		{
			if( m_nCurPos < m_nMinSize || m_nCurPos >= m_nMaxSize ) 
				return -1;
			if( m_nCurPos + nLen > m_nMaxSize )
				nLen = m_nMaxSize - m_nCurPos;
			Read( pBuf, nLen );
			return (int32)nLen;
		}

		template<typename DataType>
		const DataType*	PopData( uint32 nSize = sizeof(DataType) )		
		{ 
			GammaAst( m_nCurPos >= m_nMinSize );
			const DataType* pData = (const DataType*)( m_pBuf + GetPos() );
			GammaAst( m_nCurPos <= m_nMaxSize );
			SeekFromCur( (int32)nSize );
			return pData;
		}

		template<typename DataType>
		DataType GetData()		
		{ 
			DataType Data;
			Read( &Data, sizeof(Data) );
			return Data;
		}

		template<typename DataType> 
		CBufFile& operator >> ( DataType& Data )
		{
			Read( &Data, sizeof(Data) );
			return *this;
		}
	};

	class CBufFileEx : public CBufFile
	{
	public:
		CBufFileEx( tbyte* pBuf, uint32 nMaxSize, uint32 nMinSize = 0 )
			: CBufFile( pBuf, nMaxSize, nMinSize ){}

		void Write( const void* pBuf, uint32 nLen )
		{ 
			GammaAst( m_nCurPos >= m_nMinSize );
			GammaAst( m_nCurPos + nLen <= m_nMaxSize );
			memcpy( ( (tbyte*)m_pBuf ) + m_nCurPos, pBuf, nLen );
			m_nCurPos += nLen;
		}

		template<typename DataType>
		CBufFileEx& PushData( const DataType* pData, uint32 nSize = sizeof(DataType) )		
		{ 
			Write( pData, nSize );
			return *this;
		}

		template<typename DataType> 
		CBufFileEx& operator << ( const DataType& Data )
		{
			return PushData( &Data );
		}
	};

	class CStringFile
	{
		std::string		m_szBuf;
		uint32          m_nCurPos;
	public:
		CStringFile( uint32 nReserveSize = 0 ) 
			: m_nCurPos(0)
		{
			if( nReserveSize )
				m_szBuf.reserve( nReserveSize );
		}

		const tbyte*		GetBuf() const				{ return (const tbyte*)m_szBuf.c_str(); }
		const std::string&	GetString() const			{ return m_szBuf; }
		uint32				GetBufSize() const			{ return (uint32)m_szBuf.size(); }
		uint32				GetPos()  const				{ return m_nCurPos; }
		uint32				GetCapacity() const			{ return (uint32)m_szBuf.capacity(); }
		void				Seek( uint32 nPos )			{ m_nCurPos = nPos; }
		void				SeekFromCur( int32 nDel )	{ m_nCurPos += nDel; }
		void				Reserve( uint32 nSize )		{ m_szBuf.reserve( nSize ); }
		void				Clear()						{ m_szBuf.clear(); m_nCurPos = 0; }

		void Write( const void* pBuf, uint32 nLen )
		{ 
			if( m_nCurPos + nLen > m_szBuf.capacity() )
				m_szBuf.reserve( ( GetBufSize() + nLen )*2 );

			if( m_nCurPos < GetBufSize() )
			{
				size_t nLeftSize = GetBufSize() - m_nCurPos;
				if( nLeftSize >= nLen )
					memcpy( &m_szBuf[m_nCurPos], pBuf, nLen );
				else
				{
					memcpy( &m_szBuf[m_nCurPos], pBuf, nLeftSize );
					m_szBuf.append( (const char*)pBuf + nLeftSize, nLen - nLeftSize );
				}
			}
			else
			{
				if( m_nCurPos > GetBufSize() )
					m_szBuf.append( m_nCurPos - GetBufSize(), 0 );
				m_szBuf.append( (const char*)pBuf, nLen );
			}

			m_nCurPos += nLen;
		}

		template<typename DataType>
		CStringFile& PushData( const DataType* pData, uint32 nSize = sizeof(DataType) )		
		{ 
			Write( pData, nSize );
			return *this;
		}

		template<typename DataType> 
		CStringFile& operator << ( const DataType& Data )
		{
			return PushData( &Data );
		}
	};

	typedef CBufFile CBufferReader;
	typedef CBufFileEx CBufferWriter;
	typedef CStringFile CDynamicBufferWriter;
	struct  SPkgFile;

    class GAMMA_COMMON_API CPkgFile
    {
	public:
		CPkgFile();
		CPkgFile( const char* szFileName, bool bBinary = true );
		CPkgFile( const wchar_t* szFileName, bool bBinary = true );

		~CPkgFile(); 

		bool			Open( const char *szFileName );
		bool			Open( const wchar_t *szFileName );
		bool			Seek( int32 pos, int32 nSeekType = SEEK_SET );	// TODO : add default
		int32			Read( void *buffer, uint32 len );
		void			Close();

		int32			Tell()	const;
		bool			IsValid() const;
		const char*		GetFileName() const;
		const tbyte*	GetFileBuffer() const;
		int32			Size()		const;

		operator bool() const { return IsValid(); }
	private:
		CPkgFile(const CPkgFile &);
		CPkgFile& operator = (const CPkgFile &);

		SPkgFile*		m_pFile;
    };


	template<class _Elem, class _Traits >
    class basic_ipkgstream;

	template<class _Elem, class _Traits = std::char_traits<_Elem> >
    class ipkgbuf : public std::basic_streambuf<_Elem, _Traits >
    {
	private:
		typedef basic_ipkgstream<_Elem, _Traits > Parent_t;
		typedef typename Parent_t::pos_type pos_type_t;
		typedef typename Parent_t::off_type off_type_t;
	
		friend class basic_ipkgstream<_Elem, _Traits >;

        _Elem       m_Buf[1];
        CPkgFile    m_PkFile;

    protected:
        ipkgbuf()
        {
            this->setg( m_Buf, m_Buf, m_Buf );
        }

        virtual typename _Traits::int_type underflow()
        {
            if( 0 == m_PkFile.Read( &m_Buf, sizeof(m_Buf) ) )
                return _Traits::eof();
            this->setg( m_Buf, m_Buf, m_Buf + 1 );
            return _Traits::to_int_type( m_Buf[0] );
        }

        virtual pos_type_t seekpos( pos_type_t _Sp, std::ios_base::openmode /*_Which*/)
        {
            if( !m_PkFile.Seek( (int32)_Sp, SEEK_SET ) )
                return _Traits::eof();
            return m_PkFile.Tell();
        }

        virtual pos_type_t seekoff( off_type_t _Off, std::ios_base::seekdir _Way,std::ios_base::openmode /*_Which*/)
        {
            switch( _Way )
            {
            case std::ios_base::beg:
                return m_PkFile.Seek( (int32)_Off, SEEK_SET ) ? m_PkFile.Tell() : _Traits::eof();
            case std::ios_base::end:
                return m_PkFile.Seek( (int32)_Off, SEEK_END ) ? m_PkFile.Tell() : _Traits::eof();
            case std::ios_base::cur:
                return m_PkFile.Seek( (int32)_Off, SEEK_CUR ) ? m_PkFile.Tell() : _Traits::eof();
            default:
                GammaThrow( "Invalid seek type!");
            };
        }

    public:
        bool open( const char *szFilename )		{ return m_PkFile.Open( szFilename ); }
        bool open( const wchar_t *szFilename )	{ return m_PkFile.Open( szFilename ); }
        void close()							{ m_PkFile.Close(); }
        bool isopen()const						{ return m_PkFile.IsValid(); }
        std::size_t size()const					{ return m_PkFile.Size(); }
		const char*	GetFileName() const			{ return m_PkFile.GetFileName();}
		const tbyte* GetFileBuffer() const		{ return m_PkFile.GetFileBuffer(); }
    };

	template<class _Elem, class _Traits = std::char_traits<_Elem> >
    class basic_ipkgstream : public std::basic_istream<_Elem, _Traits >
    {
		typedef std::basic_istream<_Elem, _Traits > parent_t;
        ipkgbuf<_Elem, _Traits >    m_StreamBuf;

		const basic_ipkgstream& operator= ( const basic_ipkgstream& ){}
        explicit basic_ipkgstream( const basic_ipkgstream& ){}
    public:
        explicit basic_ipkgstream() : parent_t( &m_StreamBuf ){}
        explicit basic_ipkgstream( const char* szFilename ) 
			: parent_t( &m_StreamBuf )
        {
            open( szFilename );
        }

        explicit basic_ipkgstream( const wchar_t* szFilename ) 
			: parent_t( &m_StreamBuf )
        {
            open( szFilename );
        }

        bool open( const char *szFilename )	
		{ 
			if( !m_StreamBuf.open( szFilename ) )
				this->setstate( std::ios::failbit );
			return !!*this;
		}

		bool open( const wchar_t *szFilename )	
		{ 
			if( !m_StreamBuf.open( szFilename ) )
				this->setstate( std::ios::failbit );
			return !!*this;
		}

        void close()			{ m_StreamBuf.close(); }
        bool isopen()const		{ return m_StreamBuf.isopen(); }
        size_t size()const		{ return m_StreamBuf.size(); }
		const _Elem*	GetFileName() const			{ return m_StreamBuf.GetFileName();}
		const tbyte*	GetFileBuffer() const		{ return m_StreamBuf.GetFileBuffer(); }
    };

	template<class _Elem, class _Traits = std::char_traits<_Elem> >
    class basic_opkstream : public std::basic_ofstream<_Elem, _Traits >
	{
		typedef std::basic_ofstream<_Elem, _Traits > parent_t;
    public:
        explicit basic_opkstream() {}

		explicit basic_opkstream( const char* szFilename, bool bBinary = true ) 
#ifdef _WIN32
			: parent_t( Utf8ToUcs( szFilename ).c_str(),
			std::ios_base::openmode( std::ios_base::out|( bBinary ? std::ios_base::binary : 0 ) ) ){}
#else
            : parent_t( szFilename,
                std::ios_base::openmode( std::ios_base::out|( bBinary ? std::ios_base::binary : 0 ) ) ){}
#endif

		explicit basic_opkstream( const wchar_t* szFilename, bool bBinary = true )
#ifdef _WIN32
			: parent_t( szFilename, 
				std::ios_base::openmode(std::ios_base::out
                                                         | (bBinary ? std::ios_base::binary : 0))){}
#else
			: parent_t( UcsToUtf8( szFilename ).c_str(),
                 std::ios_base::openmode( std::ios_base::out|( bBinary ? std::ios_base::binary : 0 ) ) ){}
#endif
        void write( const void* pBuf, std::streamsize nSize )
		{
			GammaAst( nSize%sizeof(_Elem) == 0 );
			parent_t::write( (const _Elem*)pBuf, nSize/sizeof(_Elem) );
		}
    };

	typedef basic_ipkgstream<char, std::char_traits<char> >		ipkgstream;
	typedef basic_opkstream<char, std::char_traits<char> >			opkgstream;
	typedef basic_ipkgstream<wchar_t, std::char_traits<wchar_t> >	wipkgstream;
	typedef basic_opkstream<wchar_t, std::char_traits<wchar_t> >	wopkgstream;
}

#endif
