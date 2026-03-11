#pragma once

namespace Gamma
{
	template< typename ImpWholeClass, typename ImpPartClass >
		TWhole< ImpWholeClass, ImpPartClass >::TWhole(void)
	{
	}

	template< typename ImpWholeClass, typename ImpPartClass >
		TWhole< ImpWholeClass, ImpPartClass >::~TWhole(void)
	{
		ClearParts();
	}

	template< typename ImpWholeClass, typename ImpPartClass >
		void TWhole< ImpWholeClass, ImpPartClass >::ClearParts(void)
	{
		while( !m_ImpParts.empty() )
			static_cast<Part_t*>( *m_ImpParts.begin() )->_Delete();
	}

	template< typename ImpWholeClass, typename ImpPartClass >
		const typename TWhole< ImpWholeClass, ImpPartClass >::ImpParts_t&
		TWhole< ImpWholeClass, ImpPartClass >::GetParts(void)const
	{
		return m_ImpParts;
	}
}
