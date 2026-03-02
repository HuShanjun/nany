//==============================================================
// BinaryFind.h 
// 定义对已经排好序的vector进行二叉查找
// 柯达昭
// 2007-09-15
//==============================================================

#ifndef _GAMMA_BINARY_FIND_H_
#define _GAMMA_BINARY_FIND_H_

#include "GammaCommon/GammaHelp.h"

namespace Gamma
{
    //=====================================================================
    //   void GetBound( const VectorType& vecKey, const KeyType& nPos, 
    //                    IntType& nBegin, IntType& nEnd )
    //   在已经排好序的vector查找前后两个相邻的值，使得nPos在这两个值之间［nBegin, nEnd）
    //   返回的区间左闭右开
	//	 当nPos不存在vector中任何两个值间（vector为空或者nPos小于vector的第一个元素）时返回false
	//	 当nPos在vector中某两个值间或者大于等于vector最后一个元素返回true。
	//   nPos大于等于vector最后一个元素时，返回的nBegin = nVectorSize - 1， nEnd = nVectorSize
    //=====================================================================
    template<class VectorType, class KeyType, class IntType, class CompareType > 
    bool GetBound( const VectorType& vecKey, size_t nVectorSize, const KeyType& nPos, IntType& nBegin, IntType& nEnd, const CompareType& Compare )
	{
		nBegin = nEnd = 0;
		if( nVectorSize == 0 )
			return false;

		if( Compare( nPos, vecKey[0] ) )
			return false;

        nEnd = (IntType)nVectorSize;
        while( nBegin != nEnd )
        {
            IntType nCur = (IntType)( ( nBegin + nEnd ) >> 1 );
            if( nCur == nBegin )
                return true;

            if( Compare( nPos, vecKey[nCur] ) )
                nEnd = nCur;
            else
            {
                nBegin = nCur;
                if( !Compare( vecKey[nCur], nPos ) )
                {
                    nEnd = (IntType)( nCur + 1 );
                    return true;
                }
            }
        }

		return true;
	}

	template<class VectorType, class KeyType, class IntType > 
	bool GetBound( const VectorType& vecKey, size_t nVectorSize, const KeyType& nPos, IntType& nBegin, IntType& nEnd )
	{
		return GetBound( vecKey, nVectorSize, nPos, nBegin, nEnd, std::less<KeyType>() );
	}

    //=====================================================================
    //   int32 GetBound( const VectorType& vecNode, const KeyType& KeyVal )
    //   查找某个值在vector中的索引
    //   返回vector中的索引，-1表示没找到
    //=====================================================================
    template< class VectorType, class KeyType, class CompareType > 
    int32 Find( const VectorType& vecNode, size_t nVectorSize, const KeyType& KeyVal, const CompareType& Compare )
	{
        if( nVectorSize == 0 )
            return -1;

        int32 nBegin = 0;
        int32 nEnd = (int32)nVectorSize;

        for( ;; )
        {
            if( nBegin + 1 == nEnd )
			{
				if( !( Compare( vecNode[nBegin], KeyVal ) || Compare( KeyVal, vecNode[nBegin] ) ) )
					return nBegin;
                return -1;
			}

            int nCur = ( nBegin + nEnd ) >> 1;
            if( !( Compare( vecNode[nCur], KeyVal ) || Compare( KeyVal, vecNode[nCur] ) ) )
                return nCur;        
            if( Compare( KeyVal, vecNode[nCur] ) )
                nEnd = nCur;
            else
                nBegin = nCur;
        }
    }

	template< class VectorType, class KeyType > 
	int32 Find( const VectorType& vecNode, size_t nVectorSize, const KeyType& KeyVal )
	{
		return Find( vecNode, nVectorSize, KeyVal, std::less<KeyType>() );
	}

	//=========================================================================
	// 根据fPos以及fPos所处区间对对线性排列数组进行插值
	//=========================================================================
	template< class DataType, class KeyType >
	inline DataType Interpolate( const DataType* vecData, const KeyType* vecKey,
		float fPos, std::pair<size_t,size_t> nBeginEnd )
	{
		if( nBeginEnd.first == nBeginEnd.second )
			return (DataType)vecData[nBeginEnd.first];
		float fWeight = ( vecKey[nBeginEnd.second] - fPos )/( vecKey[nBeginEnd.second] - vecKey[nBeginEnd.first] );
		return (DataType)( vecData[nBeginEnd.first]*fWeight + vecData[nBeginEnd.second]*( 1 - fWeight ) );
	}
}

#endif
