if (BUILD_TESTING)
	file(
	DOWNLOAD
	https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.39.0/CPM.cmake
	${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
	EXPECTED_HASH SHA256=66639bcac9dd2907b2918de466783554c1334446b9874e90d38e3778d404c2ef
	)
	include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

	CPMAddPackage("gh:catchorg/Catch2@3.6.0")
	list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)

	include(CTest)
	include(Catch)

	enable_testing()
	include("${CMAKE_PATH}/product-template.cmake")

	target_link_libraries(${PRODUCT_NAME} PUBLIC genesis Catch2::Catch2WithMain)

	add_test(NAME ${PRODUCT_NAME} COMMAND ${PRODUCT_NAME})
	enable_testing()
endif()