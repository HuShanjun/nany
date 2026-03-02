
#include "GammaCommon/CGammaObject.h"
#include "GammaCommon/GammaHelp.h"

namespace Gamma
{
	ObjectCreateCallBack	g_ObjectCreateCallBack	= NULL;
	ObjectDestroyCallBack	g_ObjectDestroyCallBack = NULL;

	void SetObjectCreateCallBack( ObjectCreateCallBack pCallback )
	{
		g_ObjectCreateCallBack = pCallback;
	}

	ObjectCreateCallBack GetObjectCreateCallBack()
	{
		return g_ObjectCreateCallBack;
	}

	void SetObjectDestroyCallBack( ObjectDestroyCallBack pCallback )
	{
		g_ObjectDestroyCallBack = pCallback;
	}

	ObjectDestroyCallBack GetObjectDestroyCallBack()
	{
		return g_ObjectDestroyCallBack;
	}

	CGammaObject::CGammaObject(void)
	{
		if( g_ObjectCreateCallBack )
			g_ObjectCreateCallBack( this );
	}

	CGammaObject::~CGammaObject(void)
	{
		if( g_ObjectDestroyCallBack )
			g_ObjectDestroyCallBack( this );
	}
}
