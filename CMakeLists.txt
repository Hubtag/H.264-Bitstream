cmake_minimum_required (VERSION 2.6)
project (h264_bitstream CXX C)
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -Wall -fstrict-aliasing -Wextra -Wno-unused-parameter -Wstrict-aliasing=2"  )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -g -Wall -fstrict-aliasing -Wextra -Wno-unused-parameter -Wstrict-aliasing=2"  )

include_directories( "${PROJECT_SOURCE_DIR}/src/" )

file(GLOB_RECURSE src_files "${PROJECT_SOURCE_DIR}/src/*.*")
file(GLOB_RECURSE tst_files "${PROJECT_SOURCE_DIR}/test/*.*")

add_library( h264_bitstream
	${src_files}
)

target_link_libraries( h264_bitstream )


add_executable( h264_bitstream_test
	${src_files}
	${tst_files}
)

target_link_libraries( h264_bitstream_test h264_bitstream gtest pthread )

