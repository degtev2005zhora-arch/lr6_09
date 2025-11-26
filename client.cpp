#pragma comment(lib, "ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <iostream>
#include <string>

using namespace std;

int main() {
    // Инициализация Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "WSAStartup failed" << endl;
        return 1;
    }
    
    // Создание сокета
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        cout << "Socket creation failed" << endl;
        WSACleanup();
        return 1;
    }
    
    // Настройка адреса сервера

    //в структуре всего два поля –(1) первое поле хранит семейство адресов,второе поле хранит некие упакованные 
    //последовательно и упорядоченные данные в размере 14 - ти байт
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    
    string serverIP;
    int port;
    
    cout << "Enter server IP: ";
    cin >> serverIP;
    cout << "Enter server port: ";
    cin >> port;
    
    //Функция inet_addr преобразует строку, содержащую десятичный адрес с запятой IPv4, в правильный адрес для структуры IN_ADDR.
    serverAddr.sin_addr.s_addr = inet_addr(serverIP.c_str());

    //порт всегда указывается через вызов функции htons(), которая переупаковывает привычное цифровое значение портa
	// типа unsigned short в побайтовый порядок понятный для протокола TCP/IP	
    serverAddr.sin_port = htons(port);
    
    //Установка связи с сервером. 

    // Привязка сокета к конкретному процессу (bind()) не требу-ется,
	//т.к. сокет будет привязан к серверному Адресу и Порту через вызов функци
	//В структуру для клиента должна заноситься информация о сервере, т.е. IPv4-адрес сервера и номер «слушающего» порта на сервере.

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        //Присоединение к серверу. ПАРАМЕТРЫ коннекта Дескриптор, определяющий неподключенные сокеты,
        //Указатель на структуру sockaddr, с которой должно быть установлено соединение,Длина структуры sockaddr в байтах
        cout << "Connection failed" << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }
    
    cout << "Type 'exit' to quit" << endl;
    
    string input;
    char buffer[1024];
    
    cin.ignore(); // Очистка буфера
    
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
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        
        if (bytesReceived <= 0) {
            cout << "Server disconnected" << endl;
            break;
        }
        
        cout << "Server response: " << buffer << endl;
    }
    
    closesocket(clientSocket);
    WSACleanup();
    return 0;
}