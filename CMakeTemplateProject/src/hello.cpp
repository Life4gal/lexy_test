#include <CMakeTemplateProject/hello.hpp>

#include <lexy/dsl.hpp>
#include <lexy/action/parse.hpp> // lexy::parse
#include <lexy/input/file.hpp>   // lexy::read_file
#include <lexy/input/string_input.hpp>
#include <lexy_ext/report_error.hpp> // lexy_ext::report_error
#include <lexy/callback.hpp>     // value callbacks

#include <iostream>
#include <exception>

namespace grammar
{
	namespace dsl = lexy::dsl;

	// -123456.789e-42
	struct number : public lexy::token_production
	{
		struct invalid_digit
		{
			consteval static auto name() { return "Invalid number digit."; }
		};

		struct trailing_space_required
		{
			consteval static auto name() { return "Trailing spaces or line breaks are required.(It may also be an invalid digit in the string.)"; }
		};

		struct digit_with_optional_sign : public lexyd::char_class_base<digit_with_optional_sign>
		{
			consteval static auto char_class_name() { return "ASCII.digit_with_optional_sign"; }

			constexpr static auto char_class_ascii()
			{
				lexy::_detail::ascii_set result;
				result.insert('-');
				result.insert(lexyd::ascii::_digit::char_class_ascii());
				return result;
			}
		};

		struct value_part : public lexy::transparent_production
		{
			constexpr static auto rule = dsl::identifier(digit_with_optional_sign{}, dsl::ascii::digit.error<invalid_digit>);

			constexpr static auto value = lexy::as_string<ast::number::value_type, lexy::utf8_encoding>;
		};

		struct fraction_part : lexy::transparent_production
		{
			constexpr static auto rule = dsl::lit_c<'.'> >> dsl::capture(dsl::digits<>.error<invalid_digit>);

			constexpr static auto value = lexy::as_string<ast::number::fraction_type, lexy::utf8_encoding>;
		};

		struct exponent_part : lexy::transparent_production
		{
			constexpr static auto rule = (dsl::lit_c<'e'> | dsl::lit_c<'E'>) >> dsl::sign + dsl::integer<ast::number::exponent_type>;

			constexpr static auto value = lexy::as_integer<ast::number::exponent_type>;
		};

		constexpr static auto rule = []
		{
			// constexpr auto trailing_space = dsl::try_((dsl::ascii::blank / dsl::ascii::newline).error<trailing_space_required>, dsl::ascii::blank / dsl::ascii::newline);
			constexpr auto trailing_space = (dsl::ascii::blank / dsl::ascii::newline).error<trailing_space_required>;
			// Starts with `-` or any digit
			return dsl::peek(digit_with_optional_sign{}) >>
					// Necessary integer part
					dsl::p<value_part> +
					// Optional fraction Part
					dsl::opt(dsl::p<fraction_part>) +
					// Optional exponent part
					dsl::opt(dsl::p<exponent_part>) +
					// End with blank or line break
					trailing_space;
		}();

		constexpr static auto value = lexy::construct<ast::number>;
	};
	static_assert(lexy::match<number>(lexy::zstring_input<lexy::utf8_encoding>(u8"-123456.789e-42 ")));
}

namespace lexy_test
{
	auto parse_and_print(const std::string_view filename) -> void
	{
		const auto file = lexy::read_file<lexy::utf8_encoding>(filename.data());
		if (!file)
		{
			// todo
			throw std::exception{"Cannot read file!"};
		}

		auto production = lexy::parse<grammar::number>(file.buffer(), lexy_ext::report_error);
		if (!production.has_value())
		{
			// todo
			throw std::exception{"Context error!"};
		}

		[[maybe_unused]] const auto& value = std::move(production).value();
	}
}// namespace ctp
