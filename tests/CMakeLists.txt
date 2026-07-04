add_executable(LutumTests
        TestMain.cpp
        Core/TestJobs.cpp
)

target_link_libraries(LutumTests PRIVATE
        LutumEngine
        doctest::doctest
)

add_test(NAME LutumTests COMMAND LutumTests)