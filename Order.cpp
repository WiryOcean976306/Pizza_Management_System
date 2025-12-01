#include "Order.h"
#include "nlohmann/json.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <cstdlib>

using namespace std;
using json = nlohmann::json;

static string PrettyParseOrder(const string jSon, const string str) {
    string ToParse = jSon;
    json parsed = json::parse(ToParse);
    string result = parsed[str];
    result.erase(remove(result.begin(), result.end(), '"'), result.end());
    return result;
}

static string now_timestamp_order() {
    auto now = chrono::system_clock::now();
    auto t = chrono::system_clock::to_time_t(now);
    tm tm = *gmtime(&t);
    time_t cst_time = t - (6 * 3600); 
    tm = *gmtime(&cst_time);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return string(buf);
}

static string generate_order_id() {
    auto now = chrono::system_clock::now();
    auto t = chrono::system_clock::to_time_t(now);
    tm tm = *gmtime(&t);
    time_t cst_time = t - (6 * 3600); 
    tm = *gmtime(&cst_time);
    char buf[64];
    strftime(buf, sizeof(buf), "ORD%Y%m%d%H%M%S", &tm);
    int r = rand() % 900 + 100;
    ostringstream oss; oss << buf << r; return oss.str();
}

void processOrder(const string& json_str) {
    // Extract order info using PrettyParse
    string customerName, street, city, state, zipcode, cardName, cardNumber, expiry, cvv;
    try {
        customerName = PrettyParseOrder(json_str, "customerName");
        // Try to parse nested address object
        json parsed = json::parse(json_str);
        if (parsed.contains("address")) {
            auto addr = parsed["address"];
            street = addr.value("street", "");
            city = addr.value("city", "");
            state = addr.value("state", "");
            zipcode = addr.value("zipcode", "");
        }
        if (parsed.contains("payment")) {
            auto pay = parsed["payment"];
            cardName = pay.value("cardName", "");
            cardNumber = pay.value("cardNumber", "");
            expiry = pay.value("expiry", "");
            cvv = pay.value("cvv", "");
        }
    } catch (...) {
        // If parsing fails, continue with empty strings
        cerr << "Error parsing JSON for order" << endl;
    }
    
    // Generate order ID and save to orders.txt with parsed details
    srand((unsigned)time(nullptr));
    string oid = generate_order_id();
    ofstream ofs("orders.txt", ios::app);
    ofs << "------------------------------\n";
    ofs << "Order ID: " << oid << "\n";
    ofs << "Date: " << now_timestamp_order() << "\n";
    if (!customerName.empty()) {
        ofs << "Customer Name: " << customerName << "\n";
        ofs << "Address: " << street << ", " << city << ", " << state << " " << zipcode << "\n";
        ofs << "Payment: " << cardName << " (Card: " << cardNumber << ", Exp: " << expiry << ")\n";
    }
    ofs << "Full JSON:\n" << json_str << "\n";
    ofs << "------------------------------\n\n";
    ofs.close();
    
    cout << "Order received: " << oid << " from " << customerName << endl;
}