#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ctime>
#include <sstream>
#include "Order.h"
#include "OrderHistory.h"
#include "AccountRegistration.h"
#include "Login.h"
// Add other processor headers as needed

using namespace std;

// URL decode helper function
string url_decode(const string& str) {
    string result;
    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == '%' && i + 2 < str.length()) {
            int value;
            istringstream is(str.substr(i + 1, 2));
            if (is >> hex >> value) {
                result += static_cast<char>(value);
                i += 2;
            } else {
                result += str[i];
            }
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}

void send_file(int client_fd, const string& filename) {
    ifstream file(filename);
    if (!file) {
        string resp = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        send(client_fd, resp.c_str(), resp.size(), 0);
        return;
    }
    string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    string header = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n";
    header += "Content-Length: " + to_string(content.size()) + "\r\n\r\n";
    send(client_fd, header.c_str(), header.size(), 0);
    send(client_fd, content.c_str(), content.size(), 0);
}

void handle_json(const string& path, const string& json_str) {
    if (path == "/order") {
        processOrder(json_str);
        processHistory(json_str);
    } else if (path == "/account") {
        processAccount(json_str);
    } else if (path == "/login") {
        // Handled specially in handle_client for status codes; keep placeholder
    }
}

void handle_client(int client_fd) {
    char buffer[8192];
    int bytes = recv(client_fd, buffer, sizeof(buffer)-1, 0);
    if (bytes <= 0) { close(client_fd); return; }
    buffer[bytes] = 0;
    string request(buffer);

    // Parse HTTP method and path
    size_t method_end = request.find(' ');
    size_t path_end = request.find(' ', method_end + 1);
    string method = request.substr(0, method_end);
    string path = request.substr(method_end + 1, path_end - method_end - 1);

    // Find body
    size_t body_pos = request.find("\r\n\r\n");
    string body = (body_pos != string::npos) ? request.substr(body_pos + 4) : "";

    if (method == "GET") {
        // Handle order history request
        if (path.find("/orderhistory?email=") == 0) {
            size_t email_start = path.find("=") + 1;
            string email_encoded = path.substr(email_start);
            string email = url_decode(email_encoded);
            string history_file = "OrderHistory/" + email + ".txt";
            
            cout << "Order history request for email: " << email << endl;
            cout << "Looking for file: " << history_file << endl;
            
            ifstream file(history_file);
            if (!file) {
                cout << "File not found: " << history_file << endl;
                string resp = "HTTP/1.1 200 OK\r\nContent-Type: text/plain; charset=utf-8\r\nContent-Length: 0\r\n\r\n";
                send(client_fd, resp.c_str(), resp.size(), 0);
                close(client_fd);
                return;
            }
            
            string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
            cout << "Found file, content length: " << content.size() << endl;
            string header = "HTTP/1.1 200 OK\r\nContent-Type: text/plain; charset=utf-8\r\n";
            header += "Content-Length: " + to_string(content.size()) + "\r\n\r\n";
            send(client_fd, header.c_str(), header.size(), 0);
            send(client_fd, content.c_str(), content.size(), 0);
            close(client_fd);
            return;
        }
        
        string file = "MainPizzapage.html";
        if (path != "/" && path.find("/") == 0) {
            file = path.substr(1); // Remove leading /
        }
        send_file(client_fd, file);
        close(client_fd);
        return;
    }

    if (method == "POST") {
        if (path == "/login") {
            bool ok = processLogin(body);
            string body_resp = ok ? "Login successful" : "Invalid email or password";
            string status = ok ? "HTTP/1.1 200 OK" : "HTTP/1.1 401 Unauthorized";
            string header = status + "\r\nContent-Type: text/plain; charset=utf-8\r\n";
            header += "Content-Length: " + to_string(body_resp.size()) + "\r\n\r\n";
            send(client_fd, header.c_str(), header.size(), 0);
            send(client_fd, body_resp.c_str(), body_resp.size(), 0);
            close(client_fd);
            return;
        }
        handle_json(path, body);
        string resp = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK";
        send(client_fd, resp.c_str(), resp.size(), 0);
        close(client_fd);
        return;
    }

    // Default: 404
    string resp = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
    send(client_fd, resp.c_str(), resp.size(), 0);
    close(client_fd);
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }
    listen(server_fd, 10);

    cout << "Server running on port 8080\n";
    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd >= 0) handle_client(client_fd);
    }
    close(server_fd);
    return 0;
}