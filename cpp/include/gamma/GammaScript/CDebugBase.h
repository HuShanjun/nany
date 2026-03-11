/**@file  		CDebugBase.h
* @brief		Debugger interface
* @author		Daphnis Kau
* @date			2019-06-24
* @version		V1.0
*/

#ifndef __DEBUG_BASE_H__
#define __DEBUG_BASE_H__
#include "GammaScript/GammaScriptDef.h"
#include "GammaCommon/TConstString.h"
#include "GammaCommon/TGammaList.h"
#include "GammaCommon/CJson.h"
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <thread>
#include <mutex>

namespace Gamma
{
	class CDebugCmd;
	class CBreakPoint;
	typedef std::set<CBreakPoint> CBreakPointList;
	typedef std::vector<std::string> CFileLines;
	typedef std::map<std::string, CFileLines> CFileMap;
	typedef std::map<std::string, std::string> CPathRedirectMap;
	typedef TGammaList<CDebugCmd> CDebugCmdList;
	typedef CDebugCmdList::CGammaListNode CDebugNode;
	class CDebugCmd : public CDebugNode, public CJson{};

	class IDebugHandler
	{
	public:
		virtual void*		GetVM() = 0;
		virtual int32_t		Input( char* szBuffer, int nCount ) = 0;
		virtual int32_t		Output( const char* szBuffer, int nCount, bool bError = false ) = 0;
		virtual void*		OpenFile( const char* szFileName ) = 0;
		virtual int32_t		ReadFile( void* pContext, char* szBuffer, int32_t nCount ) = 0;
		virtual void		CloseFile( void* pContext ) = 0;
		virtual bool		RunFile( const char* szFileName, bool bForce = false ) = 0;
	};

	struct SException 
	{ 
		const char* szException; 
		bool bBeCaught; 
	};

	struct SValueInfo 
	{ 
		SValueInfo( const char* szValue = "" ) 
			: nID( 0 )
			, nNameValues( 0 )
			, nIndexValues( 0 )
			, strValue( szValue ? szValue : "" )
		{}
		uint32_t		nID;
		std::string strName;
		std::string strValue; 
		uint32_t		nNameValues;
		uint32_t		nIndexValues;
	};

    class CBreakPoint : public gammacstring
	{
		uint32_t				m_nBreakPointID;
		uint32_t				m_nFileNameStart;
		uint32_t				m_nLineNum;
	public:
        CBreakPoint( uint32_t nID, const char* szFileName, bool bRef, uint32_t uLineNum );
        bool operator < ( const CBreakPoint& ano ) const;
		bool operator == ( const CBreakPoint& ano ) const;
		const char* GetModuleName() const { return c_str() + m_nFileNameStart; }
		uint32_t GetBreakPointID() const { return m_nBreakPointID; }
		uint32_t GetLineNum() const { return m_nLineNum; }
	};

    class GAMMA_SCRIPT_API CDebugBase
	{
	protected:
		enum EAttachType
		{ 
			eAT_Detach, 
			eAT_Waiting, 
			eAT_Launch, 
			eAT_Attach 
		};

		IDebugHandler*		m_pHandler;
		std::thread			m_hThread;
		std::mutex			m_hCmdLock;
		intptr_t			m_nRemoteListener;
		intptr_t			m_nRemoteConnecter;
		CDebugCmdList		m_listDebugCmd;

		char*				m_pBuf;
		char				m_szBuffer[1024];
		CPathRedirectMap	m_mapRedirectPath;
		CBreakPointList		m_setBreakPoint;
		CFileMap			m_mapFileBuffer;

		EAttachType			m_eAttachType;
		bool				m_bQuit;
		bool				m_bLoopOnPause;
		bool				m_bAllExceptionsBreak;
		bool				m_bUncaughtExceptionsBreak;
		bool				m_bPrintFrame;
		bool				m_bEnterDebug;
		bool				m_bExpectStep;
		uint32_t				m_nExceptionID;
		std::string			m_szException;
		std::string			m_szStringSend;
		
		int32_t				m_nCurFrame;
		int32_t				m_nCurLine;
		int32_t				m_nShowLine;
		std::string			m_strLastVarName;

		//=================================================================
		// ConsoleDebug
		//=================================================================
		void				ConsoleDebug( SException* pException );
		const char*			ReadWord( bool bNewLine = false );
		bool				PrintLine( int32_t nFrame, const char* szSource, int32_t nLine, bool bIsCurLine );
		void				PrintFrame( int32_t nFrame, const char* szFun, const char* szSource, int32_t nLine );
		const char*			ReadFileLine( const char* szSource, int32_t nLine );

		void				AddBreakPoint( const char* szBuf );
		void				PrintBreakInfo();
		void				ShowFileLines( int32_t nLineCount );

		//=================================================================
		// RemoteDebug
		// https://microsoft.github.io/debug-adapter-protocol/
		//=================================================================
		void				RemoteDebug( SException* pException );
		void				CmdLock();
		void				CmdUnLock();
		void				ListenRemote( const char* strDebugHost, uint16_t nDebugPort );
		void				TeminateRemote( const char* szSequence );
		void				Run();

		void				SendEvent( CJson* pBody, const char* szEvent );
		void				SendRespone( CJson* pBody, const char* szSequence, 
								bool bSucceeded, const char* szCommand, const char* szMsg = "" );
		void				SendNetData( CJson* pJson );
		void				OnNetData( CDebugCmd* pCmd );
		virtual bool		ProcessCommand( CDebugCmd* pCmd );

		//=================================================================
		// Implement other debug protocol by override next two function
		//=================================================================
		virtual bool		ReciveRemoteData(char(&szBuffer)[2048], int32_t nCurSize);
		virtual bool		CheckRemoteCmd();

	protected:
		IDebugHandler*		GetDebugHandler() const { return m_pHandler; }
		virtual uint32_t		GenBreakPointID( const char* szFileName, int32_t nLine ) = 0;
		bool				HaveBreakPoint() const { return !m_setBreakPoint.empty(); }
		std::string			ReadEntirFile( const char* szFileName );

    public:
        CDebugBase( IDebugHandler* pHandler, const char* strDebugHost, uint16_t nDebugPort );
		virtual ~CDebugBase(void);

		void				Debug();
		bool				Error( const char* szException, bool bBeCaught );
		void				BTrace( int32_t nFrameCount );

		void				AddFileContent( const char* szSource, const char* szData );
		bool				HasLoadFile(const char* szFile);
		bool				RemoteDebugEnable() const;
		bool				RemoteCmdValid() const { return !m_listDebugCmd.IsEmpty(); }
		bool				CheckEnterRemoteDebug();
		int32_t				GetDebuggerState();

		virtual uint32_t		AddBreakPoint( const char* szFileName, int32_t nLine );
		virtual void		DelBreakPoint( uint32_t nBreakPointID );
		const CBreakPoint*	GetBreakPoint( const char* szSource, int32_t nLine );

		virtual uint32_t		GetFrameCount() = 0;
		virtual bool		GetFrameInfo( int32_t nFrame, int32_t* nLine, 
								const char** szFunction, const char** szSource ) = 0;
		virtual int32_t		SwitchFrame( int32_t nCurFrame ) = 0;
		virtual uint32_t		EvaluateExpression( int32_t nCurFrame, const char* szExpression ) = 0;
		virtual uint32_t		GetScopeChainID( int32_t nCurFrame ) = 0;
		virtual uint32_t		GetChildrenID( uint32_t nParentID, bool bIndex, uint32_t nStart, 
								uint32_t* aryChild = nullptr, uint32_t nCount = 0 ) = 0;
		virtual SValueInfo	GetVariable( uint32_t nID ) = 0;
		virtual void		Stop() = 0;
		virtual void		Continue() = 0;
		virtual void		StepIn() = 0;
		virtual void		StepNext() = 0;
		virtual void		StepOut() = 0;
    };
}

#endif
