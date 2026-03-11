#if !defined(_CCEXCEPTION_H_)
#define _CCEXCEPTION_H_


#ifdef WIN32
#pragma warning(disable:4996)
#endif

#include <exception>
#include <string>
#include <stdarg.h>
#include <stdio.h>
#include "GammaCommon/GammaCommonType.h"

template <class T>
void ThrowException(const char* fmt,...)
{
	char buffer[200];
	va_list list;
	va_start(list,fmt);
#ifdef WIN32
	int retval = _vsnprintf(buffer,sizeof(buffer)-1,fmt,list);
	if ( retval > 0 )
	{
		buffer[retval] = 0;
	}
	else if ( retval == -1 )
	{
		buffer[sizeof(buffer)-1] = 0;
	}
#elif defined GCC
	//linux锟斤拷锟皆讹拷锟斤拷锟斤拷0锟斤拷锟斤拷锟斤拷锟?
	int retval = vsnprintf(buffer,sizeof(buffer),fmt, list);

/*
#ifdef _DEBUG
__asm__( 
"int3" 
);
#endif // _DEBUG
*/

#else
/*
#ifdef _DEBUG
__asm__( 
"int3" 
);
#endif // _DEBUG
*/
	int retval = vsnprintf(buffer,sizeof(buffer)-1,fmt,list);
	if ( retval > 0 )
	{
		buffer[retval] = 0;
	}
	else if ( retval == -1 )
	{
		buffer[sizeof(buffer)-1] = 0;
	}
#endif

	buffer[sizeof(buffer)-1] = 0 ;
	va_end(list);

	throw T(buffer);
}

#ifdef _WIN32
class GAMMA_COMMON_API std::exception;
#endif

class GAMMA_COMMON_API CCException : public std::exception
{
protected :
	std::string m_ErrMsg;

public :
	CCException(const std::string& strErrMsg);
	CCException(const char* strErrMsg);	
	virtual ~CCException() throw();
	const char* what() const throw();
};

#endif

