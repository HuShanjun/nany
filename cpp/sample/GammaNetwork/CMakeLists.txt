cmake_minimum_required(VERSION 3.19)

set(LINK_LIBS GammaCommon GammaNetwork)

add_executable(GammaNetworkServerSample server.cpp)
target_link_libraries(GammaNetworkServerSample ${LINK_LIBS})
set_target_properties(GammaNetworkServerSample PROPERTIES FOLDER "sample")

add_executable(GammaNetworkClientSample client.cpp)
target_link_libraries(GammaNetworkClientSample ${LINK_LIBS})
set_target_properties(GammaNetworkClientSample PROPERTIES FOLDER "sample")
