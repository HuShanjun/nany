/*
*	与操作系统无关的读写Ini文件的实现
*/
#pragma once

#include "GammaCommon/IGammaFileMgr.h"
#include "GammaCommon/CGammaObject.h"

namespace Gamma
{
	//---------------------------------------------------------------------------
	struct SIniFile;

	class GAMMA_COMMON_API CIniFile : public CGammaObject
	{
		SIniFile*		m_pFile;
		bool			Prepare( char* lpBuffer ); 

	public:
		CIniFile();
		~CIniFile();						

		const char*		GetFileName();
		// 从缓冲区初始化		
		bool			Init( const tbyte* pBuffer, uint32_t nSize );		
		// 打开文件，扫描一遍，为以后的读操作做好准备(Get...)		
		bool			Open( const char* szFileName );		
		// 打开文件，扫描一遍，为以后的读操作做好准备(Get...)		
		bool			Open( const wchar_t* szFileName );	
		// 保存文件
		bool			WriteTo( IGammFileInterface* pFile, ETextFileType eType = eTFT_Default );		
		// 保存文件
		bool			Save( const char* szFileName = NULL, ETextFileType eType = eTFT_Default );		
		// 清除文件内容
		void			Clear();
		// 关闭文件,释放内存 		
		void			Close();	
		// 得到下一个Section下的名字，NULL获取第一个
		const char*		GetNextSection( const char* lpSection );	
		// 得到某个Section下的下一个Key下的名字，NULL获取第一个
		const char*		GetNextKey( const char* lpSection, const char* lpKeyName );	
		// 判断某个Section是否存在或者Section下的Key的配置是否存在
		bool			HasSetting(const char* lpSection, const char* lpKeyName);
		// 得到某个Section下的Key的值		
		const char*		GetString( const char* lpSection, const char* lpKeyName, const char* szDefault = NULL );	
		// 得到某个Section下的整数Key的值
		int32_t			GetInteger( const char* lpSection, const char* lpKeyName, int32_t nDefault = 0 );
		// 得到某个Section下的整数Key的值
		int64_t			GetInteger64( const char* lpSection, const char* lpKeyName, int64_t nDefault = 0 );
		// 得到某个Section下的浮点Key的值
		float			GetFloat( const char* lpSection, const char* lpKeyName, float fDefault = 0.0f );
		// 得到某个Section下的字符Key的值
		char			GetChar( const char* lpSection, const char* lpKeyName, char chDefault );	
		// 写入某个Section下的Key的值，字符串
		void 			WriteString( const char* lpSection, const char* lpKeyName, const char* lpString );
		// 写入某个Section下的Key的值，字符串
		void			WriteInteger( const char* lpSection, const char* lpKeyName, int32_t nValue );
		// 写入某个Section下的Key的值，字符串
		void			WriteInteger64( const char* lpSection, const char* lpKeyName, int64_t nValue );
		// 写入某个Section下的Key的值，字符串
		void			WriteFloat( const char* lpSection, const char* lpKeyName, float fValue );
	};
}


