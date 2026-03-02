
#ifdef _WIN32
#include "GammaCommon/CThread.h"
#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/TGammaList.h"
#include "GammaCommon/GammaCodeCvs.h"
#include "GammaCommon/TGammaStrStream.h"
#include <string>
#include <vector>
#include "Iphlpapi.h"
#include <winsock2.h>
#include "ddraw.h"

namespace Gamma
{
	uint32_t AnsiToUcs( wchar_t* pUcs, uint32_t nSize, const char* pAnsi, uint32_t nLen );
	uint32_t UcsToAnsi( char* pAnsi, uint32_t nSize, const wchar_t* pUnicode, uint32_t nLen = -1 );
	const std::wstring AnsiToUcs( const char* pUtf8, uint32_t nLen = -1 );
	const std::string UcsToAnsi( const wchar_t* pUnicode, uint32_t nLen = -1 );


	class CWin32App
	{
		struct SFileOpenContext;
		typedef std::vector<std::string> CFileNameList;
		typedef TGammaList<SFileOpenContext> CFileOpenContextList;
		struct SFileOpenContext 
			: public CFileOpenContextList::CGammaListNode
		{
			bool				m_bList;
			int32_t				m_nType;
			void*				m_pContext;
			void*				m_funCallback;
			CFileNameList		m_vecFileName;
		};

		SHardwareDesc			m_HardwareDesc;
		std::string				m_strPackagePath;

		CLock					m_Lock;
		CFileOpenContextList	m_listContext;

		CWin32App();
		~CWin32App();
		void					FetchHardwareInfo();
		void					CheckFileCallback();

	public:
		static CWin32App&		GetInstance();
		void*					GetModuleHandle();
		void					GetHardwareDesc( SHardwareDesc& HardwareDesc );
		uint64_t					GetVersion();
		void					SetPackagePath( const char* szPath );
		const char*				GetPackagePath();
		int32_t					WindowMessagePump();
		void					SetClipboardContent( int32_t nType, const void* pContent, uint32_t nSize );
		void					GetClipboardContent( int32_t nType, const void*& pContent, uint32_t& nSize );
		bool					GetSystemFile( bool bList, int32_t nType, void* pContext, void* funCallback );
		void*					LoadDynamicLib( const char* szName );
		void*					GetFunctionAddress( void* pLibContext, const char* szName );
		void					OpenURL( const char* szUrl );
	};
}

#endif