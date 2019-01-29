include_directories(.)



add_definitions(-DSNIPER_LOG_TRACE -DSNIPER_LOG_DEBUG -DRAPIDJSON_HAS_STDSTRING -D_GLIBCXX_USE_CXX11_ABI=0 )
set (CMAKE_CXX_STANDARD 17)

set(PROJECT_HEADERS
    meta_erc_convert.h
    )


set(PROJECT_SOURCES
    meta_erc_convert.cpp
  )


set(PROJECT_MAIN
    main.cpp
    )

#Threads
find_package(Threads)


#fmt
find_package(fmt REQUIRED)


#Sniper
find_package(Sniper REQUIRED)
set(SNIPER_LIBS
    sniper_event
    sniper_url
    sniper_pico
    sniper_hashes
    sniper_base64
	sniper_mhd
    )


set(PROJECT_LIBS
    ${SNIPER_LIBS}
    ${CMAKE_THREAD_LIBS_INIT}
    ${LIBBSONCXX_STATIC_LIBRARIES}
    ${BSON_STATIC_LIBRARIES}
    #${SNAPPY_LIBRARY}
    
    fmt
    stdc++fs.a
	event.a
    rt.a
    ev.a
    config++.a
    crypto
    base64.a
    tcmalloc
    re2
    boost_system.a
    boost_filesystem.a
    boost_regex.a
    z
    lz4
    bz2
    uriparser
    microhttpd
    gmpxx.a
    gmp.a
    rocksdb.a
    libsnappy.a
    )



#APP Lib
add_library(${PROJECT_NAME}_lib STATIC ${PROJECT_HEADERS} ${PROJECT_SOURCES})
target_compile_options(${PROJECT_NAME}_lib PRIVATE -Wno-unused-parameter)


#APP
add_executable(${PROJECT_NAME} ${PROJECT_MAIN})
target_compile_options(${PROJECT_NAME} PRIVATE -Wno-unused-parameter)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_lib)
target_link_libraries(${PROJECT_NAME} ${PROJECT_LIBS})
#target_link_libraries(${PROJECT_NAME} ${ROCKSDB_LIBRARY})


