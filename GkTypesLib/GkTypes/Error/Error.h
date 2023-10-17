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

		//virtual String toString() const {
		//	String outString = String("Error: "_str).append(gk::Str::fromAscii(errorName())).append('\n');
		//	if (_errorFileOrigin != nullptr) {
		//		outString.append(gk::Str::fromAscii(_errorFileOrigin)).append('\n');
		//	}
		//	outString.append("Description: "_str).append(gk::Str::fromAscii(description())).append("\nCause: "_str).append(_errorCause);
		//	const Option<String> extraErrorInfo = extraInfo();
		//	if (extraErrorInfo.none()) {
		//		return outString;
		//	}
		//	return outString.append("\nInfo: "_str).append(extraErrorInfo.some());
		//}

	protected:

		// See ERROR_FILE_ORIGIN
		//Error(const String& errorCause, const char* errorFileOrigin = nullptr) : _errorCause(errorCause), _errorFileOrigin(errorFileOrigin) {}
		// See ERROR_FILE_ORIGIN
		//Error(String&& errorCause, const char* errorFileOrigin = nullptr) : _errorCause(std::move(errorCause)), _errorFileOrigin(errorFileOrigin) {}

		Error(const char* errorFileOrigin = nullptr) : _errorFileOrigin(errorFileOrigin) {}

		// Name of the error for printing. Should be independent of error instances.
		virtual const char* errorName() const = 0;

		// Simple description of the error. Should be independent of error instances.
		virtual const char* description() const = 0;

		// Information about the actual cause of the error.
		//const String& cause() const {
		//	return _errorCause;
		//}

		//virtual Option<String> extraInfo() const { return Option<String>(); }

	private:

		const char* _errorFileOrigin;
		//String _errorCause;

	};
}

