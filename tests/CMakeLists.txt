add_executable(test_args test_args.cpp)
target_link_libraries(test_args PRIVATE mtfind_lib)
add_test(NAME args COMMAND test_args)

add_executable(test_mask test_mask.cpp)
target_link_libraries(test_mask PRIVATE mtfind_lib)
add_test(NAME mask COMMAND test_mask)

add_executable(test_searcher test_searcher.cpp)
target_link_libraries(test_searcher PRIVATE mtfind_lib)
add_test(NAME searcher COMMAND test_searcher)