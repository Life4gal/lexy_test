#include <CMakeTemplateProject/hello.hpp>

#include <lexy/dsl.hpp>
#include <lexy/action/parse.hpp> // lexy::parse
#include <lexy/input/file.hpp>   // lexy::read_file
#include <lexy/input/string_input.hpp>
#include <lexy_ext/report_error.hpp> // lexy_ext::report_error
#include <lexy/callback.hpp>     // value callbacks

#include <iostream>
#include <exception>
#include <bit>

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
			constexpr auto trailing_space = dsl::peek(dsl::ascii::blank / dsl::ascii::newline).error<trailing_space_required>;
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

	struct orderless_birthday_info : public lexy::token_production
	{
		struct missing_field
		{
			static constexpr auto name = "missing info field";
		};

		struct duplicate_field
		{
			static constexpr auto name = "duplicate info field";
		};

		struct name : public lexy::transparent_production
		{
			struct invalid_character
			{
				static constexpr auto name = "invalid string character";
			};

			constexpr static auto rule = []
			{
				// constexpr auto lead_char = dsl::ascii::alpha;
				// constexpr auto trailing_char = dsl::ascii::word;
				// return dsl::identifier(lead_char, trailing_char);

				// Match zero or more non-control code points ("characters") surrounded by quotation marks.
				// We allow `\u` and `\U` as escape sequences.
				constexpr auto cp = (-dsl::ascii::control).error<invalid_character>;
				constexpr auto escape =
						dsl::backslash_escape
						.rule(dsl::lit_c<'u'> >> dsl::code_point_id<4>)
						.rule(dsl::lit_c<'U'> >> dsl::code_point_id<8>);

				return dsl::quoted(cp, escape);
			}();

			constexpr static auto value = lexy::as_string<ast::orderless_birthday_info::name_type, lexy::utf8_encoding>;
		};

		// todo: Almost the same structure makes me feel awful
		// template<typename T>
		// struct any_integer : public lexy::transparent_production
		// {
		// 	constexpr static auto rule = dsl::integer<T>(dsl::digits<>.no_leading_zero());
		//
		// 	constexpr static auto value = lexy::as_integer<T>;
		// };

		// using year = any_integer<ast::orderless_birthday_info::year_type>;
		struct year : public lexy::transparent_production
		{
			constexpr static auto rule = dsl::integer<ast::orderless_birthday_info::year_type>(dsl::digits<>.no_leading_zero());

			constexpr static auto value = lexy::as_integer<ast::orderless_birthday_info::year_type>;
		};

		// using month = any_integer<ast::orderless_birthday_info::month_type>;
		struct month : public lexy::transparent_production
		{
			constexpr static auto rule = dsl::integer<ast::orderless_birthday_info::month_type>(dsl::digits<>.no_leading_zero());

			constexpr static auto value = lexy::as_integer<ast::orderless_birthday_info::month_type>;
		};

		// using day = any_integer<ast::orderless_birthday_info::day_type>;
		struct day : public lexy::transparent_production
		{
			constexpr static auto rule = dsl::integer<ast::orderless_birthday_info::day_type>(dsl::digits<>.no_leading_zero());

			constexpr static auto value = lexy::as_integer<ast::orderless_birthday_info::day_type>;
		};

		struct time : public lexy::transparent_production
		{
			constexpr static auto rule = []
			{
				constexpr auto num = dsl::try_(dsl::integer<ast::orderless_birthday_info::time_type::value_type>, dsl::nullopt);
				constexpr auto colon = dsl::try_(dsl::colon);
				constexpr auto times = std::tuple_size_v<ast::orderless_birthday_info::time_type>;

				return dsl::times<times>(
						num,
						dsl::sep(colon));
			}();

			// Construct a ast::orderless_birthday_info::time_type as the result of the production.
			static_assert(std::tuple_size_v<ast::orderless_birthday_info::time_type> == 3);
			constexpr static auto value =
					lexy::bind(lexy::callback<ast::orderless_birthday_info::time_type>(
										// []() noexcept -> ast::orderless_birthday_info::time_type { return ast::orderless_birthday_info::time_type{42, 42, 42}; },
										[](
										const ast::orderless_birthday_info::time_type::value_type i1,
										const ast::orderless_birthday_info::time_type::value_type i2,
										const ast::orderless_birthday_info::time_type::value_type i3) noexcept -> ast::orderless_birthday_info::time_type
										{
											return ast::orderless_birthday_info::time_type{i1, i2, i3};
										}),
								lexy::_1 || ast::orderless_birthday_info::time_type::value_type{},
								lexy::_2 || ast::orderless_birthday_info::time_type::value_type{},
								lexy::_3 || ast::orderless_birthday_info::time_type::value_type{}
							);
		};

		// Allow spaces, tabs and newlines between the elements
		constexpr static auto whitespace = dsl::ascii::blank | dsl::ascii::newline;

		constexpr static auto rule = []
		{
			struct expected_trailing_comma
			{
				consteval static auto name() { return "expected trailing comma"; }
			};

			constexpr auto make_field = [](auto name, auto rule) constexpr noexcept
			{
				constexpr auto sep = dsl::try_(
						dsl::comma | dsl::ascii::blank | dsl::eol,
						dsl::until(dsl::newline).error<expected_trailing_comma>
						);
				return (name >> (LEXY_LIT("=") | LEXY_LIT(":"))) + rule + sep;// + end;
			};

			constexpr auto name_field = make_field(LEXY_LIT("name"), LEXY_MEM(name) = dsl::p<name>);
			constexpr auto year_field = make_field(LEXY_LIT("year"), LEXY_MEM(year) = dsl::p<year>);
			constexpr auto month_field = make_field(LEXY_LIT("month"), LEXY_MEM(month) = dsl::p<month>);
			constexpr auto day_field = make_field(LEXY_LIT("day"), LEXY_MEM(day) = dsl::p<day>);
			constexpr auto time_field = make_field(LEXY_LIT("time"), LEXY_MEM(time) = dsl::p<time>);

			return dsl::combination(
							name_field,
							year_field,
							month_field,
							day_field,
							time_field
							)
					.missing_error<missing_field>
					.duplicate_error<duplicate_field> +
					// Ensure that there are no trailing fields at the end
					dsl::eof;
		}();

		constexpr static auto value = lexy::as_aggregate<ast::orderless_birthday_info>;
	};

	static_assert(lexy::match<orderless_birthday_info>(lexy::zstring_input<lexy::utf8_encoding>(
			u8R"(month = 10 ,		time=23:59:59,
					 year = 2022, name = "somebody", day = 24)"
			)));
}

namespace lexy_test
{
	auto parse_and_print(const std::string_view filename) -> void
	{
		// constexpr auto is_little = std::endian::native == std::endian::little;

		const auto file = lexy::read_file<
			lexy::utf8_encoding//, is_little ? lexy::encoding_endianness::little : lexy::encoding_endianness::big
		>(filename.data());

		if (!file)
		{
			// todo
			throw std::exception{"Cannot read file!"};
		}

		auto production = lexy::parse<grammar::orderless_birthday_info>(file.buffer(), lexy_ext::report_error);
		if (!production.has_value())
		{
			// todo
			throw std::exception{"Context error!"};
		}

		[[maybe_unused]] const auto& value = std::move(production).value();
	}
}// namespace ctp
