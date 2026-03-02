#include "CCallInfo.h"
#include "CClassInfo.h"
#include "GammaScript/CScriptBase.h"

namespace Gamma
{
	//=====================================================================
	// 类型管理器
	//=====================================================================
	class CGlobalClassRegist
	{
		CGlobalClassRegist();
		~CGlobalClassRegist();
	public:
		static CGlobalClassRegist& GetInst();
		CTypeIDNameMap	m_mapTypeID2ClassInfo;
	};

	CGlobalClassRegist::CGlobalClassRegist()
	{
		m_mapTypeID2ClassInfo.Insert( *new CClassInfo( "" ) );
	}

	CGlobalClassRegist::~CGlobalClassRegist()
	{
		while( m_mapTypeID2ClassInfo.GetFirst() )
		{
			auto* pClassInfo = m_mapTypeID2ClassInfo.GetFirst();
			delete static_cast<CClassInfo*>( pClassInfo );
		}
	}

	inline CGlobalClassRegist& CGlobalClassRegist::GetInst()
	{
		static CGlobalClassRegist s_Instance;
		return s_Instance;
	}

	const CClassInfo* CClassInfo::RegisterClass(
		const char* szClassName, const char* szTypeIDName, uint32_t nSize, bool bEnum )
	{
		gammacstring strKey( szTypeIDName, true );
		CGlobalClassRegist& Inst = CGlobalClassRegist::GetInst();
		CClassInfo* pInfo = Inst.m_mapTypeID2ClassInfo.Find( strKey );
		if( !pInfo )
		{
			pInfo = new CClassInfo( szTypeIDName );
			Inst.m_mapTypeID2ClassInfo.Insert( *pInfo );
		}

		if( szClassName && szClassName[0] )
			pInfo->m_szClassName = szClassName;
		pInfo->m_bIsEnum = bEnum;
		GammaAst( pInfo->m_nSizeOfClass == 0 || pInfo->m_nSizeOfClass == nSize );
		pInfo->m_nSizeOfClass = nSize;
		pInfo->m_nAligenSizeOfClass = AligenUp( nSize, sizeof( void* ) );
		return pInfo;
	}

	const CClassInfo* CClassInfo::GetClassInfo( const char* szTypeInfoName )
	{
		gammacstring strKey( szTypeInfoName, true );
		CGlobalClassRegist& Inst = CGlobalClassRegist::GetInst();
		return Inst.m_mapTypeID2ClassInfo.Find( strKey );
	}

	const CClassInfo* CClassInfo::SetObjectConstruct( 
		const char* szTypeInfoName, IObjectConstruct* pObjectConstruct )
	{
		gammacstring strKey( szTypeInfoName, true );
		CGlobalClassRegist& Inst = CGlobalClassRegist::GetInst();
		GammaAst( Inst.m_mapTypeID2ClassInfo.Find( strKey ) );
		CClassInfo* pInfo = Inst.m_mapTypeID2ClassInfo.Find( strKey );
		pInfo->m_vecParamType.clear();
		pInfo->m_pObjectConstruct = pObjectConstruct;

		if( !pObjectConstruct )
			return pInfo;
		STypeInfoArray aryTypeInfo = pObjectConstruct->GetFunArg();

		pInfo->m_vecParamType.resize(aryTypeInfo.nSize - 1);
		pInfo->m_vecParamSize.resize(pInfo->m_vecParamType.size());
		for (size_t i = 0; i < pInfo->m_vecParamType.size(); i++)
		{
			pInfo->m_vecParamType[i] = ToDataType(aryTypeInfo.aryInfo[i]);
			pInfo->m_vecParamSize[i] = (uint32_t)GetAligenSizeOfType(pInfo->m_vecParamType[i]);
			pInfo->m_nTotalParamSize += pInfo->m_vecParamSize[i];
		}
		return pInfo;
	}

	const CClassInfo* CClassInfo::AddBaseInfo( 
		const char* szTypeInfoName, const char* szBaseTypeInfoName, ptrdiff_t nOffset )
	{
		CGlobalClassRegist& Inst = CGlobalClassRegist::GetInst();
		gammacstring strDeriveKey( szTypeInfoName, true );
		CClassInfo* pInfo = Inst.m_mapTypeID2ClassInfo.Find( strDeriveKey );
		gammacstring strBaseKey( szBaseTypeInfoName, true );
		CClassInfo* pBaseInfo = Inst.m_mapTypeID2ClassInfo.Find( strBaseKey );
		GammaAst( pInfo && pBaseInfo && nOffset >= 0 );
		SBaseInfo BaseInfo = { pBaseInfo, (int32_t)nOffset };
		if( pBaseInfo->m_nInheritDepth + 1 > pInfo->m_nInheritDepth )
			pInfo->m_nInheritDepth = pBaseInfo->m_nInheritDepth + 1;
		pInfo->m_vecBaseRegist.push_back( BaseInfo );

		BaseInfo.m_pBaseInfo = pInfo;
		BaseInfo.m_nBaseOff = -BaseInfo.m_nBaseOff;
		pBaseInfo->m_vecChildRegist.push_back( BaseInfo );

		if( nOffset )
			return pInfo;

		// 自然继承，虚表要延续
		auto& vecNewFunction = pBaseInfo->m_vecOverridableFun;
		for( int32_t i = 0; i < (int32_t)vecNewFunction.size(); i++ )
		{
			if( !vecNewFunction[i] )
				continue;
			GammaAst( vecNewFunction[i]->GetFunctionIndex() == i );
			RegisterCallBack( szTypeInfoName, i, vecNewFunction[i] );
		}
		return pInfo;
	}

	const Gamma::CCallInfo* CClassInfo::RegisterFunction(
		const char* szTypeInfoName, CCallInfo* pCallBase )
	{
		gammacstring strKey( szTypeInfoName, true );
		CGlobalClassRegist& Inst = CGlobalClassRegist::GetInst();
		CClassInfo* pInfo = Inst.m_mapTypeID2ClassInfo.Find( strKey );
		if( !pInfo )
			return nullptr;
		GammaAst( pInfo->m_mapRegistFunction.find( 
			pCallBase->GetFunctionName() ) == pInfo->m_mapRegistFunction.end() );
		pInfo->m_mapRegistFunction.Insert( *pCallBase );
		return pCallBase;
	}

	const CCallInfo* CClassInfo::RegisterCallBack(
		const char* szTypeInfoName, uint32_t nIndex, CCallbackInfo* pCallScriptBase )
	{
		gammacstring strKey( szTypeInfoName, true );
		CGlobalClassRegist& Inst = CGlobalClassRegist::GetInst();
		CClassInfo* pInfo = Inst.m_mapTypeID2ClassInfo.Find( strKey );
		if( !pInfo )
			return nullptr;
		// 不能重复注册
		if( nIndex >= pInfo->m_vecOverridableFun.size() )
			pInfo->m_vecOverridableFun.resize( nIndex + 1 );
		GammaAst( pInfo->m_vecOverridableFun[nIndex] == nullptr );
		pInfo->m_vecOverridableFun[nIndex] = pCallScriptBase;

		for( size_t i = 0; i < pInfo->m_vecChildRegist.size(); ++i )
		{
			if( pInfo->m_vecChildRegist[i].m_nBaseOff )
				continue;
			auto& strName = pInfo->m_vecChildRegist[i].m_pBaseInfo->GetTypeIDName();
			RegisterCallBack( strName.c_str(), nIndex, pCallScriptBase );
		}
		return pCallScriptBase;
	}

	const Gamma::CTypeIDNameMap& CClassInfo::GetAllRegisterInfo()
	{
		return CGlobalClassRegist::GetInst().m_mapTypeID2ClassInfo;
	}

	//=====================================================================
    // 类型的继承关系
    //=====================================================================
	CClassInfo::CClassInfo( const char* szTypeIDName )
		: m_szTypeIDName( szTypeIDName )
        , m_nSizeOfClass( 0 )
		, m_nAligenSizeOfClass( 0 )
        , m_pObjectConstruct( nullptr )
        , m_bIsEnum(false)
		, m_nInheritDepth(0)
		, m_nTotalParamSize(0)
	{
    }

    CClassInfo::~CClassInfo()
	{
		while( m_mapRegistFunction.GetFirst() )
			delete m_mapRegistFunction.GetFirst();
    }

    void CClassInfo::InitVirtualTable( SFunctionTable* pNewTable ) const
	{
		for( int32_t i = 0; i < (int32_t)m_vecOverridableFun.size(); i++ )
		{
			if( !m_vecOverridableFun[i] )
				continue;
			CCallbackInfo* pCallInfo = m_vecOverridableFun[i];
			GammaAst( pCallInfo->GetFunctionIndex() == i );
			pNewTable->m_pFun[i] = pCallInfo->GetBootFun();
		}
    }

    int32_t CClassInfo::GetMaxRegisterFunctionIndex() const
    {        
		return (int32_t)m_vecOverridableFun.size();
    }

    void CClassInfo::Construct( CScriptBase* pScript, void* pObject, void** aryArg ) const
	{
		pScript->CheckDebugCmd();
		//声明性质的类不可创建
		GammaAst( m_nSizeOfClass );
		GammaAst( m_pObjectConstruct );
		if( !m_pObjectConstruct )
			return;
		m_pObjectConstruct->Construct( pObject, aryArg );
	}

	void CClassInfo::CopyConstruct( CScriptBase* pScript, void* pDest, void* pSrc ) const
	{
		pScript->CheckDebugCmd();
		GammaAst( m_pObjectConstruct );
		if( !m_pObjectConstruct )
			return;
		m_pObjectConstruct->CopyConstruct( pDest, pSrc );
	}

    void CClassInfo::Destruct( CScriptBase* pScript, void* pObject ) const
	{
		pScript->CheckDebugCmd();
		//声明性质的类不可销毁
		GammaAst( m_pObjectConstruct );
		if( !m_pObjectConstruct )
			return;
		m_pObjectConstruct->Destruct( pObject );
	}

	void CClassInfo::Assign( CScriptBase* pScript, void* pDest, void* pSrc ) const
	{
		pScript->CheckDebugCmd();
		GammaAst( m_pObjectConstruct );
		if( !m_pObjectConstruct )
			return;
		m_pObjectConstruct->Assign( pDest, pSrc );
	}

	const CCallInfo* CClassInfo::GetCallBase( const gammacstring& strFunName ) const
	{
		return m_mapRegistFunction.Find( strFunName );
	}

    bool CClassInfo::IsCallBack() const
    {
		return !m_vecOverridableFun.empty();
    }

    int32_t CClassInfo::GetBaseOffset( const CClassInfo* pRegist ) const
    {
        if( pRegist == this )
            return 0;

        for( size_t i = 0; i < m_vecBaseRegist.size(); i++ )
        {
            int32_t nOffset = m_vecBaseRegist[i].m_pBaseInfo->GetBaseOffset( pRegist );
            if( nOffset >= 0 )
                return m_vecBaseRegist[i].m_nBaseOff + nOffset;
        }

        return -1;
	}

    void CClassInfo::ReplaceVirtualTable( CScriptBase* pScript,
		void* pObj, bool bNewByVM, uint32_t nInheritDepth ) const
    {
        SVirtualObj* pVObj        = (SVirtualObj*)pObj;
        SFunctionTable* pOldTable = pVObj->m_pTable;
        SFunctionTable* pNewTable = nullptr;

		if( !m_vecOverridableFun.empty() )
		{
			// 确保pOldTable是原始虚表，因为pVObj有可能已经被修改过了
			pOldTable = pScript->GetOrgVirtualTable( pVObj );
			pNewTable = pScript->CheckNewVirtualTable( pOldTable, this, bNewByVM, nInheritDepth );
		}

        for( size_t i = 0; i < m_vecBaseRegist.size(); i++ )
        {
            if( m_vecBaseRegist[i].m_pBaseInfo->IsCallBack() )
			{
				void* pBaseObj = ( (char*)pObj ) + m_vecBaseRegist[i].m_nBaseOff;
                m_vecBaseRegist[i].m_pBaseInfo->ReplaceVirtualTable( 
					pScript, pBaseObj, bNewByVM, nInheritDepth + 1 );
			}
        }

        if( pNewTable )
		{
			// 不允许不同的虚拟机共同使用同一份虚表
			GammaAst( pScript->IsVirtualTableValid(pVObj) );
            pVObj->m_pTable = pNewTable;
		}
    }

    void CClassInfo::RecoverVirtualTable( CScriptBase* pScript, void* pObj ) const
    {
        SFunctionTable* pOrgTable = nullptr;
        if( !m_vecOverridableFun.empty() )
            pOrgTable = pScript->GetOrgVirtualTable( pObj );

        for( size_t i = 0; i < m_vecBaseRegist.size(); i++ )
            m_vecBaseRegist[i].m_pBaseInfo->RecoverVirtualTable( 
				pScript, ( (char*)pObj ) + m_vecBaseRegist[i].m_nBaseOff );

        if( pOrgTable )
            ( (SVirtualObj*)pObj )->m_pTable = pOrgTable;
    }

    bool CClassInfo::FindBase( const CClassInfo* pRegistBase ) const
    {
        if( pRegistBase == this )
            return true;
        for( size_t i = 0; i < m_vecBaseRegist.size(); i++ )
            if( m_vecBaseRegist[i].m_pBaseInfo->FindBase( pRegistBase ) )
                return true;
        return false;
    }

	bool CClassInfo::IsBaseObject( ptrdiff_t nDiff ) const
	{
		// 命中基类
		if( nDiff == 0 )
			return true;

		// 超出基类内存范围
		if( nDiff > (int32_t)GetClassSize() )
			return false;

		for( size_t i = 0; i < m_vecBaseRegist.size(); i++ )
		{
			ptrdiff_t nBaseDiff = m_vecBaseRegist[i].m_nBaseOff;
			if( nDiff < nBaseDiff )
				continue;
			if( m_vecBaseRegist[i].m_pBaseInfo->IsBaseObject( nDiff - nBaseDiff ) )
				return true;
		}

		// 没有适配的基类
		return false;
	}
}
