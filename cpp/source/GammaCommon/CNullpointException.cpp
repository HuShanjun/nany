#include "GammaCommon/CNullpointException.h"

/*

Class	CNullPointException

*/

CNullPointException::CNullPointException(const std::string& strErrMsg): CCException(strErrMsg)
{
}

CNullPointException::~CNullPointException() throw()
{
}
