CPMFindPackage(
		NAME lexy
		VERSION 2022.05.1
		GITHUB_REPOSITORY "foonathan/lexy"
		OPTIONS "LEXY_BUILD_PACKAGE OFF"
)

CPM_link_libraries_APPEND(lexy PRIVATE)
