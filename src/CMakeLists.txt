FILE(GLOB SOURCE_1 "${PROJECT_ROOT_PATH}/src/*.cpp")
FILE(GLOB SOURCE_2 "${PROJECT_ROOT_PATH}/thirdparty/src/*.cpp")
ADD_LIBRARY(classifier STATIC ${SOURCE_1} ${SOURCE_2} )
TARGET_LINK_LIBRARIES(classifier Linear blas ICTCLAS50 glog pthread)
