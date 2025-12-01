#include "OrderHistory.h"
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
#include <sys/stat.h>

using namespace std;
using json = nlohmann::json;

static string now_timestamp_history() {
    auto now = chrono::system_clock::now();
    auto t = chrono::system_clock::to_time_t(now);
    tm tm = *gmtime(&t);
    time_t cst_time = t - (6 * 3600); 
    tm = *gmtime(&cst_time);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return string(buf);
}

static string get_date() {
    auto now = chrono::system_clock::now();
    auto t = chrono::system_clock::to_time_t(now);
    tm tm = *gmtime(&t);
    time_t cst_time = t - (6 * 3600); 
    tm = *gmtime(&cst_time);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d", &tm);
    return string(buf);
}

static string get_time() {
    auto now = chrono::system_clock::now();
    auto t = chrono::system_clock::to_time_t(now);
    tm tm = *gmtime(&t);
    time_t cst_time = t - (6 * 3600); 
    tm = *gmtime(&cst_time);
    char buf[64];
    strftime(buf, sizeof(buf), "%H:%M:%S", &tm);
    return string(buf);
}

static string generate_order_id_history() {
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

static double calculate_total(const json& pizzas) {
    double total = 0.0;
    if (!pizzas.is_array()) return total;
    
    for (const auto& pizza : pizzas) {
        if (pizza.contains("size")) {
            string size_str = pizza["size"].get<string>();
            total += stod(size_str);
        }
        if (pizza.contains("toppings") && pizza["toppings"].is_array()) {
            // Each topping adds $1-$2 (approximate from the form)
            total += pizza["toppings"].size() * 1.5;
        }
    }
    return total;
}

void processHistory(const string& json_str) {
    string customerName, email, street, city, state, zipcode;
    string orderID, date, time;
    double total = 0.0;
    
    try {
        json parsed = json::parse(json_str);
        
        // Extract customer info
        customerName = parsed.value("customerName", "");
        
        // Extract email (from logged-in user data if available, or from order)
        email = parsed.value("email", "");
        
        // Extract address
        if (parsed.contains("address")) {
            auto addr = parsed["address"];
            street = addr.value("street", "");
            city = addr.value("city", "");
            state = addr.value("state", "");
            zipcode = addr.value("zipcode", "");
        }
        
        // Generate order details
        srand((unsigned)std::time(nullptr));
        orderID = generate_order_id_history();
        date = get_date();
        time = get_time();
        
        // Calculate total from pizzas
        if (parsed.contains("pizzas")) {
            total = calculate_total(parsed["pizzas"]);
        }
        
        // Create OrderHistory directory if it doesn't exist
        mkdir("OrderHistory", 0755);
        
        // If no email in order, try to extract from localStorage (would need to be sent from frontend)
        // For now, save to a default file if email is missing
        if (email.empty()) {
            email = "guest";
        }
        
        // Save to OrderHistory/<email>.txt
        string outputPath = "OrderHistory/" + email + ".txt";
        ofstream ofs(outputPath, ios::app);
        
        if (!ofs.is_open()) {
            cerr << "Error: Could not create " << outputPath << endl;
            return;
        }
        
        ofs << "------------------------------\n";
        ofs << "Order ID: " << orderID << "\n";
        ofs << "Customer Name: " << customerName << "\n";
        ofs << "Address: " << street << ", " << city << ", " << state << " " << zipcode << "\n";
        ofs << "Date: " << date << "\n";
        ofs << "Time: " << time << "\n";
        ofs << "Total: $" << fixed << setprecision(2) << total << "\n";
        
        // Add pizza details
        if (parsed.contains("pizzas") && parsed["pizzas"].is_array()) {
            ofs << "Pizzas:\n";
            int pizzaNum = 1;
            for (const auto& pizza : parsed["pizzas"]) {
                ofs << "  Pizza " << pizzaNum++ << ": ";
                if (pizza.contains("size")) {
                    string size = pizza["size"].get<string>();
                    ofs << "Size $" << size;
                }
                if (pizza.contains("toppings") && pizza["toppings"].is_array()) {
                    ofs << ", Toppings: ";
                    bool first = true;
                    for (const auto& topping : pizza["toppings"]) {
                        if (!first) ofs << ", ";
                        ofs << topping.get<string>();
                        first = false;
                    }
                }
                ofs << "\n";
            }
        }
        
        ofs << "------------------------------\n\n";
        ofs.close();
        
        cout << "Order history saved to " << outputPath << " (Order ID: " << orderID << ")" << endl;
        
    } catch (const exception& e) {
        cerr << "Error processing order history: " << e.what() << endl;
    } catch (...) {
        cerr << "Unknown error processing order history" << endl;
    }
}