#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;

// Глобальный мьютекс для защиты списка клиентов
mutex clients_mutex;

int ServerThread(int Client, sockaddr_in ClientAddr, vector<int>& allClients)
{
    // Добавляем клиента в список с защитой мьютексом
    {
        lock_guard<mutex> lock(clients_mutex);
        allClients.push_back(Client);
    }
    
    int ErrorStatus, MaxLength = 1024;
    char UserName[20];
    char* RecvBuffer = new char[MaxLength];  // Буфер для приема данных
    char* SendBuffer = new char[MaxLength];  // Буфер для отправки данных
    char ip[INET_ADDRSTRLEN];
    bool flag = true;

    inet_ntop(AF_INET, &(ClientAddr.sin_addr), ip, INET_ADDRSTRLEN); // Запись IP
    unsigned short port = ntohs(ClientAddr.sin_port); // Запись порта

    recv(Client, UserName, 20, 0);
    snprintf(SendBuffer, MaxLength, "Connect %s, ip: %s:%d\n", UserName, ip, port); // Вывод информации всем подключенным пользователям
    printf("%s", SendBuffer); // Вывод информации на сервер

    // Рассылка сообщения о подключении с защитой мьютексом
    {
        lock_guard<mutex> lock(clients_mutex);
        for (size_t i = 0; i < allClients.size(); i++)
            if (allClients[i] != Client)
                send(allClients[i], SendBuffer, strlen(SendBuffer), 0);
    }
    
    memset(SendBuffer, 0, MaxLength);

    while (flag)
    {
        memset(RecvBuffer, 0, MaxLength);
        memset(SendBuffer, 0, MaxLength);
        ErrorStatus = recv(Client, RecvBuffer, MaxLength, 0);
        
        if (ErrorStatus > 0)
        {
            RecvBuffer[ErrorStatus] = 0;
            if (strcmp(RecvBuffer, "/exit") == 0) // Проверка на попытку отключения
            {
                printf("Disconnect %s , ip %s\n", UserName, ip);
                snprintf(SendBuffer, MaxLength, "Disconnect %s\n", UserName);
                
                // Удаление клиента и рассылка с защитой мьютексом
                {
                    lock_guard<mutex> lock(clients_mutex);
                    for (size_t i = 0; i < allClients.size(); i++)
                    {
                        if (allClients.at(i) != Client)
                            send(allClients[i], SendBuffer, strlen(SendBuffer), 0);
                        else
                        {
                            allClients.erase(allClients.begin() + i);
                            i--; // Корректируем индекс после удаления
                        }
                    }
                }
                break;
            }

            printf("%s: %s\n", UserName, RecvBuffer);
            snprintf(SendBuffer, MaxLength, "%s: %s\n", UserName, RecvBuffer);

            // Рассылка сообщения всем клиентам с защитой мьютексом
            {
                lock_guard<mutex> lock(clients_mutex);
                for (size_t i = 0; i < allClients.size(); i++)
                    send(allClients[i], SendBuffer, strlen(SendBuffer), 0);
            }
        }
        else
        {
            printf("Error %s. Connection closed\n", ip);
            
            // Удаление клиента при ошибке с защитой мьютексом
            {
                lock_guard<mutex> lock(clients_mutex);
                for (size_t i = 0; i < allClients.size(); i++)
                    if (allClients[i] == Client)
                    {
                        allClients.erase(allClients.begin() + i);
                        break;
                    }
            }
            
            close(Client);
            delete[] SendBuffer;
            delete[] RecvBuffer;
            return 1;
        }
    }

    // Освобождение ресурсов при нормальном завершении
    close(Client);
    delete[] SendBuffer;
    delete[] RecvBuffer;
    return 0;
}

int main()
{
    setlocale(LC_ALL, "ru_RU.UTF-8");
    
    int ClientSocket;
    sockaddr_in ServerAddr;
    int ErrorStatus, MaxLength = 1024;
    unsigned short port = 2009;

    int ServerSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (ServerSocket == -1) {
        cout << "Socket creation failed" << endl;
        return 1;
    }

    // Установка опции для повторного использования адреса
    int opt = 1;
    if (setsockopt(ServerSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        cout << "Setsockopt failed" << endl;
        close(ServerSocket);
        return 1;
    }

    // Установка адресной информации для сокета
    ServerAddr.sin_addr.s_addr = INADDR_ANY; // Регистрирование сервера на всех адресах машины, на которой он запущен
    ServerAddr.sin_family = AF_INET; // Семейство
    ServerAddr.sin_port = htons(port); // Порт

    if (bind(ServerSocket, (sockaddr*)&ServerAddr, sizeof(ServerAddr)) == -1)
    {
        cout << "Bind failed" << endl;
        close(ServerSocket);
        return 1;
    }

    if (listen(ServerSocket, 50) == -1) {
        cout << "Listen failed" << endl;
        close(ServerSocket);
        return 1;
    }

    cout << "Server started on port " << port << endl;
    cout << "Waiting for connections..." << endl;

    vector<int> clients;

    while (true) {
        sockaddr_in ClientAddr;
        socklen_t ClientAddrSize = sizeof(ClientAddr);
        int ClientSocket = accept(ServerSocket, (struct sockaddr*)&ClientAddr, &ClientAddrSize);

        if (ClientSocket == -1)
        {
            cout << "Accept failed" << endl;
            continue;
        }

        cout << "Client connected: " << inet_ntoa(ClientAddr.sin_addr) << ":" << ntohs(ClientAddr.sin_port) << endl << endl;

        thread t(ServerThread, ClientSocket, ClientAddr, std::ref(clients));
        t.detach(); // Отсоединяет связанный поток от объекта thread
    }

    close(ServerSocket);
    return 0;
}