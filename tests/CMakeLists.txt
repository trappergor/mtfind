add_executable(test_args test_args.cpp)
target_link_libraries(test_args PRIVATE mtfind_lib)
add_test(NAME args COMMAND test_args)