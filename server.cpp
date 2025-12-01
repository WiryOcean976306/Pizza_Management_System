#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ctime>
#include "Order.h"
#include "AccountRegistration.h"
// Add other processor headers as needed

using namespace std;

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
        processOrder(json_str); // defined in Order.cpp
    } else if (path == "/account") {
        processAccount(json_str); // defined in AccountRegistration.cpp
    }
    // Add more endpoints as needed
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
        string file = "MainPizzapage.html";
        if (path != "/" && path.find("/") == 0) {
            file = path.substr(1); // Remove leading /
        }
        send_file(client_fd, file);
        close(client_fd);
        return;
    }

    if (method == "POST") {
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