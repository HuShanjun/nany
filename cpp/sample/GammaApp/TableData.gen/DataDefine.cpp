
#include "DataDefine.h"
#include "GammaCommon/GammaCpp.h"
#include "GammaCommon/CPkgFile.h"
#include "GammaApp/IGameApp.h"

extern IGameAppPtr g_pApp;

CPlayerDataDefineShm::CPlayerDataDefineShm()
{
	m_nChunkHeadSize = GetClassMemberOffset(CPlayerDataDefineShm, m_BlockInfo);
	m_nChunckID = INVALID_64BITID;
	m_nBlockCount = ePP_Count;

	SetBlockInfo(m_BlockInfo[ePP_SevenDayLogin], m_SevenDayLogin, ePP_SevenDayLogin, 0, 0, 0);
	SetBlockInfo(m_BlockInfo[ePP_CharBase], m_CharBase, ePP_CharBase, 0, 0, 0);
	SetBlockInfo(m_BlockInfo[ePP_BagItem], m_BagItem, ePP_BagItem,
		(const tbyte*)&m_BagItem.GetBagItemSize(), m_BagItem.GetBagItemMaxCount(), m_BagItem.GetBagItemElemSize());
}

CPlayerDataDefine::CPlayerDataDefine(int64_t uuid, CPlayerDataDefineShm* pShm)
	: CDataContainer(uuid)
	, m_pDBDefine( pShm )
	, m_pSevenDayLogin(nullptr)
	, m_pCharBase(nullptr)
	, m_pBagItem(nullptr)
{
	BindShm( pShm );
}

CPlayerDataDefine::~CPlayerDataDefine()
{
	if( m_pDBDefine )
		return;

	SAFE_DELETE(m_pSevenDayLogin)
	SAFE_DELETE(m_pCharBase)
	SAFE_DELETE(m_pBagItem)
}

void CPlayerDataDefine::BindShm(CPlayerDataDefineShm* pShm)
{
	m_pSevenDayLogin = pShm ? &pShm->m_SevenDayLogin : new CSevenDayLogin();
	m_pSevenDayLogin->SetDataContainter(this);

	m_pCharBase = pShm ? &pShm->m_CharBase : new CCharBase();
	m_pCharBase->SetDataContainter(this);

	m_pBagItem = pShm ? &pShm->m_BagItem : new CBagItemArray();
	m_pBagItem->SetDataContainter(this);
}

bool CPlayerDataDefine::InitFrShm(CPlayerDataDefineShm* pShm)
{
	if( !pShm || m_pDBDefine == pShm )
		return false;

	*m_pSevenDayLogin = pShm->m_SevenDayLogin;
	*m_pCharBase = pShm->m_CharBase;
	*m_pBagItem = pShm->m_BagItem;
	return true;
}

void CPlayerDataDefine::UnPack(void* _pBuffer, uint32_t _nSize, uint32_t _nPos)
{
	CBufFileEx buffFile((tbyte*)_pBuffer, _nSize);
	buffFile.Seek(_nPos);
	m_pSevenDayLogin->UnPack(buffFile);
	m_pCharBase->UnPack(buffFile);
	m_pBagItem->UnPack(buffFile);
}

void CPlayerDataDefine::CopyFrom(CPlayerDataDefine* pOther, bool bSimple)
{
	m_pSevenDayLogin->CopyFrom(pOther->m_pSevenDayLogin, bSimple );
	m_pCharBase->CopyFrom(pOther->m_pCharBase, bSimple );
	m_pBagItem->CopyFrom(pOther->m_pBagItem, bSimple );
}

uint32_t CPlayerDataDefine::Pack(void* _pBuffer, uint32_t _nSize)
{
	CBufFileEx buffFile((tbyte*)_pBuffer, _nSize);
	uint32_t nOldPos = buffFile.GetPos();
	m_pSevenDayLogin->Pack(buffFile);
	m_pCharBase->Pack(buffFile);
	m_pBagItem->Pack(buffFile);
	return buffFile.GetPos() - nOldPos;
}

bool CPlayerDataDefine::SynFieldData(uint8_t synType, uint8_t nTblID, int64_t nDataUUID, uint16_t nFiledID, const void* pData, int32_t nLen, int32_t nPos )
{
	switch(nTblID)
	{
	case ePP_SevenDayLogin: return m_pSevenDayLogin->SynField(synType, nFiledID, pData, nLen, nPos);
	case ePP_CharBase: return m_pCharBase->SynField(synType, nFiledID, pData, nLen, nPos);
	case ePP_BagItem: return m_pBagItem->SynField(synType, nFiledID, pData, nLen, nPos);
	default: return false;
	}
}

void CPlayerDataDefine::SynToOther()
{
	m_pSevenDayLogin->SynToOther();
	m_pCharBase->SynToOther();
	m_pBagItem->SynToOther();
}