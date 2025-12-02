#ifndef LOGIN_H
#define LOGIN_H

#include <string>

// Validate login credentials against AllAccounts.txt
// Returns true if email/password pair exists.
bool processLogin(const std::string& json_str);

#endif // LOGIN_H
