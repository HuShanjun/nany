gamma_add_gtest(test_GammaShm
  SOURCES test_ShareMemoryMgr.cpp
  LIBS GammaCommon GammaShm
  RUN_SERIAL
)
