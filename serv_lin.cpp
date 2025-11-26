#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <netdb.h>

using namespace std;

// Функция для получения IP-адресов сервера
void printServerIPs() {
    cout << "=== Server IP Addresses ===" << endl;
    
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;
        
        int family = ifa->ifa_addr->sa_family;
        
        // IPv4
        if (family == AF_INET) {
            char host[NI_MAXHOST];
            int s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                               host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s == 0) {
                cout << "Interface: " << ifa->ifa_name << ", IPv4: " << host << endl;
            }
        }
    }
    freeifaddrs(ifaddr);
    cout << "============================" << endl << endl;
}

bool isValidNumber(const string &numStr) {
    if (numStr.empty() || !all_of(numStr.begin(), numStr.end(), ::isdigit)) {
        return false;
    }
    if (numStr.length() < 4) {
        return false;
    }
    int firstTwoSum = (numStr[0] - '0') + (numStr[1] - '0');
    int lastTwoSum = (numStr[numStr.length() - 2] - '0') + (numStr[numStr.length() - 1] - '0');
    return firstTwoSum == lastTwoSum;
}

string processNumbers(const string &input) {
    stringstream ss(input);
    string token;
    vector<string> validNumbers;

    while (getline(ss, token, ',')) {
        token.erase(remove_if(token.begin(), token.end(), ::isspace), token.end());
        if (isValidNumber(token)) {
            validNumbers.push_back(token);
        }
    }

    string result;
    for (size_t i = 0; i < validNumbers.size(); ++i) {
        result += validNumbers[i];
        if (i != validNumbers.size() - 1) {
            result += ",";
        }
    }
    return result;
}

int main(int argc, char *argv[]) {
    // Вывод IP-адресов сервера
    printServerIPs();

    // Проверка аргументов командной строки
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <port>" << endl;
        return 1;
    }

    int port;
    try {
        port = stoi(argv[1]);
    } catch (const exception&) {
        cerr << "Error: Port must be a valid integer." << endl;
        return 1;
    }

    if (port < 1 || port > 65535) {
        cerr << "Error: Port must be between 1 and 65535." << endl;
        return 1;
    }

    // Создание сокета
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        cerr << "Socket creation failed" << endl;
        return 1;
    }

    // Опция для повторного использования порта
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt SO_REUSEADDR failed");
        close(serverSocket);
        return 1;
    }

    // Настройка адреса сервера
    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); // Слушать на всех интерфейсах
    serverAddr.sin_port = htons(port);

    // Связывание сокета с адресом
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Bind failed");
        close(serverSocket);
        return 1;
    }

    // Прослушивание порта
    if (listen(serverSocket, 5) == -1) {
        perror("Listen failed");
        close(serverSocket);
        return 1;
    }

    cout << "Server started successfully on port " << port << endl;
    cout << "Waiting for connections..." << endl << endl;

    while (true) {
        sockaddr_in clientAddr;
        socklen_t clientSize = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientSize);

        if (clientSocket == -1) {
            perror("Accept failed");
            continue;
        }

        // Вывод информации о подключенном клиенте
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);
        cout << "New client connected:" << endl;
        cout << "  IP: " << clientIP << endl;
        cout << "  Port: " << ntohs(clientAddr.sin_port) << endl << endl;

        char buffer[1024];
        ssize_t bytesReceived;

        while (true) {
            memset(buffer, 0, sizeof(buffer));
            bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

            if (bytesReceived <= 0) {
                if (bytesReceived == -1) {
                    perror("Receive error");
                }
                cout << "Client " << clientIP << ":" << ntohs(clientAddr.sin_port) << " disconnected" << endl << endl;
                break;
            }

            string receivedData(buffer, bytesReceived);
            // Удалим возможный символ перевода строки, если клиент его отправляет
            if (!receivedData.empty() && receivedData.back() == '\n') {
                receivedData.pop_back();
            }
            if (!receivedData.empty() && receivedData.back() == '\r') {
                receivedData.pop_back();
            }

            cout << "Received from " << clientIP << ":" << ntohs(clientAddr.sin_port) << " : " << receivedData << endl;

            if (receivedData == "exit") {
                cout << "Client " << clientIP << " requested exit" << endl << endl;
                break;
            }

            string result = processNumbers(receivedData);
            if (result.empty()) {
                result = "No numbers matching the condition found";
            }

            send(clientSocket, result.c_str(), result.length(), 0);
            cout << "Sent to client: " << result << endl << endl;
        }

        close(clientSocket);
    }

    close(serverSocket);
    return 0;
}