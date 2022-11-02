#include <CMakeTemplateProject/frontend.hpp>

#define BOOST_UT_DISABLE_MODULE

#include <boost/ut.hpp>

using namespace boost::ut;

suite test_frontend = [] {
	constexpr std::string_view target_file{"test_frontend.txt"};

	frontend::parse_file_and_print(target_file);
};
