#pragma once

#include <string_view>
#include <string>
#include <optional>

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
}

namespace lexy_test
{
	auto parse_and_print(std::string_view filename) -> void;
}
