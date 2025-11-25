#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cctype>        // для ::isdigit и ::isspace
#include <cstring>       // для memset
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>      // для close()

using namespace std;

// Функция для проверки, удовлетворяет ли число условию
bool isValidNumber(const string &numStr) {
    // Проверяем, что строка состоит только из цифр
    if (numStr.empty() || !all_of(numStr.begin(), numStr.end(), ::isdigit)) {
        return false;
    }

    // Число должно содержать как минимум 4 цифры
    if (numStr.length() < 4) {
        return false;
    }

    // Сумма первых двух и последних двух цифр
    int firstTwoSum = (numStr[0] - '0') + (numStr[1] - '0');
    int lastTwoSum = (numStr[numStr.length() - 2] - '0') + (numStr[numStr.length() - 1] - '0');

    return firstTwoSum == lastTwoSum;
}

// Функция для обработки строки с числами
string processNumbers(const string &input) {
    stringstream ss(input);
    string token;
    vector<string> validNumbers;

    // Разделяем строку по запятым
    while (getline(ss, token, ',')) {
        // Удаляем пробелы вокруг числа
        token.erase(remove_if(token.begin(), token.end(), ::isspace), token.end());

        if (isValidNumber(token)) {
            validNumbers.push_back(token);
        }
    }

    // Формируем результат
    string result;
    for (size_t i = 0; i < validNumbers.size(); ++i) {
        result += validNumbers[i];
        if (i != validNumbers.size() - 1) {
            result += ",";
        }
    }

    return result;
}

int main() {
    // Создание сокета
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        cerr << "Socket creation failed" << endl;
        return 1;
    }

    // Настройка адреса сервера
    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);  // Привязка ко всем интерфейсам
    serverAddr.sin_port = htons(2009);               // Порт = 2000 + номер бригады

    // Связывание сокета с адресом
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        cerr << "Bind failed" << endl;
        close(serverSocket);
        return 1;
    }

    // Прослушивание порта
    if (listen(serverSocket, 1) == -1) {
        cerr << "Listen failed" << endl;
        close(serverSocket);
        return 1;
    }

    cout << "Server started on port 2009" << endl;
    cout << "Waiting for connections..." << endl;

    while (true) {
        sockaddr_in clientAddr;
        socklen_t clientSize = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientSize);

        if (clientSocket == -1) {
            cerr << "Accept failed" << endl;
            continue;
        }

        cout << "Client connected: "
             << inet_ntoa(clientAddr.sin_addr) << ":"
             << ntohs(clientAddr.sin_port) << endl << endl;

        char buffer[1024];
        ssize_t bytesReceived;

        while (true) {
            memset(buffer, 0, sizeof(buffer));

            // Получение данных от клиента
            bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

            if (bytesReceived <= 0) {
                cout << "Client disconnected" << endl;
                break;
            }

            string receivedData(buffer, bytesReceived); // важно: не до \0, а по количеству байт
            cout << "Received from client: " << receivedData << endl;

            // Проверка на команду выхода
            if (receivedData == "exit") {
                cout << "Client requested exit" << endl;
                break;
            }

            // Обработка чисел
            string result = processNumbers(receivedData);
            if (result.empty()) {
                result = "No numbers matching the condition found";
            }

            // Отправка результата клиенту
            send(clientSocket, result.c_str(), result.length(), 0); // НЕ +1 — клиент может ждать точное кол-во байт
            cout << "Sent to client: " << result << endl << endl;
        }

        close(clientSocket);
    }

    close(serverSocket);
    return 0;
}#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cctype>        // для ::isdigit и ::isspace
#include <cstring>       // для memset
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>      // для close()

using namespace std;

// Функция для проверки, удовлетворяет ли число условию
bool isValidNumber(const string &numStr) {
    // Проверяем, что строка состоит только из цифр
    if (numStr.empty() || !all_of(numStr.begin(), numStr.end(), ::isdigit)) {
        return false;
    }

    // Число должно содержать как минимум 4 цифры
    if (numStr.length() < 4) {
        return false;
    }

    // Сумма первых двух и последних двух цифр
    int firstTwoSum = (numStr[0] - '0') + (numStr[1] - '0');
    int lastTwoSum = (numStr[numStr.length() - 2] - '0') + (numStr[numStr.length() - 1] - '0');

    return firstTwoSum == lastTwoSum;
}

// Функция для обработки строки с числами
string processNumbers(const string &input) {
    stringstream ss(input);
    string token;
    vector<string> validNumbers;

    // Разделяем строку по запятым
    while (getline(ss, token, ',')) {
        // Удаляем пробелы вокруг числа
        token.erase(remove_if(token.begin(), token.end(), ::isspace), token.end());

        if (isValidNumber(token)) {
            validNumbers.push_back(token);
        }
    }

    // Формируем результат
    string result;
    for (size_t i = 0; i < validNumbers.size(); ++i) {
        result += validNumbers[i];
        if (i != validNumbers.size() - 1) {
            result += ",";
        }
    }

    return result;
}

int main() {
    // Создание сокета
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        cerr << "Socket creation failed" << endl;
        return 1;
    }

    // Настройка адреса сервера
    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);  // Привязка ко всем интерфейсам
    serverAddr.sin_port = htons(2009);               // Порт = 2000 + номер бригады

    // Связывание сокета с адресом
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        cerr << "Bind failed" << endl;
        close(serverSocket);
        return 1;
    }

    // Прослушивание порта
    if (listen(serverSocket, 1) == -1) {
        cerr << "Listen failed" << endl;
        close(serverSocket);
        return 1;
    }

    cout << "Server started on port 2009" << endl;
    cout << "Waiting for connections..." << endl;

    while (true) {
        sockaddr_in clientAddr;
        socklen_t clientSize = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientSize);

        if (clientSocket == -1) {
            cerr << "Accept failed" << endl;
            continue;
        }

        cout << "Client connected: "
             << inet_ntoa(clientAddr.sin_addr) << ":"
             << ntohs(clientAddr.sin_port) << endl << endl;

        char buffer[1024];
        ssize_t bytesReceived;

        while (true) {
            memset(buffer, 0, sizeof(buffer));

            // Получение данных от клиента
            bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

            if (bytesReceived <= 0) {
                cout << "Client disconnected" << endl;
                break;
            }

            string receivedData(buffer, bytesReceived); // важно: не до \0, а по количеству байт
            cout << "Received from client: " << receivedData << endl;

            // Проверка на команду выхода
            if (receivedData == "exit") {
                cout << "Client requested exit" << endl;
                break;
            }

            // Обработка чисел
            string result = processNumbers(receivedData);
            if (result.empty()) {
                result = "No numbers matching the condition found";
            }

            // Отправка результата клиенту
            send(clientSocket, result.c_str(), result.length(), 0); // НЕ +1 — клиент может ждать точное кол-во байт
            cout << "Sent to client: " << result << endl << endl;
        }

        close(clientSocket);
    }

    close(serverSocket);
    return 0;
}

