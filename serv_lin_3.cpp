#define _GNU_SOURCE
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
#include <sys/select.h>
#include <signal.h>

using namespace std;

// Глобальный мьютекс для защиты списка клиентов
mutex clients_mutex;
vector<int> clients; // SOCKET -> int

// Функция обработки клиента
void ClientHandler(int client_fd, struct sockaddr_in client_addr) {
    // Добавляем клиента в список
    {
        lock_guard<mutex> lock(clients_mutex);
        clients.push_back(client_fd);
    }

    char username[20] = {0};
    char recv_buffer[1024] = {0};
    char send_buffer[1024] = {0};

    // Получаем имя пользователя
    if (recv(client_fd, username, sizeof(username) - 1, 0) <= 0) {
        // Ошибка при получении имени — удаляем клиента
        lock_guard<mutex> lock(clients_mutex);
        clients.erase(remove(clients.begin(), clients.end(), client_fd), clients.end());
        close(client_fd);
        return;
    }
    username[sizeof(username) - 1] = '\0';

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client_addr.sin_port);

    // Формируем сообщение о подключении
    snprintf(send_buffer, sizeof(send_buffer), "Connect %s, ip: %s:%d\n", username, client_ip, client_port);
    cout << send_buffer;

    // Рассылаем всем, кроме себя
    {
        lock_guard<mutex> lock(clients_mutex);
        for (int fd : clients) {
            if (fd != client_fd) {
                send(fd, send_buffer, strlen(send_buffer), 0);
            }
        }
    }

    // Основной цикл обмена сообщениями
    while (true) {
        memset(recv_buffer, 0, sizeof(recv_buffer));
        ssize_t bytes_received = recv(client_fd, recv_buffer, sizeof(recv_buffer) - 1, 0);

        if (bytes_received <= 0) {
            // Клиент отключился или ошибка
            cout << "Disconnect " << username << ", ip " << client_ip << endl;

            snprintf(send_buffer, sizeof(send_buffer), "Disconnect %s\n", username);

            // Удаляем клиента и рассылаем сообщение
            {
                lock_guard<mutex> lock(clients_mutex);
                // Рассылка
                for (int fd : clients) {
                    if (fd != client_fd) {
                        send(fd, send_buffer, strlen(send_buffer), 0);
                    }
                }
                // Удаление
                clients.erase(remove(clients.begin(), clients.end(), client_fd), clients.end());
            }
            break;
        }

        recv_buffer[bytes_received] = '\0';
        // Удаляем возможные \r\n в конце (если клиент Windows)
        recv_buffer[strcspn(recv_buffer, "\r\n")] = 0;

        if (strcmp(recv_buffer, "/exit") == 0) {
            cout << "Disconnect " << username << ", ip " << client_ip << endl;
            snprintf(send_buffer, sizeof(send_buffer), "Disconnect %s\n", username);

            {
                lock_guard<mutex> lock(clients_mutex);
                for (int fd : clients) {
                    if (fd != client_fd) {
                        send(fd, send_buffer, strlen(send_buffer), 0);
                    }
                }
                clients.erase(remove(clients.begin(), clients.end(), client_fd), clients.end());
            }
            break;
        }

        cout << username << ": " << recv_buffer << endl;
        snprintf(send_buffer, sizeof(send_buffer), "%s: %s\n", username, recv_buffer);

        {
            lock_guard<mutex> lock(clients_mutex);
            for (int fd : clients) {
                if (fd != client_fd) {
                    send(fd, send_buffer, strlen(send_buffer), 0);
                }
            }
        }
    }

    close(client_fd);
}

// Обработчик SIGINT (Ctrl+C) для корректного завершения
void signal_handler(int sig) {
    cout << "\nShutting down server..." << endl;

    {
        lock_guard<mutex> lock(clients_mutex);
        for (int fd : clients) {
            close(fd);
        }
        clients.clear();
    }

    exit(0);
}

int main() {
    const int PORT = 2009;

    // Настройка обработчика сигнала
    signal(SIGINT, signal_handler);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        return 1;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        close(server_fd);
        return 1;
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

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

    cout << "Server started on port " << PORT << endl;
    cout << "Waiting for connections..." << endl;

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
        cout << "Client connected: " << ip_str << ":" << ntohs(client_addr.sin_port) << endl << endl;

        // Запускаем обработчик в отдельном потоке
        thread t(ClientHandler, client_fd, client_addr);
        t.detach();
    }

    close(server_fd);
    return 0;
}