#pragma once

#include <string_view>

namespace frontend
{
	auto parse_file_and_print(std::string_view filename) -> void;
}
