#include <locale.h>
#include <iostream>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

void ClientThread(int ConnectSocket, char* recvbuffer, int size) {
    int ErrorStatus = 0;
    while (true)
    {
        ErrorStatus = recv(ConnectSocket, recvbuffer, size - 1, 0);
        if (ErrorStatus > 0)
        {
            recvbuffer[ErrorStatus] = 0;
            cout << recvbuffer << endl;
        }
        else
            break;
    }
}

int main(void)
{
    setlocale(LC_ALL, "ru_RU.UTF-8");

    int ConnectSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (ConnectSocket < 0) {
        cout << "Socket creation failed" << endl;
        return 1;
    }

    struct sockaddr_in ServerAddr;
    ServerAddr.sin_family = AF_INET;
    
    int ErrorStatus = 0, MaxLength = 1024;
    char* RecvBuffer = new char[MaxLength]; // Буфер для приема данных
    char* SendBuffer = new char[MaxLength]; // Буфер для отправки данных
    string ip;
    char* UserName = new char[20];
    unsigned short port;

    cout << "Enter server IP: ";
    cin >> ip;
    cout << "Enter server port: ";
    cin >> port;

    ServerAddr.sin_addr.s_addr = inet_addr(ip.c_str());
    ServerAddr.sin_port = htons(port);

    if (connect(ConnectSocket, (struct sockaddr*)&ServerAddr, sizeof(ServerAddr)) < 0) {
        cout << "Connection failed" << endl;
        close(ConnectSocket);
        return 1;
    }

    cout << "UserName: ";
    cin >> UserName;
    cin.ignore(); 
    send(ConnectSocket, UserName, 20, 0);

    thread t(ClientThread, ConnectSocket, RecvBuffer, MaxLength);
    t.detach();

    for (bool flag = true; flag;) {
        cin.getline(SendBuffer, MaxLength);
        if (!strcmp(SendBuffer, "/exit"))
            flag = false;
        send(ConnectSocket, SendBuffer, strlen(SendBuffer), 0);
    }

    delete[] UserName;
    delete[] SendBuffer;
    delete[] RecvBuffer;
    close(ConnectSocket);
    
    return 0;
}