#pragma once

#include <string_view>
#include <string>
#include <optional>
#include <vector>
#include <array>

namespace ast
{
	struct number
	{
		using value_type = std::string;
		using fraction_type = std::string;
		using exponent_type = std::int16_t;

		// -123456.789e-42
		// --> value = "-123456"
		// --> fraction = "789"
		// --> exponent = -42

		value_type value;
		std::optional<fraction_type> fraction;
		std::optional<exponent_type> exponent;
	};

	struct orderless_birthday_info
	{
		using name_type = std::string;
		using year_type = std::uint16_t;
		using month_type = std::uint8_t;
		using day_type = std::uint8_t;
		using time_type = std::array<std::uint8_t, 3>;

		// Allow spaces between the elements
		// [month = 10, year= 2022, name="somebody", time=23:59:59, day =24]
		// or
		// [day=24,
		// time=23:59:59,
		// month=10, year=2022,
		// name="somebody"]
		// --> name = "somebody"
		// --> year = 2022
		// --> month = 10
		// --> day = 24
		// --> time = [23, 59, 59]
		
		name_type  name;
		year_type year;
		month_type month;
		day_type   day;
		time_type  time;
	};
}

namespace lexy_test
{
	auto parse_and_print(std::string_view filename) -> void;
}
