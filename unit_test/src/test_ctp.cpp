#include <CMakeTemplateProject/macro.hpp>
#include <CMakeTemplateProject/hello.hpp>

#define BOOST_UT_DISABLE_MODULE

#include <boost/ut.hpp>

// CTP_DISABLE_WARNING_PUSH
// CTP_DISABLE_WARNING(-Wswitch-enum)

// :(
#include <fmt/format.h>

// CTP_DISABLE_WARNING_POP

using namespace boost::ut;

// suite answer_test = []{
// 	expect(type<decltype(ctp::answer())> == type<int>) << fmt::format("Expected type of answer is `{}`, but actually `{}`.", reflection::type_name<int>(), reflection::type_name<decltype(ctp::answer())>());
// 	expect(ctp::answer() == 42_i) << "No! The answer should be 42!";
// };

suite test_parse_function_arguments = []
{
	expect(nothrow([]
	{
		constexpr std::u8string_view context{
				u8R"((int a,b:
			double
			,c: string,array
			d))"};

		lexy_test::parse_string_and_print(context);
	}));
};
