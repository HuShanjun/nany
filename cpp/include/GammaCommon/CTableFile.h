/*---------------------------------------------------------------------------
2005-12-16 柯达昭
完成制表符分割文件的读取，制表文件是一个特殊格式的文本文件，将Excel编辑的制表
保存为文本文件即可，制表符文件每行由回车换行符分隔(0x0d 0x0a)，每个单元之间由
制表符（TAB）（0x09）分隔.
//---------------------------------------------------------------------------*/
#ifndef _GAMMA_TABFILE_H_
#define _GAMMA_TABFILE_H_

#include "GammaCommon/GammaCommonType.h"
#include "GammaCommon/IGammaFileMgr.h"
#include "GammaCommon/GammaCodeCvs.h"
#include "GammaCommon/CGammaObject.h"
#include <iostream>

namespace Gamma
{
	/// 注意，数据行下标从1开始，标题栏为0，列下标还是从0开始
	struct STableFile;

	class GAMMA_COMMON_API CTabFile : public CGammaObject
	{
		STableFile*		m_pFile;		

	protected:
		bool			MakeOffset( char cSeperator = '\t' );

	public:
		CTabFile(void);
		~CTabFile(void);

		// 从缓冲区初始化		
		bool			Init( const tbyte* pBuffer, uint32 nSize, char cSeperator = '\t' );
		// 载入制表符分隔文件
		bool			Load( const char* szFileName, char cSeperator = '\t' );
		// 载入制表符分隔文件
		bool			Load( const wchar_t* szFileName, char cSeperator = '\t' );
		// 清空
		void			Clear();
		// 得到行数
		int32			GetHeight() const;
		// 得到列数
		int32			GetWidth() const;
		// 得到列名
		int32			GetCloumn( const char* szColumnName ) const;
		// 根据列号得到某行某列
		const char*		GetString( int32 nRow, int32 nColumn, const char* szDefault = "" ) const;
		// 根据列名得到某行某列
		const char*		GetString( int32 nRow, const char* szColumnName, const char* szDefault = "" ) const;
		// 根据列号得到某行某列
		const char*		GetDicString( int32 nRow, int32 nColumn, const char* szDefault = "" ) const;
		// 根据列名得到某行某列
		const char*		GetDicString( int32 nRow, const char* szColumnName, const char* szDefault = "" ) const;
		// 根据列号得到某行某列
		int32			GetInteger( int32 nRow, int32 nColumn, int32 defaultvalue = 0  ) const;
		// 根据列名得到某行某列
		int32			GetInteger( int32 nRow, const char* szColumnName, int32 defaultvalue = 0 ) const;
		// 根据列号得到某行某列
		float			GetFloat( int32 nRow, int32 nColumn, float defaultvalue = 0.0f) const;
		// 根据列名得到某行某列
		float			GetFloat( int32 nRow, const char* szColumnName, float defaultvalue = 0.0f ) const;
		// 根据列号得到某行某列
		int64			GetInteger64( int32 nRow, int32 nColumn, int64 defaultvalue = 0 ) const;	
		// 根据列名得到某行某列
		int64			GetInteger64( int32 nRow, const char* szColumnName, int64 defaultvalue = 0 ) const;	
		// 根据列号得到某行某列
		double			GetDouble( int32 nRow, int32 nColumn, double defaultvalue = 0 ) const;	
		// 根据列名得到某行某列
		double			GetDouble( int32 nRow, const char* szColumnName, double defaultvalue = 0 ) const;
		// 根据列号得到某行某列 日期格式:2015-03-25 00:00:00
		int64			GetDateSec( int32 nRow, int32 nColumn, int64 defaultvalue = 0 ) const;	
		// 根据列名得到某行某列 日期格式:2015-03-25 00:00:00
		int64			GetDateSec( int32 nRow, const char* szColumnName, int64 defaultvalue = 0 ) const;

		template<typename T1, typename T2>
		inline void ReadTableData(T1& rtValue, int32 nRow, T2 Column)
		{
			GammaAst( nRow != nRow );
		}
	};

	template<>
	inline void CTabFile::ReadTableData<int32, int32>(int32& rtValue, int32 nRow, int32 nColumn)
	{ rtValue = GetInteger(nRow, nColumn); }

	template<>
	inline void CTabFile::ReadTableData<uint32, int32>(uint32& rtValue, int32 nRow, int32 nColumn)
	{ rtValue = (uint32)GetInteger(nRow, nColumn); }

	template<>
	inline void CTabFile::ReadTableData<uint16, int32>(uint16& rtValue, int32 nRow, int32 nColumn)
	{ rtValue = (uint16)GetInteger(nRow, nColumn); }

	template<>
	inline void CTabFile::ReadTableData<int16, int32>(int16& rtValue, int32 nRow, int32 nColumn)
	{ rtValue = (int16)GetInteger(nRow, nColumn); }

	template<>
	inline void CTabFile::ReadTableData<float, int32>(float& rtValue, int32 nRow, int32 nColumn)
	{ rtValue = GetFloat(nRow, nColumn); }

	template<>
	inline void CTabFile::ReadTableData<const char*, int32>(const char*& rtValue, int32 nRow, int32 nColumn)	
	{ rtValue = GetString(nRow, nColumn); }

	template<>
	inline void CTabFile::ReadTableData<int32, const char*>(int32& rtValue, int32 nRow, const char* szColumnName)
	{ rtValue = GetInteger(nRow, szColumnName, 0); }
	
	template<>
	inline void CTabFile::ReadTableData<uint32, const char*>(uint32& rtValue, int32 nRow, const char* szColumnName)
	{ rtValue = (uint32)GetInteger(nRow, szColumnName, 0); }

	template<>
	inline void CTabFile::ReadTableData<uint16, const char*>(uint16& rtValue, int32 nRow, const char* szColumnName)
	{ rtValue = (uint16)GetInteger(nRow, szColumnName, 0); }

	template<>
	inline void CTabFile::ReadTableData<float, const char*>(float& rtValue, int32 nRow, const char* szColumnName)
	{ rtValue = GetFloat(nRow, szColumnName, 0.0f); }

	template<>
	inline void CTabFile::ReadTableData<bool, const char*>(bool& rtValue, int32 nRow, const char* szColumnName)
	{ rtValue = !!GetInteger(nRow, szColumnName, 0); }

	template<>
	inline void CTabFile::ReadTableData<const char*, const char*>(const char*& rtValue, int32 nRow, const char* szColumnName)
	{ rtValue = GetString(nRow, szColumnName); }

	template<>
	inline void CTabFile::ReadTableData<std::string, const char*>(std::string& rtValue, int32 nRow, const char* szColumnName)
	{ rtValue = GetString(nRow, szColumnName); }

	template<>
	inline void CTabFile::ReadTableData<std::string, int32>(std::string& rtValue, int32 nRow, int32 nColumn)
	{ rtValue = GetString(nRow, nColumn); }

}
#endif
