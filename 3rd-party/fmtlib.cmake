CPMFindPackage(
		NAME fmt
#		GIT_TAG master
		GIT_TAG 9.1.0
		GITHUB_REPOSITORY "fmtlib/fmt"
		OPTIONS "FMT_PEDANTIC ON" "FMT_WERROR ON" "FMT_DOC OFF" "FMT_INSTALL ON" "FMT_TEST ON"
)

CPM_link_libraries_APPEND(fmt PUBLIC)