#ifndef APP_INCLUDE_APP_VERSION_HPP
#define APP_INCLUDE_APP_VERSION_HPP


// Must be char [] for sizeof
#ifndef MOCK_DATA
constexpr const char VERSION[] = "0.1";
#else
constexpr const char VERSION[] = "0.1-mock";
#endif

#endif