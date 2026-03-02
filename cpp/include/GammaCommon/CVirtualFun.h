/**@file  		CVirtualFun.h
* @brief		Define virtual function structure and interface
* @author		Daphnis Kau
* @date			2019-06-24
* @version		V1.0
*/
#ifndef __XS_VIRTUALFUN_H__
#define __XS_VIRTUALFUN_H__

#include "GammaCommon/GammaHelp.h"

namespace Gamma
{
	#define MAX_VTABLE_SIZE        512

	/**@struct Define virtual table
	 * @brief Assume C++ Object structure as blow		
	 * （Depend on compiler，such as vc++/gcc/clang）	
	 * pObj-> ----------------							
	 *       | SFunctionTable |  -> --------------		
	 *        ----------------     |  FunMember1  |		
	 *       |   DataMember1  |    |  FunMember2  |		
	 *       |   DataMember2  |    |  FunMember3  |		
	 *       |   DataMember3  |    |  FunMember4  |		
	 *       |   DataMember4  |    |  FunMember5  |		
	 *       |   DataMember5  |    | ...........  |		
	 *       |   ...........  |     --------------		
	 *        --------------     						
	 *                             						
	 *													
	 */
	struct GAMMA_COMMON_API SFunctionTable
	{
		SFunctionTable();
		int32_t GetFunctionCount();

		void* m_pFun[MAX_VTABLE_SIZE];
	};

	struct SVirtualObj 
	{ 
		SFunctionTable* m_pTable;
	};

	///< Find virtual function index in virtual table
	typedef void( *VirtualFunCallback )( void*, void* );
	uint32_t GAMMA_COMMON_API FindVirtualFunction( uint32_t nSize,
		VirtualFunCallback funCallback, void* pContext );

	///< Find normal virtual function's index in virtual table
	template< typename ClassType, typename RetType, typename... Param >
	static uint32_t GetVirtualFunIndex( RetType ( ClassType::*pFun )( Param... ) )
	{
		typedef void ( ClassType::*FunctionType )();
		FunctionType funCall = (FunctionType)pFun;
		struct SFun { static void Call( ClassType* pObj, void* pContext )
		{ FunctionType funCall = *(FunctionType*)pContext; ( pObj->*funCall )(); } };
		VirtualFunCallback funCallback = (VirtualFunCallback)&SFun::Call;
		return FindVirtualFunction( sizeof( ClassType ), funCallback, &funCall );
	}

	///< Find destructor's index in virtual table
	template<typename ClassType>
	uint32_t GetDestructorFunIndex()
	{
		struct SFun : public ClassType 
		{ virtual ~SFun(){} static void Call( SFun* pObj, void* ) { pObj->~SFun(); } };
		if( sizeof( SFun ) != sizeof( ClassType ) )
			throw "the class have not virtual table";
		VirtualFunCallback funCallback = (VirtualFunCallback)&SFun::Call;
		return FindVirtualFunction( sizeof( SFun ), funCallback, nullptr );
	}
}

#endif