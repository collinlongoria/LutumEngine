add_executable(LutumTests
        TestMain.cpp
        Core/TestJobs.cpp
        ECS/TestCuria.cpp
)

target_link_libraries(LutumTests PRIVATE
        LutumCore
        doctest::doctest
)

add_test(NAME LutumTests COMMAND LutumTests)