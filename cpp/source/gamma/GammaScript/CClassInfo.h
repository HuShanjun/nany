/**@file  		CClassInfo.h
* @brief		Register information of class 
* @author		Daphnis Kau
* @date			2019-06-24
* @version		V1.0
*/

#ifndef __CLASS_REGIST_INFO_H__
#define __CLASS_REGIST_INFO_H__

#include "GammaCommon/TGammaRBTree.h"
#include "GammaCommon/CVirtualFun.h"
#include "GammaCommon/TConstString.h"
#include "GammaScript/GammaScriptDef.h"
#include "CTypeBase.h"
#include <string>
#include <vector>
#include <map>

namespace Gamma
{
	class CTypeBase;
	class CScriptBase;
	class CCallInfo;
	class CCallbackInfo;
	class CClassInfo;
	class CGlobalClassRegist;
	typedef TGammaRBTree<CCallInfo> CCallBaseMap;
	typedef TGammaRBTree<CClassInfo> CTypeIDNameMap;

    class CClassInfo : public CTypeIDNameMap::CGammaRBTreeNode
	{
		struct SBaseInfo
		{
			const CClassInfo*			m_pBaseInfo;        // Base class information
			int32_t						m_nBaseOff;         // Base class offset
		};

		gammacstring					m_szClassName;		// Name of class
		gammacstring					m_szTypeIDName;		// typeid of the class
		std::vector<DataType>			m_vecParamType;		// Parameter of constructor
		std::vector<uint32_t>				m_vecParamSize;		// Size of Parameters
		uint32_t							m_nTotalParamSize;	// Total size of Parameters

		std::vector<CCallbackInfo*>		m_vecOverridableFun;// Overridable function information
		std::vector<SBaseInfo>			m_vecBaseRegist;    // All base classes' information
		std::vector<SBaseInfo>			m_vecChildRegist;   // All subclass information
        IObjectConstruct*				m_pObjectConstruct;
		uint32_t							m_nSizeOfClass;
		uint32_t							m_nAligenSizeOfClass;
		bool							m_bIsEnum;
		uint8_t							m_nInheritDepth;
		CCallBaseMap					m_mapRegistFunction;
		
		friend class CGlobalClassRegist;
		CClassInfo(const char* szClassName);
		~CClassInfo( void );
    public:

		operator const gammacstring&( ) const { return m_szTypeIDName; }
		bool operator < ( const gammacstring& strKey ) { return (const gammacstring&)*this < strKey; }

		static const CClassInfo*		RegisterClass( const char* szClassName, const char* szTypeIDName, uint32_t nSize, bool bEnum );
		static const CClassInfo*		GetClassInfo( const char* szTypeInfoName );
		static const CClassInfo*		SetObjectConstruct( const char* szTypeInfoName, IObjectConstruct* pObjectConstruct );
		static const CClassInfo*		AddBaseInfo( const char* szTypeInfoName, const char* szBaseTypeInfoName, ptrdiff_t nOffset );
		static const CCallInfo*			RegisterFunction( const char* szTypeInfoName, CCallInfo* pCallBase );
		static const CCallInfo*			RegisterCallBack( const char* szTypeInfoName, uint32_t nIndex, CCallbackInfo* pCallScriptBase );
		static const CTypeIDNameMap&	GetAllRegisterInfo();

		const std::vector<DataType>&	GetConstructorParamType() const { return m_vecParamType; }
		const std::vector<uint32_t>&		GetConstructorParamSize() const { return m_vecParamSize; }
		uint32_t							GetConstructorParamTotalSize() const { return m_nTotalParamSize; }
		void							Construct( CScriptBase* pScript, void* pObject, void** aryArg ) const;
		void							CopyConstruct( CScriptBase* pScript, void* pDest, void* pSrc ) const;
		void                        	Destruct( CScriptBase* pScript, void * pObject ) const;
		void							Assign( CScriptBase* pScript, void* pDest, void* pSrc ) const;

		void							InitVirtualTable( SFunctionTable* pNewTable ) const;
		int32_t							GetMaxRegisterFunctionIndex() const;
        void                            ReplaceVirtualTable( CScriptBase* pScript, void* pObj, bool bNewByVM, uint32_t nInheritDepth ) const;
		void                            RecoverVirtualTable( CScriptBase* pScript, void* pObj ) const;
		bool                            IsCallBack() const;
		int32_t                       	GetBaseOffset( const CClassInfo* pRegist ) const;
		const CCallInfo*				GetCallBase( const gammacstring& strFunName ) const;
        bool                            FindBase( const CClassInfo* pRegistBase ) const;
		bool							IsBaseObject(ptrdiff_t nDiff) const;
		bool							IsEnum() const { return m_bIsEnum; }
		const std::vector<SBaseInfo>&  	BaseRegist() const { return m_vecBaseRegist; }
		const gammacstring&            	GetTypeIDName() const { return m_szTypeIDName; }
		const gammacstring&            	GetClassName() const { return m_szClassName; }
		const gammacstring&            	GetObjectIndex() const { return m_szTypeIDName; }
		uint32_t                          GetClassSize() const { return m_nSizeOfClass; }
		uint32_t                          GetClassAligenSize() const { return m_nAligenSizeOfClass; }
		uint8_t							GetInheritDepth() const { return m_nInheritDepth; }
		const CCallBaseMap&				GetRegistFunction() const { return m_mapRegistFunction; }
		const CCallbackInfo*			GetOverridableFunction( int32_t nIndex ) const { return m_vecOverridableFun[nIndex]; }
    }; 
}                                            
                                            
#endif                                        
