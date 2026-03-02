
#include "CDbResult.h"
#include "GammaCommon/GammaHelp.h"

namespace Gamma
{
    CDbResult::CDbResult(MYSQL_RES* pRes)
        :m_pRes(pRes),m_Row(NULL),m_pTag(NULL)
    {
        GammaAst(pRes);
    }

    CDbResult::~CDbResult()
    {
        mysql_free_result(m_pRes);
    }


    uint32 CDbResult::GetRowNum()
    {
        return static_cast<uint32>(mysql_num_rows(m_pRes));
    }

    uint32 CDbResult::GetColNum()
    {
        return mysql_num_fields(m_pRes);
    }

	void CDbResult::Locate( uint32 nIndex )
	{
		GammaAst( nIndex < GetRowNum() );
		mysql_data_seek( m_pRes, nIndex );
		m_Row = mysql_fetch_row( m_pRes );
		m_Lengths = mysql_fetch_lengths( m_pRes );
	}

    const char* CDbResult::GetData(uint32 uColIndex)
    {
        GammaAst(GetRowNum()>0);
        GammaAst(uColIndex<GetColNum());
        return m_Row[uColIndex];
    }

    unsigned CDbResult::GetDataLength(uint32 uColIndex)
    {
        GammaAst(uColIndex<GetColNum());
        return m_Lengths[uColIndex];
    }

    void CDbResult::Release()
    {
        delete this;
    }

    void CDbResult::SetTag(void* pTag)
    {
        m_pTag = pTag;
    }

    void* CDbResult::GetTag()const
    {
        return m_pTag;
    }
}
