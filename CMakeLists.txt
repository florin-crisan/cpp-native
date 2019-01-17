project(NativeStreamBuf)
cmake_minimum_required (VERSION 3.0)

add_library(${PROJECT_NAME} STATIC 
	include/fcrisan/native/error.hpp
	src/error.cpp
	include/fcrisan/native/file.hpp
	src/file.cpp
	include/fcrisan/native/code_page.hpp
	include/fcrisan/native/text.hpp
	src/text.cpp
	include/fcrisan/native/text_writer.hpp
	src/text_writer.cpp
	include/fcrisan/native/filebuf.hpp
	src/filebuf.cpp
	include/fcrisan/native/ostream_extractors.hpp
	src/ostream_extractors.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC include)

add_executable(Test${PROJECT_NAME} test/test.cpp)
target_link_libraries(Test${PROJECT_NAME} ${PROJECT_NAME})

add_test(NAME Test${PROJECT_NAME} COMMAND Test${PROJECT_NAME})


add_executable(Echo test/echo.cpp)
target_link_libraries(Echo ${PROJECT_NAME})
