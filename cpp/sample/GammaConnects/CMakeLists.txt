cmake_minimum_required(VERSION 3.19)

set(LINK_LIBS GammaCommon GammaNetwork GammaConnects)

add_executable(GammaConnectsServerSample server.cpp)
target_link_libraries(GammaConnectsServerSample ${LINK_LIBS})
set_target_properties(GammaConnectsServerSample PROPERTIES FOLDER "sample")

add_executable(GammaConnectsClientSample client.cpp)
target_link_libraries(GammaConnectsClientSample ${LINK_LIBS})
set_target_properties(GammaConnectsClientSample PROPERTIES FOLDER "sample")
