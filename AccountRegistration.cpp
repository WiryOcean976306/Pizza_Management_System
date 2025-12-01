#include "AccountRegistration.h"
#include "nlohmann/json.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <algorithm>

using namespace std;
using json = nlohmann::json;

string PrettyParse(const string jSon, const string str) {
    string ToParse = jSon;
    json parsed = json::parse(ToParse);
    string result = parsed[str];
    result.erase(remove(result.begin(), result.end(), '"'), result.end());
    return result;
}

string now_timestamp() {
    auto now = chrono::system_clock::now();
    auto t = chrono::system_clock::to_time_t(now);
    tm tm = *gmtime(&t);
    time_t cst_time = t - (6 * 3600); 
    tm = *gmtime(&cst_time);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return string(buf);
}

void processAccount(const string& json_str) {
    // Extract account info using PrettyParse
    string firstName, lastName, email, password, phone;
    try {
        firstName = PrettyParse(json_str, "firstName");
        lastName = PrettyParse(json_str, "lastName");
        email = PrettyParse(json_str, "email");
        password = PrettyParse(json_str, "password");
        phone = PrettyParse(json_str, "phone");
    } catch (...) {
        // If parsing fails, continue with empty strings
        cerr << "Error parsing JSON for account registration" << endl;
    }
    
    // If account info was found, save to AllAccounts.txt
    if (!firstName.empty() || !lastName.empty() || !email.empty()) {
        ofstream ofs_accounts("AllAccounts.txt", ios::app);
        ofs_accounts << "------------------------------\n";
        ofs_accounts << "First Name: " << firstName << "\n";
        ofs_accounts << "Last Name: " << lastName << "\n";
        ofs_accounts << "Email: " << email << "\n";
        ofs_accounts << "Password: " << password << "\n";
        if (!phone.empty()) {
            ofs_accounts << "Phone: " << phone << "\n";
        }
        ofs_accounts << "Date: " << now_timestamp() << "\n";
        ofs_accounts << "------------------------------\n\n";
        ofs_accounts.close();
        
        cout << "Account registered: " << firstName << " " << lastName << " (" << email << ")" << endl;
    }
}
