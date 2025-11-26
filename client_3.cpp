#define _CRT_SECURE_NO_WARNINGS
#include <locale.h>
#include <iostream>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment (lib, "Ws2_32.lib")

using namespace std;

void ClientThread(SOCKET ConnectSocket, char* recvbuffer, int size) {
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

int __cdecl main(void)
{
    SetConsoleCP(1251);
	SetConsoleOutputCP(1251);

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) 
    {
        cout << "WSAStartup failed" << endl;
        return 1;
    }

    SOCKET ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ConnectSocket == INVALID_SOCKET) {
        cout << "Socket creation failed" << endl;
        WSACleanup();
        return 1;
    }

	sockaddr_in ServerAddr;
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

    if (connect(ConnectSocket, (sockaddr*)&ServerAddr, sizeof(ServerAddr)) == SOCKET_ERROR) {
        cout << "Connection failed" << endl;
        closesocket(ConnectSocket);
        WSACleanup();
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
	closesocket(ConnectSocket);
	WSACleanup();
	system("pause");
	return 1;
}
