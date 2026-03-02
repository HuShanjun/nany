#pragma once

namespace Gamma
{
	template<typename ImpPartClass,typename ImpWholeClass>
	TPart<ImpPartClass,ImpWholeClass>::TPart(ImpWholeClass* pWhole)
		:m_pWhole(pWhole)
	{
		_Whole()->m_ImpParts.push_front(static_cast<ImpPartClass*>(this));
		m_itWhole = _Whole()->m_ImpParts.begin();
	}

	template<typename ImpPartClass,typename ImpWholeClass>
	TPart<ImpPartClass,ImpWholeClass>::~TPart(void)
	{
		_Whole()->m_ImpParts.erase(m_itWhole);
	}

	template<typename ImpPartClass,typename ImpWholeClass>
	ImpWholeClass* TPart<ImpPartClass,ImpWholeClass>::GetWhole()const
	{
		return m_pWhole;
	}

	template<typename ImpPartClass,typename ImpWholeClass>
	typename TPart<ImpPartClass,ImpWholeClass>::Whole_t* 
		TPart<ImpPartClass,ImpWholeClass>::_Whole()const
	{
		return m_pWhole;
	}

	template<typename ImpPartClass,typename ImpWholeClass>
	void TPart<ImpPartClass,ImpWholeClass>::_Delete()
	{
		delete static_cast<ImpPartClass*>(this);
	}
}

