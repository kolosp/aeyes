function(aeyes_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE aeyes_core Qt6::Test)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

aeyes_test(tst_providers)
aeyes_test(tst_store)

find_program(NODE node)
if(NODE)
    add_test(NAME render_js COMMAND ${NODE} ${CMAKE_CURRENT_SOURCE_DIR}/render.test.js)
endif()
