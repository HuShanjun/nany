#pragma once
#include "GammaShm/CBaseShareMemory.h"
#include "TableCommonDefine.h"
#include "PlayerDataTable.h"

#pragma pack(push,1)

class CPlayerDataDefineShm : public Gamma::SChunckHead
{
public:
	CPlayerDataDefineShm();
	Gamma::SBlockInfo	m_BlockInfo[ePP_Count];

	CSevenDayLogin		m_SevenDayLogin;
	CCharBase			m_CharBase;
	CBagItemArray		m_BagItem;

	CSevenDayLogin*		GetSevenDayLogin()	{ return &m_SevenDayLogin; }
	CCharBase*			GetCharBase()		{ return &m_CharBase; }
	CBagItemArray*		GetBagItem()		{ return &m_BagItem; }

	inline void SetZeroValue()
	{
		m_SevenDayLogin.SetZeroValue();
		m_CharBase.SetZeroValue();
		m_BagItem.SetZeroValue( true );
	}
};

#pragma pack(pop)

class CPlayerDataDefine : public Gamma::CDataContainer
{
public:
	CPlayerDataDefine( int64_t uuid, CPlayerDataDefineShm* pShm );
	~CPlayerDataDefine();
	uint8_t GetDataDefineType() const { return eDDT_Player; }
	static const char* GetDataDefineName() { return "CPlayerDataDefine"; }
	void BindShm(CPlayerDataDefineShm* pShm);
	bool InitFrShm(CPlayerDataDefineShm* pShm);
	void UpdateDB( uint8_t _eCTType, uint16_t _nTableID, int64_t nDataUUID, uint16_t _nFieldIndex) {}
	void ClearData( bool bDelDB ) {}
	virtual void OnDBResult( uint8_t _eCTType, uint16_t _nTableID, int64_t nDataUUID ) {}
	void CopyFrom(CPlayerDataDefine* pOther, bool bSimple = false );

private:
	CPlayerDataDefineShm*	m_pDBDefine;
	CSevenDayLogin*			m_pSevenDayLogin;
	CCharBase*				m_pCharBase;
	CBagItemArray*			m_pBagItem;

public:
	uint32_t GetMaxPackSize()	{ return sizeof(CPlayerDataDefineShm); }
	CPlayerDataDefineShm* GetDBDefine() const { return m_pDBDefine; }

	CSevenDayLogin* GetSevenDayLogin() { return m_pSevenDayLogin; }
	CSevenDayLogin* SetSevenDayLogin(CSevenDayLogin* pData ) { if(pData) *m_pSevenDayLogin = *pData; return m_pSevenDayLogin; }

	CCharBase* GetCharBase() { return m_pCharBase; }
	CCharBase* SetCharBase(CCharBase* pData ) { if(pData) *m_pCharBase = *pData; return m_pCharBase; }

	CBagItemArray* GetBagItem() { return m_pBagItem; }
	CBagItemArray* SetBagItem(CBagItemArray* pData ) { if(pData) *m_pBagItem = *pData; return m_pBagItem; }

	uint32_t Pack(void* _pBuffer, uint32_t _nSize);
	void   UnPack(void* _pBuffer, uint32_t _nSize, uint32_t _nPos);
	bool SynFieldData(uint8_t synType, uint8_t nTblID, int64_t nDataUUID, uint16_t nFiledID, const void* pData, int32_t nLen, int32_t nPos );
	void SynToOther();
};
