// Cross-platform socket setup
#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

typedef SOCKET socket_t;
#define CLOSESOCK closesocket
#define SOCK_INIT()                                \
    do                                             \
    {                                              \
        WSADATA wsa;                               \
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) \
        {                                          \
            std::cerr << "WSAStartup failed\n";    \
            std::exit(1);                          \
        }                                          \
    } while (0)
#define SOCK_CLEANUP() WSACleanup()
#ifndef INVALID_SOCKET
#define INVALID_SOCKET (SOCKET)(~0)
#endif
#ifndef SOCKET_ERROR
#define SOCKET_ERROR (-1)
#endif

#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

typedef int socket_t;
#define CLOSESOCK close
#define SOCK_INIT() \
    do              \
    {               \
    } while (0)
#define SOCK_CLEANUP() \
    do                 \
    {                  \
    } while (0)
#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif
#ifndef SOCKET_ERROR
#define SOCKET_ERROR (-1)
#endif
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <map>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <cctype>

using namespace std;

string url_decode(const string &s)
{
    string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i)
    {
        if (s[i] == '+')
            out += ' ';
        else if (s[i] == '%' && i + 2 < s.size())
        {
            int hi = isdigit(s[i + 1]) ? s[i + 1] - '0' : tolower(s[i + 1]) - 'a' + 10;
            int lo = isdigit(s[i + 2]) ? s[i + 2] - '0' : tolower(s[i + 2]) - 'a' + 10;
            out += char((hi << 4) | lo);
            i += 2;
        }
        else
            out += s[i];
    }
    return out;
}

string now_timestamp()
{
    auto now = chrono::system_clock::now();
    auto t = chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&t);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return string(buf);
}

string generate_order_id()
{
    auto now = chrono::system_clock::now();
    auto t = chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&t);
    char buf[64];
    std::strftime(buf, sizeof(buf), "ORD%Y%m%d%H%M%S", &tm);
    int r = rand() % 900 + 100;
    std::ostringstream oss;
    oss << buf << r;
    return oss.str();
}

string read_file_to_string(const string &path)
{
    ifstream ifs(path, ios::binary);
    if (!ifs)
        return {};
    std::ostringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

void handle_client(socket_t client_fd)
{
    constexpr size_t BUF_SZ = 8192;
    string request;
    char buffer[BUF_SZ];
    int n;
    // read headers
    while ((n = recv(client_fd, buffer, BUF_SZ, 0)) > 0)
    {
        request.append(buffer, buffer + n);
        if (request.find("\r\n\r\n") != string::npos)
            break;
    }
    if (request.empty())
    {
        CLOSESOCK(client_fd);
        return;
    }

    // parse request line
    istringstream reqs(request);
    string request_line;
    getline(reqs, request_line);
    if (request_line.size() && request_line.back() == '\r')
        request_line.pop_back();
    string method, path, version;
    {
        istringstream rl(request_line);
        rl >> method >> path >> version;
    }

    // parse headers to find content-length
    size_t content_length = 0;
    string header;
    while (getline(reqs, header) && header != "\r" && !header.empty())
    {
        if (header.back() == '\r')
            header.pop_back();
        string lower = header;
        for (auto &c : lower)
            c = tolower(c);
        if (lower.find("content-length:") == 0)
        {
            string val = header.substr(header.find(':') + 1);
            content_length = stoi(val);
        }
    }

    string body;
    // if there is remaining data after headers in 'request', append
    auto pos = request.find("\r\n\r\n");
    if (pos != string::npos)
    {
        size_t hdrend = pos + 4;
        if (request.size() > hdrend)
            body = request.substr(hdrend);
    }
    // if not enough body read yet, read remaining
    while (body.size() < content_length)
    {
        n = recv(client_fd, buffer, BUF_SZ, 0);
        if (n <= 0)
            break;
        body.append(buffer, buffer + n);
    }

    if (method == "GET")
    {
        string file = "PageTest.html";
        if (path != "/" && path.find("/") == 0)
            file = path.substr(1);
        string content = read_file_to_string(file);
        if (content.empty())
        {
            string resp = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
            send(client_fd, resp.c_str(), resp.size(), 0);
            CLOSESOCK(client_fd);
            return;
        }
        string header = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n";
        header += "Content-Length: " + to_string(content.size()) + "\r\n\r\n";
        send(client_fd, header.c_str(), header.size(), 0);
        send(client_fd, content.c_str(), content.size(), 0);
        CLOSESOCK(client_fd);
        return;
    }

    if (method == "POST" && (path == "/order" || path == "/submit"))
    {
        // Expect JSON body (the PageTest.html sends application/json)
        string order_json = body;
        // Save to orders.txt with meta
        srand((unsigned)time(nullptr));

        string oid = generate_order_id();
        ofstream ofs("orders.txt", ios::app);
        ofs << "------------------------------\n";
        ofs << "Order ID: " << oid << "\n";
        ofs << "Date: " << now_timestamp() << "\n";
        ofs << order_json << "\n";
        ofs << "------------------------------\n\n";
        ofs.close();

        // respond with simple HTML confirmation
        string body_resp = "<h2>Order received</h2><p>Order ID: " + oid + "</p><p>Saved to orders.txt</p><p><a href=\"/\">Back</a></p>";
        string header = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n";
        header += "Content-Length: " + to_string(body_resp.size()) + "\r\n\r\n";
        send(client_fd, header.c_str(), header.size(), 0);
        send(client_fd, body_resp.c_str(), body_resp.size(), 0);
        CLOSESOCK(client_fd);
        return;
    }

    // Unsupported
    string resp = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
    send(client_fd, resp.c_str(), resp.size(), 0);
    CLOSESOCK(client_fd);
}

int main()
{
    srand((unsigned)time(nullptr));
    SOCK_INIT();
    socket_t server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET)
    {
        cerr << "socket() failed\n";
        SOCK_CLEANUP();
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        cerr << "bind() failed\n";
        CLOSESOCK(server_fd);
        return 1;
    }
    if (listen(server_fd, 16) < 0)
    {
        cerr << "listen() failed\n";
        CLOSESOCK(server_fd);
        return 1;
    }

    cout << "Server listening on http://localhost:8080/ (serve PageTest.html)\n"
         << endl;

    while (true)
    {
        struct sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        socket_t client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd == INVALID_SOCKET)
            continue;
        thread t(handle_client, client_fd);
        t.detach();
    }

    CLOSESOCK(server_fd);
    SOCK_CLEANUP();
    return 0;
}
