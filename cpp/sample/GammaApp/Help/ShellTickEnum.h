#pragma once

enum EShellTickId
{
	eSCT_StatNetID,
	eSST_DBCheck,
	eSST_WaitVerifyAnswer,
	eSST_Online,
	eSST_CheckState,
	eSST_CheckVoucher,
	eSST_ReconnectTick,
	eSST_GameConnFromClient,
	eSST_FlushUsercenterRequest,
	eSST_Disconnect,
	eSST_CharDataCheck,
	eSST_ShmDataCheck,
	eSCT_CppTickCount = 1000,
};
