#include <CMakeTemplateProject/frontend.hpp>

#include <lexy/dsl.hpp>
#include <lexy/action/parse.hpp>
#include <lexy/input/file.hpp>
#include <lexy/input/string_input.hpp>
#include <lexy_ext/report_error.hpp>
#include <lexy/callback.hpp>

#include <map>
#include <string>
#include <string_view>
#include <optional>
#include <cstdio>
#include <memory>

namespace
{
	using symbol_name_type = std::string;
	using symbol_name_view_type = std::string_view;

	template<typename T>
	class SymbolTable final
	{
	public:
		using key_type = symbol_name_type;
		using mapped_type = T;

		using key_view_type = symbol_name_view_type;
		using optional_mapped_type = std::optional<std::reference_wrapper<const mapped_type>>;

	private:
		std::map<key_type, mapped_type, std::less<>> table_;

	public:
		[[nodiscard]] auto get(const key_view_type name) const -> optional_mapped_type
		{
			if (auto it = table_.find(name);
				it != table_.end()) { return it->second; }
			return std::nullopt;
		}

		auto set(const key_view_type name, mapped_type&& data) -> bool
		{
			auto [it, inserted] = table_.emplace(name, std::forward<decltype(data)>(data));
			return inserted;
		}

		auto set(const key_view_type name, const mapped_type& data) -> bool
		{
			auto [it, inserted] = table_.emplace(name, data);
			return inserted;
		}

		auto clear() -> void { table_.clear(); }
	};
}

namespace backend
{
	class BuiltinFunction { };

	class BuiltinType { };

	class Global { };

	class Local { };

	// function body
	class Block { };

	class Function
	{
	public:
		struct signature
		{
			using size_type = std::uint8_t;

			size_type input;
			size_type output;
		};

		signature sig;
	};

	using data_type = symbol_name_type;

	class LocalBuilder
	{
	public:
		auto register_local(const symbol_name_type& identifier) -> Local*
		{
			// todo
			(void)identifier;
			return new Local{};
		}

		auto register_block(const Function::signature signature) -> Block*
		{
			// todo
			(void)signature;
			return new Block{};
		}
	};

	class Module
	{
	public:
		symbol_name_type module_name;

		explicit Module(symbol_name_type&& module_name)
			: module_name{std::move(module_name)} {}

		auto register_function(const symbol_name_type& identifier, const Function::signature sig) -> Function*
		{
			// todo
			(void)identifier;
			return new Function{sig};
		}

		auto register_global_mutable_data(const symbol_name_type& identifier, data_type&& data) -> Global*
		{
			// todo
			(void)identifier;
			(void)data;
			return new Global{};
		}

		auto register_global_immutable_data(const symbol_name_type& identifier, data_type&& data) -> Global*
		{
			// todo
			(void)identifier;
			(void)data;
			return new Global{};
		}
	};
}

namespace
{
	class ParseState
	{
	public:
		template<typename Return, typename... Functions>
		[[nodiscard]] constexpr static auto callback(Functions&&... functions)
		{
			return lexy::bind(
					lexy::callback<Return>(std::forward<Functions>(functions)...),
					// out parse state
					lexy::parse_state,
					// parsed values
					lexy::values);
		}

		using context_type = lexy::buffer<lexy::utf8_encoding>;

		std::string filename;
		context_type buffer;
		lexy::input_location_anchor<context_type> buffer_anchor;

		SymbolTable<const backend::BuiltinFunction*> builtin_functions;
		SymbolTable<const backend::BuiltinType*> builtin_types;

		backend::Module* mod;
		std::unique_ptr<backend::LocalBuilder> local_builder;

		SymbolTable<backend::Global*> globals;
		SymbolTable<backend::Local*> locals;
		SymbolTable<backend::Function*> functions;
		SymbolTable<backend::Block*> blocks;

		backend::Function* current_function;

		ParseState(std::string&& filename, context_type&& buffer)
			: filename{std::move(filename)},
			buffer{std::move(buffer)},
			buffer_anchor{this->buffer},
			mod{nullptr},
			// todo: local_builder
			local_builder{std::make_unique<backend::LocalBuilder>()} { }

		auto report_invalid_identifier(const char8_t* position, const symbol_name_type& identifier, const char* category) const -> void
		{
			const auto location = lexy::get_input_location(buffer, position, buffer_anchor);

			const auto out = lexy::cfile_output_iterator{stderr};
			const lexy_ext::diagnostic_writer<context_type> writer{buffer, {.flags = lexy::visualize_fancy}};

			(void)writer.write_message(out,
										lexy_ext::diagnostic_kind::error,
										[&](lexy::cfile_output_iterator, lexy::visualization_options)
										{
											(void)std::fprintf(stderr, "unknown %s name '%s'", category, identifier.c_str());
											return out;
										});

			if (!filename.empty()) { (void)writer.write_path(out, filename.c_str()); }

			(void)writer.write_empty_annotation(out);
			(void)writer.write_annotation(
					out,
					lexy_ext::annotation_kind::primary,
					location,
					identifier.size(),
					[&](lexy::cfile_output_iterator, lexy::visualization_options)
					{
						(void)std::fprintf(stderr, "used here");
						return out;
					});
		}

		auto report_conflicting_signature(const char8_t* position, const symbol_name_type& identifier, const char* category) const -> void
		{
			const auto location = lexy::get_input_location(buffer, position, buffer_anchor);

			const auto out = lexy::cfile_output_iterator{stderr};
			const lexy_ext::diagnostic_writer<context_type> writer{buffer, {.flags = lexy::visualize_fancy}};

			(void)writer.write_message(out,
										lexy_ext::diagnostic_kind::error,
										[&](lexy::cfile_output_iterator, lexy::visualization_options)
										{
											(void)std::fprintf(stderr, "conflicting signature in %s declaration named '%s'", category, identifier.c_str());
											return out;
										});

			if (!filename.empty()) { (void)writer.write_path(out, filename.c_str()); }

			(void)writer.write_empty_annotation(out);
			(void)writer.write_annotation(
					out,
					lexy_ext::annotation_kind::primary,
					location,
					identifier.size(),
					[&](lexy::cfile_output_iterator, lexy::visualization_options)
					{
						(void)std::fprintf(stderr, "second declaration here");
						return out;
					});
		}

		auto report_duplicate_declaration(const char8_t* position, const symbol_name_type& identifier, const char* category) const -> void
		{
			const auto location = lexy::get_input_location(buffer, position, buffer_anchor);

			const auto out = lexy::cfile_output_iterator{stderr};
			const lexy_ext::diagnostic_writer<context_type> writer{buffer, {.flags = lexy::visualize_fancy}};

			(void)writer.write_message(out,
										lexy_ext::diagnostic_kind::error,
										[&](lexy::cfile_output_iterator, lexy::visualization_options)
										{
											(void)std::fprintf(stderr, "duplicate %s declaration named '%s'", category, identifier.c_str());
											return out;
										});

			if (!filename.empty()) { (void)writer.write_path(out, filename.c_str()); }

			(void)writer.write_empty_annotation(out);
			(void)writer.write_annotation(
					out,
					lexy_ext::annotation_kind::primary,
					location,
					identifier.size(),
					[&](lexy::cfile_output_iterator, lexy::visualization_options)
					{
						(void)std::fprintf(stderr, "second declaration here");
						return out;
					});
		}
	};
}

namespace grammar
{
	namespace dsl = lexy::dsl;

	struct identifier
	{
		constexpr static auto unquoted =
				dsl::identifier(
						// begin with alpha/underscore/period
						dsl::ascii::alpha_underscore / dsl::period,
						// continue with alpha/digit/underscore/period
						dsl::ascii::alpha_digit_underscore / dsl::period
						);

		constexpr static auto rule = []
		{
			constexpr auto quoted =
					dsl::single_quoted(
							// all printable unicode characters in single quotation marks
							dsl::unicode::print
							);

			return unquoted | quoted;
		}();

		constexpr static auto value = lexy::as_string<symbol_name_type>;
	};

	// it shall be unquoted
	#define BACKEND_KEYWORD(key) LEXY_KEYWORD(key, identifier::unquoted)

	// special identifier
	// $variable
	struct builtin_identifier
	{
		constexpr static auto rule = dsl::dollar_sign >> dsl::p<identifier>;

		constexpr static auto value = lexy::forward<symbol_name_type>;
	};

	// special identifier
	// @variable
	struct global_identifier
	{
		constexpr static auto rule = dsl::at_sign >> dsl::p<identifier>;

		constexpr static auto value = lexy::forward<symbol_name_type>;
	};

	// special identifier
	// %variable
	struct local_identifier
	{
		constexpr static auto rule = dsl::percent_sign >> dsl::p<identifier>;

		constexpr static auto value = lexy::forward<symbol_name_type>;
	};

	struct builtin_function_reference
	{
		constexpr static auto rule = dsl::position(dsl::p<builtin_identifier>);

		constexpr static auto value = ParseState::callback<backend::BuiltinFunction>(
				[](const ParseState& state, const char8_t* position, const symbol_name_type& symbol) -> backend::BuiltinFunction
				{
					const auto result = state.builtin_functions.get(symbol);

					if (!result.has_value()) { state.report_invalid_identifier(position, symbol, "builtin function"); }
					return **result;
				});
	};

	struct builtin_type_reference
	{
		constexpr static auto rule = dsl::position(dsl::p<builtin_identifier>);

		constexpr static auto value = ParseState::callback<backend::BuiltinType>(
				[](const ParseState& state, const char8_t* position, const symbol_name_type& symbol) -> backend::BuiltinType
				{
					const auto result = state.builtin_types.get(symbol);

					if (!result.has_value()) { state.report_invalid_identifier(position, symbol, "builtin type"); }
					return **result;
				});
	};

	struct global_reference
	{
		constexpr static auto rule = dsl::position(dsl::p<global_identifier>);

		constexpr static auto value = ParseState::callback<backend::Global*>(
				[](const ParseState& state, const char8_t* position, const symbol_name_type& symbol) -> backend::Global*
				{
					const auto result = state.globals.get(symbol);

					if (!result.has_value()) { state.report_invalid_identifier(position, symbol, "global"); }
					return *result;
				});
	};

	struct local_reference
	{
		constexpr static auto rule = dsl::position(dsl::p<local_identifier>);

		constexpr static auto value = ParseState::callback<backend::Local*>(
				[](const ParseState& state, const char8_t* position, const symbol_name_type& symbol) -> backend::Local*
				{
					const auto result = state.locals.get(symbol);

					if (!result.has_value()) { state.report_invalid_identifier(position, symbol, "local"); }
					return *result;
				});
	};

	struct function_signature
	{
		constexpr static auto rule = []
		{
			// [input => output]
			constexpr auto spec =
					dsl::integer<backend::Function::signature::size_type>
					>> LEXY_LIT("=>") +
					dsl::integer<backend::Function::signature::size_type>;
			return dsl::square_bracketed.opt(spec);
		}();

		constexpr static auto value = lexy::callback<backend::Function::signature>(
				lexy::construct<backend::Function::signature>,
				[](lexy::nullopt) { return backend::Function::signature{.input = 0, .output = 0}; });
	};

	struct function_reference
	{
		constexpr static auto rule =
				dsl::position +
				dsl::p<global_identifier> +
				// todo: dsl::if_(dsl::p<function_signature>);
				dsl::p<function_signature>;

		constexpr static auto value = ParseState::callback<backend::Function*>(
				// without signature
				[](const ParseState& state, const char8_t* position, const symbol_name_type& symbol) -> backend::Function*
				{
					const auto result = state.functions.get(symbol);

					if (!result.has_value()) { state.report_invalid_identifier(position, symbol, "function"); }
					return *result;
				},
				// with signature
				[](ParseState& state, const char8_t* position, const symbol_name_type& symbol, const backend::Function::signature& signature) -> backend::Function*
				{
					if (const auto result = state.functions.get(symbol);
						result.has_value())
					{
						if (const auto [i, o] = result->get()->sig;
							i != signature.input || o != signature.output) { state.report_conflicting_signature(position, symbol, "function"); }
						return *result;
					}

					auto* result = state.mod->register_function(symbol, signature);
					state.functions.set(symbol, result);
					return result;
				});
	};

	struct data_expression
	{
		struct byte
		{
			constexpr static auto rule = dsl::integer<std::uint8_t>(dsl::digits<dsl::hex>);

			constexpr static auto value = lexy::callback<backend::data_type>(
					[](const std::uint8_t d) { return backend::data_type{1, static_cast<backend::data_type::value_type>(d)}; });
		};

		struct string
		{
			constexpr static auto rule = dsl::quoted(dsl::ascii::print);

			constexpr static auto value = lexy::as_string<backend::data_type>;
		};

		struct repetition
		{
			constexpr static auto rule =
					dsl::square_bracketed(dsl::recurse<data_expression>) >> dsl::lit_c<'*'> + dsl::integer<backend::data_type::size_type>;

			constexpr static auto value = lexy::callback<backend::data_type>(
					[](const backend::data_type& data, const backend::data_type::size_type times)
					{
						backend::data_type result{};
						for (backend::data_type::size_type i = 0; i < times; ++i) { result += data; }
						return data;
					});
		};

		constexpr static auto rule = dsl::list(dsl::p<byte> | dsl::p<string> | dsl::p<repetition>, dsl::sep(dsl::comma));

		constexpr static auto value = lexy::concat<backend::data_type>;
	};

	struct global_declaration
	{
		struct mutable_global
		{
			constexpr static auto rule = []
			{
				constexpr auto declaration = dsl::p<global_identifier> + dsl::equal_sign + dsl::p<data_expression>;
				return BACKEND_KEYWORD("const") >> dsl::position + declaration;
			}();

			constexpr static auto value = ParseState::callback<void>(
					[](ParseState& state, const char8_t* position, const symbol_name_type& symbol, backend::data_type&& data) -> void
					{
						if (auto* result = state.mod->register_global_mutable_data(symbol, std::forward<decltype(data)>(data));
							!state.globals.set(symbol, result)) { state.report_duplicate_declaration(position, symbol, "global"); }
					});
		};

		struct immutable_global
		{
			static constexpr auto rule = []
			{
				constexpr auto declaration = dsl::p<global_identifier> + dsl::equal_sign + dsl::p<data_expression>;
				return dsl::else_ >> dsl::position + declaration;
			}();

			static constexpr auto value = ParseState::callback<void>(
					[](ParseState& state, const char8_t* position, const symbol_name_type& symbol, backend::data_type&& data) -> void
					{
						if (auto* result = state.mod->register_global_immutable_data(symbol, std::forward<decltype(data)>(data));
							!state.globals.set(symbol, result)) { state.report_duplicate_declaration(position, symbol, "global"); }
					});
		};

		constexpr static auto rule =
				BACKEND_KEYWORD("global") >>
				(dsl::p<mutable_global> | dsl::p<immutable_global>) +
				// semicolon required ?
				dsl::semicolon;

		constexpr static auto value = lexy::forward<void>;
	};

	struct local_declaration
	{
		constexpr static auto rule =
				BACKEND_KEYWORD("local") >>
				dsl::position +
				dsl::p<local_identifier> +
				// semicolon required ?
				dsl::semicolon;

		constexpr static auto value = ParseState::callback<void>(
				[](ParseState& state, const char8_t* position, const symbol_name_type& symbol) -> void
				{
					if (auto* result = state.local_builder->register_local(symbol);
						!state.locals.set(symbol, result)) { state.report_duplicate_declaration(position, symbol, "local"); }
				});
	};

	// todo
	struct instruction
	{
		struct dummy
		{
			constexpr static auto rule = BACKEND_KEYWORD("dummy");

			constexpr static auto value = ParseState::callback<void>(
					[](const ParseState& state) { (void)state; }
					);
		};

		constexpr static auto rule = dsl::p<dummy>;

		constexpr static auto value = lexy::forward<void>;
	};

	struct block_declaration
	{
		struct header
		{
			constexpr static auto rule =
					BACKEND_KEYWORD("block") +
					dsl::p<local_identifier> +
					dsl::p<function_signature>;

			constexpr static auto value = ParseState::callback<void>(
					[](ParseState& state, const symbol_name_type& symbol, const backend::Function::signature& signature) -> void
					{
						backend::Block* this_block;
						if (const auto result = state.blocks.get(symbol);
							result.has_value()) { this_block = result->get(); }
						else
						{
							this_block = state.local_builder->register_block(signature);
							if (!state.blocks.set(symbol, this_block))
							{
								// impossible ?
							}
						}

						// build block here
						(void)this_block;
					});
		};

		constexpr static auto rule = dsl::p<header> + dsl::curly_bracketed.list(dsl::p<instruction>);

		constexpr static auto value = lexy::forward<void>;
	};

	struct function_declaration
	{
		struct header
		{
			constexpr static auto rule = dsl::position + dsl::p<global_identifier> + dsl::p<function_signature>;

			constexpr static auto value = ParseState::callback<void>(
					[](ParseState& state, const char8_t* position, const symbol_name_type& symbol, const backend::Function::signature& signature) -> void
					{
						if (const auto result = state.functions.get(symbol);
							result.has_value())
						{
							if (const auto [i, o] = result->get()->sig;
								i != signature.input || o != signature.output) { state.report_conflicting_signature(position, symbol, "function"); }
							state.current_function = result->get();
						}
						else
						{
							auto* new_result = state.mod->register_function(symbol, signature);
							if (!state.functions.set(symbol, new_result))
							{
								// impossible ?
							}
							state.current_function = new_result;
						}

						// clear blocks && locals
						state.locals.clear();
						state.blocks.clear();
					});
		};

		struct body
		{
			static auto create_block_entry(ParseState& state) -> void
			{
				auto* block = state.local_builder->register_block(state.current_function->sig);

				// build block here
				(void)block;

				state.blocks.set("@block_entry@", block);
			}

			constexpr static auto rule = []
			{
				constexpr auto block_list =
						dsl::peek(BACKEND_KEYWORD("block")) >>
						dsl::curly_bracketed.as_terminator().list(dsl::p<block_declaration>);

				constexpr auto instruction_list =
						dsl::effect<create_block_entry>
						+ dsl::curly_bracketed.as_terminator().list(dsl::p<instruction>);

				constexpr auto locals =
						dsl::if_(dsl::list(dsl::p<local_declaration>));

				return dsl::curly_bracketed.open() >>
						locals +
						(block_list | dsl::else_ >> instruction_list);
			}();

			constexpr static auto value =
					lexy::noop >>
					ParseState::callback<void>(
							[](const ParseState& state) -> void
							{
								// finish local builder
								(void)state.local_builder;
							});
		};

		constexpr static auto rule =
				BACKEND_KEYWORD("function") >>
				dsl::p<header> +
				(dsl::semicolon | dsl::p<body>);

		constexpr static auto value = lexy::forward<void>;
	};

	struct module_declaration
	{
		constexpr static auto whitespace =
				// space
				dsl::ascii::space |
				// comment
				dsl::hash_sign >> dsl::until(dsl::newline);

		struct header
		{
			constexpr static auto rule =
					BACKEND_KEYWORD("module") +
					dsl::p<global_identifier> +
					// semicolon required ?
					dsl::semicolon;

			constexpr static auto value = ParseState::callback<void>(
					[](ParseState& state, symbol_name_type&& symbol) -> void
					{
						// create a module
						state.mod = new backend::Module{std::forward<decltype(symbol)>(symbol)};
					});
		};

		constexpr static auto rule =
				dsl::p<header> +
				dsl::terminator(dsl::eof).opt_list(dsl::p<global_declaration> | dsl::p<function_declaration>);

		constexpr static auto value = lexy::forward<void>;
	};
}

namespace frontend
{
	auto parse_file_and_print(const std::string_view filename) -> void
	{
		auto file = lexy::read_file<lexy::utf8_encoding>(filename.data());

		if (!file)
		{
			// todo
			throw std::exception{"Cannot read file!"};
		}

		ParseState state{std::string{filename}, std::move(file).buffer()};
		auto result = lexy::parse<grammar::module_declaration>(state.buffer, state, lexy_ext::report_error.opts({.flags = lexy::visualize_fancy}).path(state.filename.c_str()));

		if (!result.has_value())
		{
			if (state.mod)
			{
				// destroy the module
				delete state.mod;
				state.mod = nullptr;
			}
		}

		// use result?

		// return module?
		delete state.mod;
	}
}
