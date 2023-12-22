#include <iostream>
#include <csignal>
#include <cstring>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <unordered_map>
#include <mutex>


using namespace std;

long long int endrand = 1000000000; //length of params
int port = 12345;
int serverSocket;

// функция рандома
long long int irand(long long int a, long long int b){
    return a + time(NULL)*rand()%(b-a+1);
}

struct keys
{
    long long int k1;
    long long int k2;
    long long int P1;

};

int sendLine(int clientSocket, string message){
    const char* sendtext = message.c_str();
    int bytesSent = send(clientSocket, sendtext, message.length(), 0);

    // Проверяем успешность отправки
    if (bytesSent == -1) {
        cerr << "Ошибка при отправке сообщения\n";
        return -1;
    }
    return 0;
}
string reciveLine(int clientSocket){
    char buffer[1024];
    int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    
    // Проверяем успешность получения
    if (bytesRead <= 0) {
        cerr << "Ошибка при получении сообщения от сервера\n";
    }
    buffer[bytesRead] = '\0'; // Устанавливаем нуль-терминатор
    return string(buffer);
}


// Создание словаря из id и буффера ключей
unordered_map<int, keys> idbuffer;

// Функция для обработки соединения с клиентом в отдельном потоке
mutex myMutex;
int handleClient(int clientSocket) {
    int id = stoi(reciveLine(clientSocket));
    cout << "Получено от клиента ID: " << id <<'\n'; 
    
    
        unique_lock<mutex> lock(myMutex);
        auto index = idbuffer.find(id);
        // Проверка отсутствия id в словаре
        if (index == idbuffer.end()){
            keys idkeys;
            // задаём начальные параметры в буффере
            idkeys.k1=irand(1, endrand);
            idkeys.k2=irand(1, endrand);
            // отправляем два ключа первому клиенту
            string messageGP = to_string(idkeys.k1) + ' ' + to_string(idkeys.k2);
            sendLine(clientSocket, messageGP);
            cout << "ID " << id <<": параметры отправлены , ожидание ключа\n";
            // Получение первого ключа
            idkeys.P1 = stoll(reciveLine(clientSocket));
            // Кладём в буффер три значения, g, p, A
            idbuffer[id] = idkeys;
            lock.unlock();
            while(idbuffer[id].P1 == idkeys.P1){
                sleep(1);
            }
            sendLine(clientSocket, to_string(idbuffer[id].P1));
            // Очистка буфера после соединения
            idbuffer.erase(id);
        }
    
        else{
            // Отправка всех параметров и открытого ключа
            keys fullKeys = idbuffer[id];
            string messageGPB = to_string(fullKeys.k1) + ' ' + to_string(fullKeys.k2) + ' ' + to_string(fullKeys.P1);
            sendLine(clientSocket, messageGPB);
            idbuffer[id].P1 = stoll(reciveLine(clientSocket));
    
        }
        // cout <<"ID: " << id << ' ' idkeys.k1 << ' ' << idkeys.k2 << ' ' << idkeys.P1 << '\n';
    


    // Отправляем данные обратно клиенту
    // send(clientSocket, buffer, strlen(buffer), 0);

    // Закрываем сокет при завершении работы
    close(clientSocket);
    return 0;
}


volatile sig_atomic_t gSignalStatus = 0;

void signalHandler(int signum){
    if (signum == SIGINT) {
        gSignalStatus = signum;
        cout << "Exiting...\n";
        close(serverSocket);
        exit(signum);
    }
}
int main() {
    // Создаем сокет
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        cerr << "Ошибка при создании сокета\n";
        return -1;
    }
    signal(SIGINT, signalHandler);

    // Настраиваем серверный адрес
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port); // Порт сервера
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // Привязываем сокет к адресу и порту
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        cerr << "Ошибка при привязке сокета\n";
        close(serverSocket);
        return -1;
    }

    // Слушаем входящие соединения
    if (listen(serverSocket, 100) == -1) {
        cerr << "Ошибка при прослушивании сокета\n"; 
        close(serverSocket);
        return -1;
    }

    cout << "Сервер ожидает соединений...\n"; 

    // Вектор для хранения потоков
    vector<thread> threads;

    // Принимаем клиентов и обрабатываем их в отдельных потоках
    while (gSignalStatus == 0) {
        // Принимаем новое соединение
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == -1) {
            cerr << "Ошибка при принятии соединения\n"; 
            break;
        }

        cout << "Принято соединение\n";

        // Запускаем новый поток для обработки соединения с клиентом
        threads.emplace_back(handleClient, clientSocket);
    }

    // Дожидаемся завершения всех потоков
    for (auto& thread : threads) {
        thread.join();
    }

    // Закрываем сокет сервера
    close(serverSocket);

    return 0;
}
