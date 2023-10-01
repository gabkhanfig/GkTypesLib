#pragma once

#include "../String/String.h"
#include "../Option/Option.h"

#define _STRINGIZE_DETAIL(x) #x
#define _STRINGIZE(x) _STRINGIZE_DETAIL(x) // C/C++ macros are stupid but this works.
#define ERROR_FILE_ORIGIN ("File: " __FILE__ "\nLine: " _STRINGIZE(__LINE__))


namespace gk 
{
	class Error
	{
	public:

		virtual String toString() const {
			String outString = String("Error: ").Append(errorName()).Append('\n');

			if (_errorFileOrigin != nullptr) {
				outString.Append(_errorFileOrigin).Append('\n');
			}

			outString.Append("Description: ").Append(description()).Append("\nCause: ").Append(_errorCause);

			const Option<String> extraErrorInfo = extraInfo();
			if (extraErrorInfo.none()) {
				return outString;
			}

			return outString.Append("\nInfo: ").Append(extraErrorInfo.some());
		}

	protected:

		// See ERROR_FILE_ORIGIN
		Error(const String& errorCause, const char* errorFileOrigin = nullptr) : _errorCause(errorCause), _errorFileOrigin(errorFileOrigin) {}
		// See ERROR_FILE_ORIGIN
		Error(String&& errorCause, const char* errorFileOrigin = nullptr) : _errorCause(std::move(errorCause)), _errorFileOrigin(errorFileOrigin) {}

		// Name of the error for printing. Should be independent of error instances.
		virtual const char* errorName() const = 0;

		// Simple description of the error. Should be independent of error instances.
		virtual const char* description() const = 0;

		// Information about the actual cause of the error.
		const String& cause() const {
			return _errorCause;
		}

		virtual Option<String> extraInfo() const { return Option<String>(); }

	private:

		const char* _errorFileOrigin;
		String _errorCause;

	};
}

