set (Boost_USE_STATIC_LIBS OFF)
find_package(Boost 1.74 COMPONENTS system unit_test_framework)
if ($Boost_FOUND)
    include_directories (${Boost_INCLUDE_DIRS})
<<<<<<< HEAD
    add_executable (asrlabtest test2.cpp test3.cpp test4.cpp)
=======
    add_executable (asrlabtest test2.cpp test3.cpp)
>>>>>>> exercise3-zrz
    target_link_libraries(asrlabtest
                        ${Boost_FILESYSTEM_LIBRARY}
                        ${Boost_SYSTEM_LIBRARY}
                        ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
                        asrlablib
                        )
endif()
