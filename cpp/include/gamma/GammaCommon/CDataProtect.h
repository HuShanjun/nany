/*
  此代码为第九艺术网络科技公司所有
  @copyright 2007-2010 保留所有权利
*/
/**
@file CDataProtect.h
@author xophiix
@version 1.0
@date 2011/01/24
数据保护

*/
#pragma once

#include "CGammaRand.h"

namespace Gamma
{
    template<class T>
    class CDataProtectMgr
    {
		typedef T ValueDataType;
		typedef map<unsigned, T*> CValuesType;
		typedef typename CValuesType::iterator iterator;
        map<unsigned,T*> m_mapValues;

        inline unsigned Void2Unsigned( const uint8_t* p )
        {
            unsigned short* pU16 = (unsigned short*)&p;
            unsigned short m = pU16[0]*0xe941 + pU16[1]*0x9843;
            unsigned short n = pU16[0]*0xab28 + pU16[1]*0xc4a5;
            return ( unsigned(m)<<16 )|n;
        }

    public:
        void RegistValue( uint8_t* p )
        {
            m_mapValues[ Void2Unsigned( p ) ] = NULL;
        }

        void RemoveValue( uint8_t* p )
        {
            iterator it = m_mapValues.find( Void2Unsigned( p ) );
			T* pData = it->second;
            delete []pData;
            m_mapValues.erase( it );
        }

        T Get( const uint8_t* p )
        {
			iterator it = m_mapValues.find( Void2Unsigned( p ) );
            if ( it == m_mapValues.end() || !it->second )
                return T();

            T a = it->second[ uint8_t( p[0] + 5 ) ];
            uint8_t* pBuf = (uint8_t*)&a;
            for( int i = 0; i < sizeof(T); i++ )
                pBuf[i] = pBuf[i]^0x78;

            return a;
        }

        const T& Set( uint8_t* p, const T& value )
        {
            T* pNew = new T[256];
            iterator it = m_mapValues.find( Void2Unsigned( p ) );
            if ( it == m_mapValues.end() )
                return value;

            delete []it->second;
            it->second = pNew;
            p[0] = (uint8_t)CGammaRand::Rand<int16_t>( 0, 256 );

            T a = value;
            uint8_t* pBuf = (uint8_t*)&a;
            for( int i = 0; i < sizeof(T); i++ )
                pBuf[i] = pBuf[i]^0x78;
            it->second[uint8_t( p[0] + 5 ) ] = a;

            return value;
        }

        static CDataProtectMgr<T>& Instance()
        {
            static CDataProtectMgr<T> _instance;
            return _instance;
        } 
    };

    template<class T>
    class CDataProtect
    {
        uint8_t uMagicValue;
    public:
        CDataProtect()
        {
            CDataProtectMgr<T>::Instance().RegistValue( &uMagicValue );
        }

        CDataProtect( const T& Value )
        {
            CDataProtectMgr<T>::Instance().RegistValue( &uMagicValue );
            CDataProtectMgr<T>::Instance().Set( &uMagicValue, Value );
        }

        ~CDataProtect()
        {
            CDataProtectMgr<T>::Instance().RemoveValue( &uMagicValue );
        }

        operator T() const
        {
            return CDataProtectMgr<T>::Instance().Get( &uMagicValue );
        }

        const T& operator= ( const T& Value )
        {
            return CDataProtectMgr<T>::Instance().Set( &uMagicValue, Value );
        }

        const T& operator= ( const CDataProtect<T>& Value )
        {
            return CDataProtectMgr<T>::Instance().Set( &uMagicValue, (T)Value );
        }
    };
}