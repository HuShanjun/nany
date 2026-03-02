#include "GammaCommon/CUserException.h"

/*

		Class	CCException

*/


CCException::CCException(const std::string& strErrMsg): m_ErrMsg(strErrMsg)
{

}

CCException::CCException(const char* strErrMsg): m_ErrMsg(strErrMsg)
{

}

CCException::~CCException() throw()
{
}

const char* CCException::what() const throw()
{
	return m_ErrMsg.c_str();
}
