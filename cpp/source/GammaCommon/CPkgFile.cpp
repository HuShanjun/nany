
#ifdef _WIN32
#include <windows.h>
#endif
#include "CGammaFileMgr.h"
#include "GammaCommon/CPkgFile.h"
#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/GammaCodeCvs.h"
#include "GammaCommon/GammaPackageCommon.h"
#include <algorithm>
#include <string>
#include <map>
#include <list>
#include "CPackage.h"
#include "GammaCommon/GammaHelp.h"
#include <fstream>
#include <iostream>

using namespace std;

using namespace std;

namespace Gamma
{
	struct SPkgFile : public IResListener
	{
		SPkgFile()
			: m_pBuffer( NULL )
			, m_nBufferSize( INVALID_32BITID )
			, m_nPosition( INVALID_32BITID )
		{
		}

		~SPkgFile()
		{
			Clear();
		}

		union
		{
			CFileContext*	m_pBufferFile;
			tbyte*			m_pBuffer;
		};

		std::string			m_strFileName;
		uint32				m_nBufferSize;
		uint32				m_nPosition;

		void Clear()
		{
			if( m_pBuffer == NULL )
				return;
			if( m_nBufferSize != INVALID_32BITID )
				delete [] m_pBuffer;
			else if( m_pBufferFile->GetPackage() )
				m_pBufferFile->GetPackage()->Release();

			m_pBuffer = NULL;
			m_nBufferSize = INVALID_32BITID;
			m_nPosition = INVALID_32BITID;
		}

		void OnLoadedEnd( const char* szFileName, const tbyte* pBuffer, uint32 nSize )
		{
			if( pBuffer )
			{
				m_pBuffer = new tbyte[nSize];
				m_nBufferSize = nSize;
				m_nPosition = 0;
				memcpy( m_pBuffer, pBuffer, nSize );
			}
			else
			{
				m_pBuffer = NULL;
				m_nBufferSize = INVALID_32BITID;
				m_nPosition = INVALID_32BITID;
			}
		}
	};

	//===================================================================================
	//  打包文件接口
	//===================================================================================
	CPkgFile::CPkgFile()
		: m_pFile( new SPkgFile )
	{
	}

	CPkgFile::CPkgFile( const char* szFileName, bool bBinary /*= true */ ) 
		: m_pFile( new SPkgFile )
	{
		Open( szFileName );
	}

	CPkgFile::CPkgFile( const wchar_t* szFileName, bool bBinary /*= true */ ) 
		: m_pFile( new SPkgFile )
	{
		Open( szFileName );
	}

	CPkgFile::~CPkgFile()
	{
		Close();
		delete m_pFile;
	}

	void CPkgFile::Close()
	{
		m_pFile->Clear();
	}

	bool CPkgFile::Seek( int32 pos, int32 nSeekType )
	{
		if ( !IsValid() )
			return false;

		int32 nNewPos;
		int32 nFileSize = (int32)Size();
		switch( nSeekType )
		{
		case SEEK_SET:
			nNewPos = pos;
			break;
		case SEEK_END:
			nNewPos = nFileSize + pos;
			break;
		case SEEK_CUR:
			nNewPos = m_pFile->m_nPosition + pos;
			break;
		default:
			GammaThrow( "Invalid seek origin.");
		}

		if( ( nNewPos < 0 ) || ( nNewPos > nFileSize ) )
			return false;

		m_pFile->m_nPosition = nNewPos;
		return true;
	}

	int32 CPkgFile::Read( void* pBuffer, uint32 nLen )
	{
		if( !IsValid() )
			return -1;

		uint32 nFileSize = Size();
		uint32 nNewPos = m_pFile->m_nPosition + nLen;
		if ( nNewPos > nFileSize )
		{
			nNewPos = nFileSize;
			nLen = nNewPos - m_pFile->m_nPosition;
		}

		if( nNewPos < (uint32)m_pFile->m_nPosition )
			return -1;

		nLen = nNewPos - m_pFile->m_nPosition;		
		memcpy( pBuffer, GetFileBuffer() + m_pFile->m_nPosition, nLen );
		m_pFile->m_nPosition = nNewPos;
		return nLen;
	}

	bool CPkgFile::Open( const char* szFileName )
	{
		Close();

		if( szFileName == NULL )
			return false;

		m_pFile->m_strFileName = szFileName;
		GammaString( m_pFile->m_strFileName );

		CPackageMgr& PackageMgr = CGammaFileMgr::Instance().GetFilePackageManager();
		m_pFile->m_pBufferFile = PackageMgr.GetFileBuff( m_pFile->m_strFileName.c_str() );

		if( m_pFile->m_pBufferFile )
		{
			m_pFile->m_nBufferSize = INVALID_32BITID;
			m_pFile->m_nPosition = 0;
			return true;
		}
		
		CGammaFileMgr::Instance().SyncLoad( m_pFile->m_strFileName.c_str(), true, m_pFile );
		if( !IsValid() )
			return false;
		return true;
	}

	bool CPkgFile::Open( const wchar_t *szFileName )
	{
		Close();

		if( szFileName == NULL )
			return false;

		m_pFile->m_strFileName = UcsToUtf8( szFileName );
		GammaString( m_pFile->m_strFileName );//将\\变为 \

		CPackageMgr& PackageMgr = CGammaFileMgr::Instance().GetFilePackageManager();
		m_pFile->m_pBufferFile = PackageMgr.GetFileBuff( m_pFile->m_strFileName.c_str() );

		if( m_pFile->m_pBufferFile )
		{
			m_pFile->m_nBufferSize = INVALID_32BITID;
			m_pFile->m_nPosition = 0;
			return true;
		}

		CGammaFileMgr::Instance().SyncLoad( m_pFile->m_strFileName.c_str(), true, m_pFile );
		if( !IsValid() )
			return false;
		return true;
	}

	int32 CPkgFile::Size() const
	{
		if( m_pFile->m_nBufferSize != INVALID_32BITID )
			return m_pFile->m_nBufferSize;
		if( !m_pFile->m_pBufferFile )
			return 0;
		return m_pFile->m_pBufferFile->Size();
	}

	const tbyte* CPkgFile::GetFileBuffer() const
	{
		if( m_pFile->m_nBufferSize != INVALID_32BITID )
			return m_pFile->m_pBuffer;
		if( m_pFile->m_pBufferFile )
			return m_pFile->m_pBufferFile->GetBuffer();
		return NULL;
	}

	int32 CPkgFile::Tell() const
	{ 
		return m_pFile->m_nPosition;		
	} 

	bool CPkgFile::IsValid() const 
	{ 
		return m_pFile->m_pBuffer != NULL;
	}

	const char* CPkgFile::GetFileName() const
	{ 
		return m_pFile->m_strFileName.c_str();
	}
}
