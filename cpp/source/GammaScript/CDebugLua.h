/**@file  		CLuaDebug.h
* @brief		LUA debugger interface
* @author		Daphnis Kau
* @date			2019-06-24
* @version		V1.0
*/

#ifndef __LUA_DEBUG_H__
#define __LUA_DEBUG_H__
#include "GammaCommon/TGammaRBTree.h"
#include "GammaScript/CDebugBase.h"
#include <vector>

struct lua_State;
struct lua_Debug;
struct lua_TValue;

namespace Gamma
{
    class CDebugLua : public CDebugBase
	{
		struct SFieldInfo;
		struct SVariableInfo;
		typedef TGammaRBTree<SFieldInfo> CFieldMap;
		typedef TGammaRBTree<SVariableInfo> CVariableMap;

		struct SFieldInfo : public CFieldMap::CGammaRBTreeNode
		{
			gammacstring	m_strField;
			uint32_t			m_nRegisterID;
			operator const gammacstring&( ) const { return m_strField; }
			bool operator < ( const gammacstring& strKey ) { return m_strField < strKey; }
		};

		struct SVariableInfo : public CVariableMap::CGammaRBTreeNode
		{
			uint32_t			m_nParentID;
			uint32_t			m_nVariableID;
			CFieldMap		m_mapFields[2];
			operator const uint32_t( ) const { return m_nVariableID; }
			bool operator < ( uint32_t nKey ) { return (uint32_t)*this < nKey; }
		};

		struct SVariableNode : public SFieldInfo, public SVariableInfo {};

        lua_State*			m_pState;
		lua_State*			m_pPreState;
        int32_t				m_nBreakFrame;

		uint32_t				m_nValueID;
		CVariableMap		m_mapVariable;
		std::string			m_szFunctionName;
		std::string			m_strLastSorece;
		int32_t				m_nLastLine;

        static void			DebugHook( lua_State *pState, lua_Debug* pDebug );
		static CDebugLua*	GetDebugger( lua_State* pState );
		void				Debug( lua_State* pState );

		void				ClearVariables();
		uint32_t				TouchVariable( const char* szField, uint32_t nParentID );
		virtual uint32_t		GenBreakPointID( const char* szFileName, int32_t nLine );
		const char*			PresentValue( void* pValue );

    public:
        CDebugLua( IDebugHandler* pHandler, const char* strDebugHost, uint16_t nDebugPort );
        ~CDebugLua(void);

		void				SetCurState( lua_State* pL );
		
		virtual uint32_t		AddBreakPoint( const char* szFileName, int32_t nLine );
		virtual void		DelBreakPoint( uint32_t nBreakPointID );
		virtual uint32_t		GetFrameCount();
		virtual bool		GetFrameInfo( int32_t nFrame, int32_t* nLine, 
								const char** szFunction, const char** szSource );
		virtual int32_t		SwitchFrame( int32_t nCurFrame );
		virtual uint32_t		EvaluateExpression( int32_t nCurFrame, const char* szExpression );
		virtual uint32_t		GetScopeChainID( int32_t nCurFrame );
		virtual uint32_t		GetChildrenID( uint32_t nParentID, bool bIndex, uint32_t nStart, 
								uint32_t* aryChild = nullptr, uint32_t nCount = INVALID_32BITID );
		virtual SValueInfo	GetVariable( uint32_t nID );
		virtual void		Stop();
		virtual void		Continue();
		virtual void		StepIn();
		virtual void		StepNext();
		virtual void		StepOut();

		void				StackDump( const char *szTitle, std::ostream& out );
    };
}

#endif
