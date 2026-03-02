/**@file  		CScriptLua.h
* @brief		LUA VM base wrapper
* @author		Daphnis Kau
* @date			2019-06-24
* @version		V1.0
*/

#ifndef __SCRIPT_LUA_H__
#define __SCRIPT_LUA_H__
#include "CScriptBase.h"

struct lua_State;
struct lua_Debug;

namespace Gamma
{
    class CDebugLua;
	class GAMMA_SCRIPT_API CScriptLua : public CScriptBase
	{
		struct SMemoryBlock	{ SMemoryBlock* m_pNext; };
		typedef std::vector<SMemoryBlock*> CMemoryBlockList;

		std::vector<lua_State*>	m_vecLuaState;
		std::wstring			m_szTempUcs2;
		std::string				m_szTempUtf8;


		std::vector<void*>		m_pAllAllocBlock;
		CMemoryBlockList		m_aryBlockByClass;
		bool					m_bPreventExeInRunBuffer;

        //==============================================================================
        // aux function
		//==============================================================================
		static void				PushCppObjAndWeakTable( lua_State* pL );
		static int32_t			GetIndexClosure( lua_State* pL );
		static int32_t			GetNewIndexClosure( lua_State* pL );
		static int32_t			GetInstanceField( lua_State* pL );
		static int32_t			SetInstanceField( lua_State* pL );
		static int32_t			ClassCast( lua_State* pL );
		static int32_t			CallByLua( lua_State* pL );
		static int32_t			ErrorHandler( lua_State* pState );
		static int32_t			DebugBreak( lua_State* pState );
		static int32_t			BackTrace( lua_State* pState );
        static int32_t			ObjectGC( lua_State* pL );
        static int32_t			ObjectConstruct( lua_State* pL );
		static int32_t			LoadFile( lua_State* pL );
		static int32_t			DoFile( lua_State* pL );
		static int32_t			Panic( lua_State* pL );	
		static void*			Realloc( void* pContex, void* pPreBuff, size_t nOldSize, size_t nNewSize );	
		static int32_t			Print( lua_State* pL );
		static int32_t			ToString( lua_State* pL );

		static void				DebugHookProc( lua_State *pState, lua_Debug* pDebug );
		static bool				GetGlobObject( lua_State* pL, const char* szKey );
		static bool				SetGlobObject( lua_State* pL, const char* szKey );

		void					BuildRegisterInfo();
        void					AddLoader();
		void					IO_Replace();

		static void             RegistToLua( lua_State* pL, const CClassInfo* pInfo, void* pObj, 
									int32_t nObj, int32_t nGlobalWeakTable, int32_t nCppObjTable );

		virtual bool			CallVM( const CCallbackInfo* pCallBase, void* pRetBuf, void** pArgArray );
		virtual void			DestrucVM( const CCallbackInfo* pCallBase, SVirtualObj* pObject );

		virtual bool			Set( void* pObject, int32_t nIndex, void* pArgBuf, const STypeInfo& TypeInfo );
		virtual bool			Get( void* pObject, int32_t nIndex, void* pResultBuf, const STypeInfo& TypeInfo );
		virtual bool			Set( void* pObject, const char* szName, void* pArgBuf, const STypeInfo& TypeInfo );
		virtual bool			Get( void* pObject, const char* szName, void* pResultBuf, const STypeInfo& TypeInfo );
		virtual bool        	Call( const STypeInfoArray& aryTypeInfo, void* pResultBuf, const char* szFunction, void** aryArg );
		virtual bool        	Call( const STypeInfoArray& aryTypeInfo, void* pResultBuf, void* pFunction, void** aryArg );
		virtual bool        	RunBuffer( const void* pBuffer, size_t nSize, const char* szFileName, bool bForce = false );

		virtual void*			GetVM();

		friend class CDebugLua;
		friend class CLuaBuffer;

    public:
        CScriptLua(const char* strDebugHost, uint16_t nDebugPort = 0, bool bWaitForDebugger = false );
		~CScriptLua(void);
		//==============================================================================
		// built keys
		//==============================================================================
		static void*			ms_pGlobObjectWeakTable;
		static void*			ms_pGlobObjectTable;
		static void*			ms_pRegistScriptLua;
		static void*			ms_pFirstClassInfo;
		static void*			ms_pErrorHandler;

        //==============================================================================
        // common function
        //==============================================================================
        static void*			NewLuaObj( lua_State* pL, const CClassInfo* pInfo, int32_t nCppObjs );
		static void				RegisterObject( lua_State* pL, const CClassInfo* pInfo, 
									void* pObj, bool bGC, int32_t nCppObjStr, int32_t nWeakTable );
		static void				NewUnicodeString( lua_State* pL, const wchar_t* szStr );
		static const wchar_t*	ConvertUtf8ToUcs2(lua_State* pL, int32_t nStkId);

		static void				DumpLuaMem(const char* szFile, int nMinSize = 100);
		static void				ClearLuaMem();

		lua_State*              GetLuaState();
		void					PushLuaState( lua_State* pL );
		void					PopLuaState();
		void					SetDebugLine();

        static  CScriptLua*     GetScript( lua_State* pL );
		virtual int32_t			IncRef( void* pObj );
		virtual int32_t			DecRef( void* pObj );
		virtual void            UnlinkCppObjFromScript( void* pObj );
		virtual void        	GC();
		virtual void        	GCAll();
		virtual bool			IsValid( void* pObject );
	};
}

#endif
