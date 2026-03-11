#pragma once
#include "GammaCommon/GammaCommonType.h"

namespace Gamma
{
	class GAMMA_COMMON_API  CGammaObject
	{
	public:
		CGammaObject(void);
		~CGammaObject(void);
	};

	typedef void (*ObjectCreateCallBack)( CGammaObject* );
	typedef void (*ObjectDestroyCallBack)( CGammaObject* );

	GAMMA_COMMON_API void					SetObjectCreateCallBack( ObjectCreateCallBack );
	GAMMA_COMMON_API ObjectCreateCallBack	GetObjectCreateCallBack();

	GAMMA_COMMON_API void					SetObjectDestroyCallBack( ObjectDestroyCallBack );
	GAMMA_COMMON_API ObjectDestroyCallBack	GetObjectDestroyCallBack();
}
