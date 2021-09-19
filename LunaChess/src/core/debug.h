#ifndef LUNA_DEBUG_H
#define LUNA_DEBUG_H

#include <string_view>
#include <string>
#include <sstream>

#ifndef NDEBUG
	#define LUNA_ASSERTS_ON
#endif

#ifdef LUNA_ASSERTS_ON

#define LUNA_ASSERT(cond, msg) \
		if (!(cond)) {               \
			std::stringstream ASSERTSSTREAM_; \
			ASSERTSSTREAM_ << msg; \
			std::string ASSERTSTR_ = ASSERTSSTREAM_.str(); \
			::lunachess::debug::assertionFailure(__FILE__, __func__, __LINE__, ASSERTSTR_); \
		}

#else

#define LUNA_ASSERT(cond, msg)

#endif

#define LUNA_NOT_NULL(ptr) LUNA_ASSERT(ptr != nullptr, "'" #ptr "' cannot be NULL.")


namespace lunachess::debug {

using AssertionFailHandler = void (*)(const char* fileName, 
	                                  const char* funcName, 
	                                  int line, 
	                                  std::string_view msg);

/**
	Asserts that a given condition is true. If the assertion fails, the last set assertion fail
	handler is called. By default, this handler dumps the assertion failure message to STDERR,
	awaits for a keystroke (std::cin.get()) and exits the program. The handler can be changed
	via 'setAssertFailHandler'.

	Note: It is not advisable to call this function directly. Use the macro LUNA_ASSERT instead.
*/
void assertionFailure(const char* fileName, const char* funcName, int line, std::string_view msg);

/**
	Sets the assertion handler. If nullptr is passed, resets to the default assertion handler.
*/
void setAssertFailHandler(AssertionFailHandler h);

}

#endif // LUNA_DEBUG_H
