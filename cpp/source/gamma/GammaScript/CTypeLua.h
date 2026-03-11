/**@file  		CTypeLua.h
* @brief		Data interface between LUA&c++
* @author		Daphnis Kau
* @date			2019-06-24
* @version		V1.0
*/

#ifndef __TYPE_LUA_H__
#define __TYPE_LUA_H__
#include <stdlib.h>
#include "GammaCommon/GammaHelp.h"
#include "CTypeBase.h"
#include "GammaScript/CScriptLua.h"

struct lua_State;
namespace Gamma
{
	class CLuaTypeBase;
	//=====================================================================
	/// aux function
	//=====================================================================
	double			GetNumFromLua( lua_State* pL, int32_t nStkId );
	void*			GetPointerFromLua( lua_State* pL, int32_t nStkId, 
						int32_t nCppObjStr, int32_t nWeakTable );
	bool			PushPointerToLua( lua_State* pL, void* pBuffer, 
						int32_t nCppObjStr, int32_t nWeakTable, bool bCreateStreamBuff = true );
	void			RegisterPointerClass( CScriptLua* pScript );
	CLuaTypeBase*	GetLuaTypeBase( DataType eType );

    //=====================================================================
    /// Base class of data type
    //=====================================================================
    class CLuaTypeBase : public CTypeBase
	{
	public:
		CLuaTypeBase(){}
        virtual void GetFromVM( DataType eType, lua_State* pL, char* pDataBuf, 
			int32_t nStkId, int32_t nCppObjStr, int32_t nWeakTable ) = 0;        
        virtual void PushToVM( DataType eType, lua_State* pL, char* pDataBuf, 
			int32_t nCppObjStr, int32_t nWeakTable )= 0;
    };

    //=====================================================================
    /// Common class of data type
    //=====================================================================
    template<typename T>
    class TLuaValue : public CLuaTypeBase
    {
    public:
        void GetFromVM( DataType eType, lua_State* pL, char* pDataBuf, 
			int32_t nStkId, int32_t /*nCppObjStr*/, int32_t /*nWeakTable*/ )
		{ 
			double fValue = GetNumFromLua( pL, nStkId );
			*(T*)( pDataBuf ) = fValue < 0 ? (T)(int64_t)fValue : (T)(uint64_t)fValue;
		};

        void PushToVM( DataType eType, lua_State* pL, char* pDataBuf,
			int32_t /*nCppObjStr*/, int32_t /*nWeakTable*/ )
		{ 
			lua_pushnumber( pL, (double)*(T*)( pDataBuf ) );
		}

		static TLuaValue<T>& GetInst() 
		{ 
			static TLuaValue<T> s_Instance; 
			return s_Instance; 
		}
    };

    // POD type class specialization
    template<> inline void TLuaValue<float>::GetFromVM( DataType eType, 
		lua_State* pL, char* pDataBuf, int32_t nStkId, int32_t, int32_t )
	{ *(float*)( pDataBuf ) = (float)GetNumFromLua( pL, nStkId ); }

    template<> inline void TLuaValue<float>::PushToVM( DataType eType, 
		lua_State* pL, char* pDataBuf, int32_t, int32_t )
	{ lua_pushnumber( pL, *(float*)( pDataBuf ) ); }

	template<> inline void TLuaValue<double>::GetFromVM( DataType eType,
		lua_State* pL, char* pDataBuf, int32_t nStkId, int32_t, int32_t )
	{ *(double*)( pDataBuf ) = GetNumFromLua( pL, nStkId ); }

	template<> inline void TLuaValue<double>::PushToVM( DataType eType,
		lua_State* pL, char* pDataBuf, int32_t, int32_t )
	{ lua_pushnumber( pL, *(double*)( pDataBuf ) ); }

	template<> inline void TLuaValue<bool>::GetFromVM( DataType eType,
		lua_State* pL, char* pDataBuf, int32_t nStkId, int32_t, int32_t )
	{ *(bool*)( pDataBuf ) = lua_toboolean( pL, nStkId ); }

	template<> inline void TLuaValue<bool>::PushToVM( DataType eType,
		lua_State* pL, char* pDataBuf, int32_t, int32_t )
	{ lua_pushboolean( pL, *(bool*)( pDataBuf ) ); }

	template<> inline void TLuaValue<void*>::GetFromVM( DataType eType,
		lua_State* pL, char* pDataBuf, int32_t nStkId, 
		int32_t nCppObjStr, int32_t nWeakTable )
	{ *(void**)( pDataBuf ) = GetPointerFromLua( pL, nStkId, nCppObjStr, nWeakTable ); }

	template<> inline void TLuaValue<void*>::PushToVM( DataType eType,
		lua_State* pL, char* pDataBuf, int32_t nCppObjStr, int32_t nWeakTable )
	{ PushPointerToLua( pL, *(void**)( pDataBuf ), nCppObjStr, nWeakTable ); }

	template<> inline void TLuaValue<const char*>::GetFromVM( DataType eType,
		lua_State* pL, char* pDataBuf, int32_t nStkId, int32_t, int32_t )
    { *(const char**)( pDataBuf ) = lua_tostring( pL, nStkId ); }

	template<> inline void TLuaValue<const char*>::PushToVM( DataType eType,
		lua_State* pL, char* pDataBuf, int32_t, int32_t )
    { lua_pushstring( pL, *(const char**)( pDataBuf ) ); }

	template<> inline void TLuaValue<const wchar_t*>::GetFromVM( DataType eType,
		lua_State* pL, char* pDataBuf, int32_t nStkId, int32_t, int32_t )
    { *(const wchar_t**)( pDataBuf ) = CScriptLua::ConvertUtf8ToUcs2( pL, nStkId ); }

	template<> inline void TLuaValue<const wchar_t*>::PushToVM( DataType eType,
		lua_State* pL, char* pDataBuf, int32_t, int32_t )
    { CScriptLua::NewUnicodeString( pL, *(const wchar_t**)( pDataBuf ) ); }

    //=====================================================================
    /// Interface of class pointer
    //=====================================================================
    class CLuaObject : public TLuaValue<void*>
    {
    public:
		CLuaObject();

		static CLuaObject&	GetInst();
        void GetFromVM( DataType eType, lua_State* pL, char* pDataBuf,
			int32_t nStkId, int32_t nCppObjStr, int32_t nWeakTable );
        void PushToVM( DataType eType,lua_State* pL, char* pDataBuf,
			int32_t nCppObjStr, int32_t nWeakTable );
    };

    //=====================================================================
    /// Interface of class value
    //=====================================================================
    class CLuaValueObject : public CLuaObject
    {
    public:
		CLuaValueObject();

		static CLuaValueObject&	GetInst();
		void GetFromVM( DataType eType, lua_State* pL, char* pDataBuf, 
			int32_t nStkId, int32_t nCppObjStr, int32_t nWeakTable );
		void PushToVM( DataType eType, lua_State* pL, char* pDataBuf, 
			int32_t nCppObjStr, int32_t nWeakTable );
    };
}

#endif
