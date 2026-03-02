#pragma once
#include <list>

namespace Gamma
{
	template<typename ImpWholeClass,typename ImpPartClass>
	class TWhole;

	template<typename ImpPartClass,typename ImpWholeClass>
	class TPart
	{
	private:
		typedef TWhole<ImpWholeClass,ImpPartClass>		Whole_t;
		typedef std::list<ImpPartClass*>				ImpParts_t;
		typedef typename ImpParts_t::iterator			ItWhole_t;
		friend class TWhole<ImpWholeClass,ImpPartClass>;

		ImpWholeClass*	m_pWhole;
		ItWhole_t		m_itWhole;
		Whole_t*		_Whole() const;
		void			_Delete();

	protected:
		ImpWholeClass*	GetWhole()const;

	public:
		TPart(ImpWholeClass* pWhole);
		~TPart(void);
	};
}

#include "GammaCommon/TPart.inl"
