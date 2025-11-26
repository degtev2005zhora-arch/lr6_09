#include <iostream>
#include <string>
#include <cstring>      // для memset
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>     // для close()
#include <cstdlib>      // для atoi

using namespace std;

int main() {
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        cerr << "Socket creation failed" << endl;
        return 1;
    }

    string serverIP;
    int port;

    cout << "Enter server IP: ";
    cin >> serverIP;
    cout << "Enter server port: ";
    cin >> port;

    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    // Используем inet_pton вместо устаревшего inet_addr
    if (inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr) <= 0) {
        cerr << "Invalid IP address" << endl;
        close(clientSocket);
        return 1;
    }

    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        cerr << "Connection failed" << endl;
        close(clientSocket);
        return 1;
    }

    cout << "Type 'exit' to quit" << endl;

    string input;
    char buffer[1024];

    cin.ignore(); // Очистка буфера после cin >> port

    while (true) {
        cout << endl << "Enter numbers: ";
        getline(cin, input);

        if (input == "exit") {
            send(clientSocket, input.c_str(), input.length() + 1, 0);
            break;
        }

        // Отправка данных серверу
        send(clientSocket, input.c_str(), input.length() + 1, 0);

        // Получение ответа от сервера
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

        if (bytesReceived <= 0) {
            cout << "Server disconnected" << endl;
            break;
        }

        cout << "Server response: " << buffer << endl;
    }

    close(clientSocket);
    return 0;
}