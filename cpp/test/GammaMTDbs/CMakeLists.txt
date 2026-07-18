gamma_add_gtest(test_GammaMTDbs
  SOURCES test_DbsThreadMgr.cpp
  LIBS GammaCommon GammaDatabase GammaMTDbs
  RUN_SERIAL
)
