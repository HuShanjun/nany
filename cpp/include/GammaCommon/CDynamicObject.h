//=====================================================================
// CDynamicObject.h 
// 动态分配对象
// 柯达昭
// 2010-07-26
//=======================================================================

#ifndef _GAMMA_DYNAMICOBJ_H_
#define _GAMMA_DYNAMICOBJ_H_

#include "GammaCommon/GammaCommonType.h"
#include "GammaCommon/CRefshare.h"

namespace Gamma
{
	class CDynamicObject;

	typedef CDynamicObject* (*CreateDynamicObjFun)( void*, const void* );
	typedef void* (*DestroyDynamicObjFun)( CDynamicObject* );

	class GAMMA_COMMON_API CDynamicObject : public CGammaObject
	{ 
	public: 
		virtual ~CDynamicObject() {}
		bool						IsValid() const;
		virtual uint32_t				GetClassID() = 0;
		virtual bool				IsClassID( uint32_t nClassID ) = 0;
		static void					UnregisterClass( uint32_t nClassID );
		static CDynamicObject*		CreateInstance( uint32_t nClassID, const void* pContext = NULL );
		static void					DestroyInstance( CDynamicObject* pInstance );
		static size_t				GetClassInstanceUsedCount( uint32_t nClassID );
		static uint32_t				GetAllocSize();
		static void					DumpAllocSize();
		static void					RegisterClass( uint32_t nClassID, uint32_t nObjectSize, uint32_t nObjectPoolSize,
										CreateDynamicObjFun pCreateFun, DestroyDynamicObjFun pDestroyFun );
	};

	// 带构造参数的Dynamic类型定义
	template<class CDynamicClass, class ParamType>
	struct TDynamicClassDef
	{
		TDynamicClassDef( uint32_t nClassID, uint32_t nObjectPoolSize )
		{
			CDynamicObject::RegisterClass( 
				nClassID, sizeof(CDynamicClass), nObjectPoolSize, 
				&TDynamicClassDef<CDynamicClass, ParamType>::ConstructInstance, 
				&TDynamicClassDef<CDynamicClass, ParamType>::DestructInstance );
		}

		static CDynamicObject* ConstructInstance( void* pBuf, const void* pContext )
		{ 
			return new( pBuf ) CDynamicClass( *(const ParamType*)pContext );
		}

		static void* DestructInstance( CDynamicObject* p )
		{ 
			CDynamicClass* c = static_cast<CDynamicClass*>(p); 
			c->~CDynamicClass(); 
			return c;
		}
	};

	// 不带构造参数的Dynamic类型定义
	template<class CDynamicClass>
	struct TDynamicClassDef<CDynamicClass, void>
	{
		TDynamicClassDef( uint32_t nClassID, uint32_t nObjectPoolSize )
		{
			CDynamicObject::RegisterClass( 
				nClassID, sizeof(CDynamicClass), nObjectPoolSize, 
				&TDynamicClassDef<CDynamicClass, void>::ConstructInstance, 
				&TDynamicClassDef<CDynamicClass, void>::DestructInstance );
		}

		static CDynamicObject* ConstructInstance( void* pBuf, const void* pContext )
		{ 
			return new( pBuf ) CDynamicClass();
		}

		static void* DestructInstance( CDynamicObject* p )
		{ 
			CDynamicClass* c = static_cast<CDynamicClass*>(p); 
			c->~CDynamicClass(); 
			return c;
		}
	};

	///得到class的Hash值
	#define GET_CLASS_ID( Class ) \
	( GammaHash( #Class , strlen( #Class ) ) )

	#define GET_CLASS_ID_BY_CLASS_NAME( Class ) \
	( GammaHash( Class, strlen( Class ) ) )

	///声明Dynamic类型
	#define DECLARE_DYNAMIC_CLASS( Class ) \
		private: \
		typedef Gamma::TDynamicClassDef<Class, void> CDynamicClassDef;\
		static uint32_t s_nClassID;\
		static CDynamicClassDef s_ClassDef;\
		public: \
		static uint32_t GetID() { return s_nClassID; } \
		virtual uint32_t GetClassID(){ return Class::s_nClassID; }\
		virtual bool IsClassID( uint32_t nClassID ){ return nClassID == s_nClassID; };\
		static Class* CreateInstance() { return static_cast<Class*>( \
			CDynamicObject::CreateInstance( Class::s_nClassID, 0 ) ); }

	///声明Dynamic类型
	#define DECLARE_DYNAMIC_CLASS_WITH_PARENT( Class, Parent ) \
		private:\
		typedef Gamma::TDynamicClassDef<Class, void> CDynamicClassDef;\
		static uint32_t s_nClassID;\
		static CDynamicClassDef s_ClassDef;\
		public: \
		static uint32_t GetID() { return s_nClassID; } \
		virtual uint32_t GetClassID(){ return Class::s_nClassID; }\
		virtual bool IsClassID( uint32_t nClassID ){ return nClassID == s_nClassID\
			 || Parent::IsClassID(nClassID); };\
		static Class* CreateInstance() { return static_cast<Class*>( \
			CDynamicObject::CreateInstance( Class::s_nClassID, 0 ) ); }

	///定义CLIENTCONN对象
	#define DEFINE_DYNAMIC_CLASS( Class, nPoolSize, ClassID ) \
		uint32_t Class::s_nClassID = ClassID;\
		Class::CDynamicClassDef Class::s_ClassDef( ClassID, nPoolSize );

}

#endif