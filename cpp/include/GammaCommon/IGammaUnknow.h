//===============================================
// IGammaUnknow.h
// 定义IGammaUnknown接口
// 柯达昭
// 2007-08-30
//===============================================
#ifndef __IGAMMA_UNKNOW_H__
#define __IGAMMA_UNKNOW_H__

#include "GammaCommon/GammaCommonType.h"

class IGammaUnknown
{
public:
	virtual uint32	GetRef() = 0;
    virtual void	AddRef() = 0;
    virtual void    Release() = 0;
};

#define DEFAULT_OPERATOR( ClassName ) \
	private: \
	uint32 m_nRef; \
	public: \
	ClassName() : m_nRef(1){}\
	uint32	GetRef() { return m_nRef; } \
	void	AddRef() { ++m_nRef; } \
	void	Release() { if( --m_nRef == 0 ){ delete this; } }

#define DEFAULT_METHOD( ClassName ) \
	private: \
	uint32 m_nRef; \
	public: \
	uint32	GetRef() { return m_nRef; } \
	void	AddRef() { ++m_nRef; } \
	void	Release() { if( --m_nRef == 0 ){ delete this; } }

#endif
