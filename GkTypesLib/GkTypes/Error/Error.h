#pragma once

#ifndef _STRINGIZE_LINE_DETAIL
#define _STRINGIZE_LINE_DETAIL(x) #x
#endif
#ifndef _STRINGIZE_LINE
#define _STRINGIZE_LINE(x) _STRINGIZE_DETAIL(x) // C/C++ macros are stupid but this works.
#endif
#ifndef ERROR_FILE_ORIGIN
#define ERROR_FILE_ORIGIN ("File: " __FILE__ "\nLine: " _STRINGIZE(__LINE__))
#endif

namespace gk 
{
	class Error
	{
	public:

		virtual struct String toString() const;

	protected:

		/* See ERROR_FILE_ORIGIN macro */
		Error(const char* errorFileOrigin = nullptr) : _errorFileOrigin(errorFileOrigin) {}

		// Name of the error for printing. Should be independent of error instances.
		virtual const char* errorName() const = 0;

		// Simple description of the error. Should be independent of error instances.
		virtual const char* description() const = 0;

		// Information about the actual cause of the error.
		virtual struct String cause() const = 0;

	private:

		const char* _errorFileOrigin;

	};
}

