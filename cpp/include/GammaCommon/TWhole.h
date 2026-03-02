#pragma once

#include <list>
namespace Gamma
{
	template<typename ImpPartClass,typename ImpWholeClass>
	class TPart;

	template<typename ImpWholeClass,typename ImpPartClass>
	class TWhole
	{

	public:
		typedef TPart<ImpPartClass,ImpWholeClass>	Part_t;
		typedef std::list<ImpPartClass*>			ImpParts_t;

	private:
		friend class TPart<ImpPartClass,ImpWholeClass>;
		
		ImpParts_t	m_ImpParts;

	protected:
		void ClearParts();

	public:
		TWhole(void);
		~TWhole(void);

		const ImpParts_t& GetParts()const;
	};
}

#include "GammaCommon/TWhole.inl"
