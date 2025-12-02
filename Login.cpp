#include "Login.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>

using namespace std;
using json = nlohmann::json;

static string trim(const string &s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

bool processLogin(const string &json_str) {
    try {
        json parsed = json::parse(json_str);
        if (!parsed.contains("email") || !parsed.contains("password")) {
            return false;
        }
        string email = parsed["email"].get<string>();
        string password = parsed["password"].get<string>();

        ifstream in("AllAccounts.txt");
        if (!in) {
            cerr << "Could not open AllAccounts.txt" << endl;
            return false;
        }

        string line;
        string currentEmail, currentPassword;
        while (getline(in, line)) {
            if (line.rfind("Email:", 0) == 0) {
                currentEmail = trim(line.substr(6));
            } else if (line.rfind("Password:", 0) == 0) {
                currentPassword = trim(line.substr(9));
            } else if (line.find("------------------------------") == 0) {
                if (!currentEmail.empty() && !currentPassword.empty()) {
                    if (currentEmail == email && currentPassword == password) {
                        return true;
                    }
                }
                currentEmail.clear();
                currentPassword.clear();
            }
        }
        // Check last block if no trailing separator
        if (!currentEmail.empty() && !currentPassword.empty() && currentEmail == email && currentPassword == password) {
            return true;
        }
        return false;
    } catch (const exception &e) {
        cerr << "processLogin parse error: " << e.what() << endl;
        return false;
    } catch (...) {
        return false;
    }
}
