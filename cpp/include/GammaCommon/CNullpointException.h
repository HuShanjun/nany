#ifndef _NULLPOINTEXCEPTION_H_
#define _NULLPOINTEXCEPTION_H_

#include "GammaCommon/CUserException.h"
/*
*/

class GAMMA_COMMON_API CNullPointException : public CCException
{
public :
	CNullPointException(const std::string& strErrMsg);
	virtual ~CNullPointException() throw();
};

#endif
