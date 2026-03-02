#pragma once
#include <memory.h>

namespace Gamma
{
	template<typename T>
	bool BinaryEqual( T& left, T& right )
	{
		return !memcmp( &left, &right, sizeof(right) );
	}

	// 求两个数组的差异，差异部分由三个数组返回
	// 三个返回数组的大小有函数调用者保证，
	// 必须大于两个输入数组大小之和
	template<typename iterator_const_bidirectional>
	void ArrayDiff( 
		iterator_const_bidirectional itOldBegin, iterator_const_bidirectional itOldEnd, 
		iterator_const_bidirectional itNewBegin, iterator_const_bidirectional itNewEnd, 
		iterator_const_bidirectional* itOutputNewArray,		size_t& nNewArraySize, 
		iterator_const_bidirectional* itOutputDeleteArray,	size_t& nDeleteArraySize, 
		iterator_const_bidirectional* itOutputReplaceArray,	size_t& nReplaceArraySize )
	{
		iterator_const_bidirectional	itBegin[2]	= { itOldBegin,				itNewBegin };
		iterator_const_bidirectional	itEnd[2]	= { itOldEnd,				itNewEnd };
		iterator_const_bidirectional*	itSort[2]	= { itOutputDeleteArray,	itOutputNewArray };
		size_t nCount[2]							= { 0, 0 };

		/// 将道具按照id排序
		for( size_t n = 0; n < 2; n++ )
		{
			for( iterator_const_bidirectional it = itBegin[n]; it != itEnd[n]; it++, nCount[n]++ )
			{
				size_t j = nCount[n];
				while( j > 0 && *it < *( itSort[n][ j - 1 ] ) )
				{
					itSort[n][j] = itSort[n][ j - 1 ];
					j--;
				}
				itSort[n][j] = it;
			}
		}


		/// 筛选出需要更新和删除的道具
		nDeleteArraySize = 0;
		nReplaceArraySize = 0;
		nNewArraySize = 0;

		size_t i = 0;
		size_t j = 0;
		while( i < nCount[0] && j < nCount[1] )
		{
			if( *itOutputDeleteArray[i] < *itOutputNewArray[j] )
			{
				itOutputDeleteArray[nDeleteArraySize++] = itOutputDeleteArray[i++];
			}
			else if( *itOutputNewArray[j] < *itOutputDeleteArray[i] )
			{
				itOutputNewArray[nNewArraySize++] = itOutputNewArray[j++];
			}
			else
			{
				if( !BinaryEqual( *itOutputNewArray[j], *itOutputDeleteArray[i] ) )
					itOutputReplaceArray[nReplaceArraySize++] = itOutputNewArray[j];
				j++;
				i++;
			}
		}

		while( i < nCount[0] )
			itOutputDeleteArray[nDeleteArraySize++] = itOutputDeleteArray[i++];

		while( j < nCount[1] )
			itOutputNewArray[nNewArraySize++] = itOutputNewArray[j++];
	};
}
