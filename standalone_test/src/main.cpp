#include <CMakeTemplateProject/hello.hpp>
#include <iostream>

auto main() -> int
{
	std::cout << "Hello CMakeTemplateProject!"
			<< "\nCompiler Name: " << CMakeTemplateProject_COMPILER_NAME
			<< "\nCompiler Version: " << CMakeTemplateProject_COMPILER_VERSION
			<< "\nCTP Version: " << CMakeTemplateProject_VERSION
			<< '\n';

	try { lexy_test::parse_file_and_print("test.txt"); }
	catch (const std::exception& e) { std::cout << "parse failed: " << e.what() << '\n'; }
}
