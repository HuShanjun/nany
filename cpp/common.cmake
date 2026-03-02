
################
# Common libraries to link
################

find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(CURL REQUIRED)
find_package(unofficial-libmariadb CONFIG REQUIRED)

# 3rd-libs
target_link_libraries(${ProjectName} CURL::libcurl OpenSSL::SSL OpenSSL::Crypto mysqlclient ZLIB::ZLIB unofficial::libmariadb)

if(WIN32)
    target_link_libraries(${ProjectName} ws2_32 lua)
else(WIN32)
    find_package(unofficial-libuuid CONFIG REQUIRED)
    target_link_libraries(${ProjectName} c stdc++ dl pthread rt m lua unofficial::UUID::uuid)
endif(WIN32)

################
# install
################
# install(FILES $<TARGET_FILE:${ProjectName}> DESTINATION ${SERVERBIN_PATH}/modules)
# if(WIN32)
# 	install(FILES $<TARGET_PDB_FILE:${ProjectName}> DESTINATION ${SERVERBIN_PATH}/modules OPTIONAL)
# endif()
