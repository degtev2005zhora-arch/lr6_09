#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <algorithm>
#include <cstdio> // <-- для snprintf

using namespace std;

mutex clients_mutex;

void printServerIPs() {
    cout << "=== Server IP Addresses ===" << endl;
    struct ifaddrs *ifaddrs_ptr, *ifa;
    if (getifaddrs(&ifaddrs_ptr) == -1) {
        perror("getifaddrs");
        return;
    }

    for (ifa = ifaddrs_ptr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) continue;
        if (ifa->ifa_addr->sa_family == AF_INET) {
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr, ip_str, INET_ADDRSTRLEN);
            cout << "Interface: " << ifa->ifa_name << " → IPv4: " << ip_str << endl;
        }
    }
    freeifaddrs(ifaddrs_ptr);
    cout << "============================" << endl << endl;
}

void ServerThread(int client_fd, struct sockaddr_in client_addr, vector<int>& allClients) {
    {
        lock_guard<mutex> lock(clients_mutex);
        allClients.push_back(client_fd);
    }

    char username[20] = {0};
    char recv_buffer[1024] = {0};
    char send_buffer[1024] = {0};

    ssize_t bytes = recv(client_fd, username, sizeof(username) - 1, 0);
    if (bytes <= 0) {
        lock_guard<mutex> lock(clients_mutex);
        allClients.erase(std::remove(allClients.begin(), allClients.end(), client_fd), allClients.end());
        close(client_fd);
        return;
    }
    username[bytes] = '\0';

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client_addr.sin_port);

    snprintf(send_buffer, sizeof(send_buffer), "Connect %.19s, ip: %s:%d\n", username, client_ip, client_port);
    cout << send_buffer;

    {
        lock_guard<mutex> lock(clients_mutex);
        for (int fd : allClients) {
            if (fd != client_fd) {
                send(fd, send_buffer, strlen(send_buffer), 0);
            }
        }
    }

    while (true) {
        memset(recv_buffer, 0, sizeof(recv_buffer));
        ssize_t received = recv(client_fd, recv_buffer, sizeof(recv_buffer) - 1, 0);

        if (received <= 0) {
            cout << "Client disconnected: " << username << " (" << client_ip << ":" << client_port << ")" << endl;
            snprintf(send_buffer, sizeof(send_buffer), "Disconnect %.19s\n", username);

            {
                lock_guard<mutex> lock(clients_mutex);
                for (int fd : allClients) {
                    if (fd != client_fd) {
                        send(fd, send_buffer, strlen(send_buffer), 0);
                    }
                }
                allClients.erase(std::remove(allClients.begin(), allClients.end(), client_fd), allClients.end());
            }
            break;
        }

        recv_buffer[received] = '\0';
        recv_buffer[strcspn(recv_buffer, "\r\n")] = '\0';

        if (strcmp(recv_buffer, "/exit") == 0) {
            cout << "Client requested exit: " << username << " (" << client_ip << ":" << client_port << ")" << endl;
            snprintf(send_buffer, sizeof(send_buffer), "Disconnect %.19s\n", username);

            {
                lock_guard<mutex> lock(clients_mutex);
                for (int fd : allClients) {
                    if (fd != client_fd) {
                        send(fd, send_buffer, strlen(send_buffer), 0);
                    }
                }
                allClients.erase(std::remove(allClients.begin(), allClients.end(), client_fd), allClients.end());
            }
            break;
        }

        cout << username << " (" << client_ip << ":" << client_port << "): " << recv_buffer << endl;
        snprintf(send_buffer, sizeof(send_buffer), "%.19s: %.1000s\n", username, recv_buffer);

        {
            lock_guard<mutex> lock(clients_mutex);
            for (int fd : allClients) {
                if (fd != client_fd) {
                    send(fd, send_buffer, strlen(send_buffer), 0);
                }
            }
        }
    }

    close(client_fd);
}

int main() {
    printServerIPs();

    int port;
    cout << "Enter server port (1–65535): ";
    if (!(cin >> port) || port < 1 || port > 65535) {
        cerr << "Invalid port number!" << endl;
        return 1;
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        return 1;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt SO_REUSEADDR");
        close(server_fd);
        return 1;
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // INADDR_ANY уже в сетевом порядке
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 50) == -1) {
        perror("Listen failed");
        close(server_fd);
        return 1;
    }

    cout << "\nServer started successfully on port " << port << endl;
    cout << "Waiting for connections..." << endl << endl;

    vector<int> clients;

    while (true) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);

        if (client_fd == -1) {
            perror("Accept failed");
            continue;
        }

        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, INET_ADDRSTRLEN);
        int client_port = ntohs(client_addr.sin_port);
        cout << "New client connected: " << ip_str << ":" << client_port << endl << endl;

        thread t(ServerThread, client_fd, client_addr, ref(clients));
        t.detach();
    }

    close(server_fd);
    return 0;
}