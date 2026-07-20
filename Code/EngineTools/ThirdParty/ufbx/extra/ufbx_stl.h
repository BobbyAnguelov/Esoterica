#ifndef UFBX_STL_H
#define UFBX_STL_H

#if !defined(UFBX_VERSION)
	#error You must include "ufbx.h" before including "ufbx_stl.h"
#endif

#if UFBX_CPP11
	#include <string>
	template <> struct ufbx_converter<std::string> {
		static inline std::string from(const ufbx_string &str) { return { str.data, str.length }; }
		static inline ufbx_string_view to(const std::string &str) { return { str.data(), str.size() }; }
	};

	inline std::string ufbx_format_error_string(const ufbx_error &error)
	{
		char buf[1024];
		ufbx_format_error(buf, sizeof(buf), &error);
		return std::string(buf);
	}

	#if defined(__cpp_lib_string_view)
		#include <string_view>
		template <> struct ufbx_converter<std::string_view> {
			static inline std::string_view from(const ufbx_string &str) { return { str.data, str.length }; }
			static inline ufbx_string_view to(const std::string_view &str) { return { str.data(), str.size() }; }
		};
	#endif
#endif

#endif

