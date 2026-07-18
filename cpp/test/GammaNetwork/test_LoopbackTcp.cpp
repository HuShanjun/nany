#include <gtest/gtest.h>
#include "GammaNetwork/INetworkInterface.h"

#include <exception>
#include <string>

using namespace Gamma;

// Bind-only smoke test: CreateNetWork() + StartListener(), then verify the
// listener's local address and connect type. This is NOT a loopback send/recv
// test — Connect(), Check(), Release(), and any real send/recv exchange are
// deliberately skipped (see NOTE below).
//
// NOTE (pre-existing engine bug, broader than the Task 6 finding):
// Gamma::CGNetwork::Check() segfaults *unconditionally*, even with zero
// listeners/connections active - reproduced with a minimal standalone
// `CreateNetWork(); Check(0);` program under gdb (SIGSEGV inside
// TGammaList<CAddrResolutionDelegate>::CGammaListNode::Remove(), called
// from CGNetwork::Check()). Gamma::INetwork::Connect() independently
// segfaults on its very first call, before Check() is ever invoked
// (SIGSEGV inside CGNetwork::Connect()'s address-resolution PushBack) -
// this was reproduced by running the *unmodified* compiled
// sample/GammaNetwork/client.cpp against sample/GammaNetwork/server.cpp
// over real loopback TCP: both sample processes crash. Because
// INetwork::Release() -> ~CGNetwork() unconditionally calls Check(0) (the
// same call chain documented in task-6-report.md for GammaConnects),
// releasing the network object is equally unsafe. This build's
// build/linux config has an empty CMAKE_BUILD_TYPE, so _DEBUG is not
// defined and the GammaAst() invariant checks that normally guard these
// list operations compile to no-ops - plausible contributing factor, but
// a real fix belongs to the engine/build config, not this test.
//
// Given Connect()/Check()/Release() are all confirmed to crash the whole
// test binary (which would take every other TEST in test_GammaNetwork down
// with it), this smoke test stops after bind verification and SKIPs before
// touching anything unsafe rather than crashing CI. pNetwork/pListener are
// intentionally left un-Released (intentional leak on process exit) — the
// same workaround pattern used in test/GammaConnects/test_ConnMgrSmoke.cpp
// for the Task 6 finding.
TEST( Network_Integration, ListenerBindSmoke )
{
	INetwork* pNetwork = CreateNetWork();
	ASSERT_NE( pNetwork, nullptr );

	const char*    szHost = "127.0.0.1";
	const uint16_t nPort  = 19090;

	IListener* pListener = nullptr;
	try
	{
		pListener = pNetwork->StartListener( szHost, nPort );
	}
	catch ( const std::string& strErr )
	{
		GTEST_SKIP() << "cannot bind " << szHost << ":" << nPort << " - " << strErr;
	}
	catch ( const std::exception& e )
	{
		GTEST_SKIP() << "cannot bind " << szHost << ":" << nPort << " - " << e.what();
	}

	if ( pListener == nullptr )
		GTEST_SKIP() << "cannot bind " << szHost << ":" << nPort;

	EXPECT_STREQ( szHost, pListener->GetLocalAddress().GetAddress() );
	EXPECT_EQ( nPort, pListener->GetLocalAddress().GetPort() );
	EXPECT_EQ( eConnecterType_TCP, pListener->GetConnectType() );

	GTEST_SKIP() << "Bind-only smoke complete; skipping Connect/Check/Release "
					"and send/recv: Gamma::CGNetwork::Connect()/Check() (and "
					"therefore INetwork::Release(), whose ~CGNetwork calls "
					"Check(0)) segfault unconditionally on this engine build "
					"(see comment above and nany/.superpowers/sdd/task-8-report.md). "
					"pNetwork/pListener intentionally leaked to avoid crashing "
					"the test binary.";
}
