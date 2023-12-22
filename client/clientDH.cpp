#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h> 
#include <sstream>
#include <gtk/gtk.h>

using namespace std;

class Client {
    private:
        pid_t pid;
        long long int a;
        long long int g;
        long long int p;
        long long int pubK;
        long long int privK;
        long long int B;
        int clientSocket;
        long long int endrand = 1000000000; //length of params
        sockaddr_in serverAddress;
        int id;

        // Метод для создания случайного числа
        long long int irand(long long int a, long long int b){
            return a + time(NULL)*rand()%(b-a+1);
        }
        long long int generate(){
            return irand(1, endrand);
        }
        void calcPubKey(){
            pubK = modPow(g, a, p);
        }

        long long int modPow(long long int base, long long int exponent, long long int& modulus) {
            if (modulus == 1) return 0; // Деление на 0 не допускается
            long long int result = 1;
            base = base % modulus;
            while (exponent > 0) {
                // Если exponent нечетный, умножаем результат на base и берем по модулю
                if (exponent % 2 == 1)
                    result = (result * base) % modulus;
                // Теперь exponent четный, делим его пополам и берем base^2 по модулю
                exponent = exponent >> 1;
                base = (base * base) % modulus;
            }
            return result;
        }

        int sendLine(string message){
            const char* sendtext = message.c_str();
            int bytesSent = send(clientSocket, sendtext, message.length(), 0);

            // Проверяем успешность отправки
            if (bytesSent == -1) {
                cerr << "Ошибка при отправке сообщения\n";
                return -1;
            }
            return 0;
        }
        string reciveLine(){
            char buffer[1024];
            int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            
            // Проверяем успешность получения
            if (bytesRead <= 0) {
                cerr << "Ошибка при получении сообщения от сервера\n";
                return "";
            }
            buffer[bytesRead] = '\0'; // Устанавливаем нуль-терминатор
            return string(buffer);
        }


    public:
        int connectTo(const char* ip, int port){
            clientSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (clientSocket == -1) {
                cerr << "Ошибка при создании сокета\n";
                return -1;
            }

            // Настраиваем серверный адрес
            serverAddress.sin_family = AF_INET;
            serverAddress.sin_port = htons(port); // Порт сервера

            // Преобразуем IP-адрес или доменное имя в структуру sockaddr_in
            if (inet_pton(AF_INET, ip, &serverAddress.sin_addr) <= 0) {
                cerr << "Ошибка при преобразовании адреса\n";
                close(clientSocket);
                return -1;
            }
            if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
                cerr << "Ошибка при подключении к серверу\n";
                close(clientSocket);
                return -1;
            }
            return 0;
        }


        int disconnect(){
            if (clientSocket != -1) {
                if (close(clientSocket) == 0) {
                    cout << "Сокет успешно закрыт\n";
                } else {
                    cerr << "Ошибка при закрытии сокета\n";
                }
            }
            return 0;
        }

        int setID(int num){
            id = num;
            return 0;
        }

        int init(){
            a = generate();
            sendLine(to_string(id));
            cout << "ID " << id << " отправлен, ожидание параметров\n";
            string params = reciveLine();
            if (params == ""){
                return -1;
            }
            istringstream iss(params);
            iss >> g >> p;
            cout << "Стартовые параметры получены: " << params << '\n';
            calcPubKey();
            cout << "Отправка публичного ключа\n";
            if (sendLine(to_string(pubK)) == -1){
                return -1;
            };
            if (iss >> B) {}
            else {
                // получение второго ключа
                cout << "Ожидание второго публичного ключа\n";
                string r = reciveLine();
                if (r == ""){
                    return -1;
                }
                B = stoll(r);
            }
            cout << "Генерация приватного ключа\n";
            calcPrivKey(B);
            return 0;
        }

        pid_t getPid(){
            pid = getpid();
            return pid;
        }
        long long int getPubKey(){
            return pubK;
        }
        void calcPrivKey(long long int& exKey){
            B = exKey;
            privK = modPow(B, a, p);
        }
        long long int getPrivKey(){
            return privK;
        }
        string print() {
            stringstream result;
            result << "a: " << a << "\ng: " << g << "\np: " << p << "\nB: " << B << "\npubKey: " << pubK << "\nprivKey: " << privK << '\n';

            // Output to cout
            cout << result.str();

            return result.str();
        }
};

Client client;    
// Функция обработки события нажатия кнопки
// Функция обработки события нажатия кнопки
static void setID(GtkWidget *widget, gpointer data) {
    // Получаем текст из поля ввода
    const gchar *id = gtk_entry_get_text(GTK_ENTRY(data));
    
    try {
        if (client.setID(stoi(id)) == 0) {
            gchar *message = g_strdup_printf("Done!");
            // Обновление текста в GtkLabel
            GtkWidget *label = GTK_WIDGET(g_object_get_data(G_OBJECT(widget), "setID"));
            gtk_label_set_text(GTK_LABEL(label), message);
            // Освобождение памяти, выделенной для сообщения
            g_free(message);
        } else {
            // Обработка ошибки, если не удалось установить ID
            gchar *message = g_strdup_printf("Error!");
            // Обновление текста в GtkLabel
            GtkWidget *label = GTK_WIDGET(g_object_get_data(G_OBJECT(widget), "setID"));
            gtk_label_set_text(GTK_LABEL(label), message);
            // Освобождение памяти, выделенной для сообщения
            g_free(message);
        }
    } catch (const invalid_argument& e) {
        // Обработка ошибки, если введено некорректное значение
        gchar *message = g_strdup_printf("Invalid argument!");
        // Обновление текста в GtkLabel
        GtkWidget *label = GTK_WIDGET(g_object_get_data(G_OBJECT(widget), "setID"));
        gtk_label_set_text(GTK_LABEL(label), message);
        // Освобождение памяти, выделенной для сообщения
        g_free(message);
    } catch (const out_of_range& e) {
        // Обработка ошибки, если введено слишком большое значение
        gchar *message = g_strdup_printf("Out of range!");
        // Обновление текста в GtkLabel
        GtkWidget *label = GTK_WIDGET(g_object_get_data(G_OBJECT(widget), "setID"));
        gtk_label_set_text(GTK_LABEL(label), message);
        // Освобождение памяти, выделенной для сообщения
        g_free(message);
    }
}

// Функция обработки события нажатия кнопки
static void connectS(GtkWidget *widget, gpointer data) {
    // Получаем текст из поля ввода
    const gchar *addr = gtk_entry_get_text(GTK_ENTRY(data));

    // Выводим текст в терминал
    istringstream iss(addr);
    string ip;
    string port;

    if (getline(iss, ip, ':') && getline(iss, port)) {
        try {
            if (client.connectTo(ip.c_str(), stoi(port)) != -1) {
                gchar *message = g_strdup_printf("Connected!");
                // Обновление текста в GtkLabel
                GtkWidget *label = GTK_WIDGET(g_object_get_data(G_OBJECT(widget), "connect"));
                gtk_label_set_text(GTK_LABEL(label), message);
                // Освобождение памяти, выделенной для сообщения
                g_free(message);
            } else {
                gchar *message = g_strdup_printf("Error!");
                // Обновление текста в GtkLabel
                GtkWidget *label = GTK_WIDGET(g_object_get_data(G_OBJECT(widget), "connect"));
                gtk_label_set_text(GTK_LABEL(label), message);
                // Освобождение памяти, выделенной для сообщения
                g_free(message);
            }
        } catch (const invalid_argument& e) {
            // Обработка ошибки, если введено некорректное значение
            gchar *message = g_strdup_printf("Invalid argument!");
            // Обновление текста в GtkLabel
            GtkWidget *label = GTK_WIDGET(g_object_get_data(G_OBJECT(widget), "connect"));
            gtk_label_set_text(GTK_LABEL(label), message);
            // Освобождение памяти, выделенной для сообщения
            g_free(message);
        } catch (const out_of_range& e) {
            // Обработка ошибки, если введено слишком большое значение
            gchar *message = g_strdup_printf("Out of range!");
            // Обновление текста в GtkLabel
            GtkWidget *label = GTK_WIDGET(g_object_get_data(G_OBJECT(widget), "connect"));
            gtk_label_set_text(GTK_LABEL(label), message);
            // Освобождение памяти, выделенной для сообщения
            g_free(message);
        }
    } else {
        // Обработка ошибки, если введенный адрес не соответствует ожидаемому формату
        gchar *message = g_strdup_printf("Invalid input format!");
        // Обновление текста в GtkLabel
        GtkWidget *label = GTK_WIDGET(g_object_get_data(G_OBJECT(widget), "connect"));
        gtk_label_set_text(GTK_LABEL(label), message);
        // Освобождение памяти, выделенной для сообщения
        g_free(message);
    }
}

static void initF(GtkWidget *widget, gpointer data) {
    // Assuming data is a GtkLabel* where you want to display the text
    GtkLabel *initLabel = GTK_LABEL(data);

    // Call the function to generate text from your client class
    string text;

    if (client.init() == 0) {
        text = client.print();
    } else {
        text = "Initialization failed!";
    }  // Corrected this line to call client.print()

    // Convert the C++ string to gchar* for GTK functions
    gchar *initmessage = g_strdup(text.c_str());

    // Set the text of the label
    gtk_label_set_text(initLabel, initmessage);

    // Free allocated memory
    g_free(initmessage);
}

static void disconnectS(GtkWidget *widget, gpointer data) {
    // Call the disconnect function of your client
    client.disconnect();
}


void signalHandler(int signum){
    if (signum == SIGINT) {
        client.disconnect();
        cout << "Exiting...\n";
        exit(signum);
    }
}

int main(int argc, char *argv[]) {
    signal(SIGINT, signalHandler);
    long long int g;
    long long int p;
    int id;

    // Инициализация GTK
    gtk_init(&argc, &argv);

    // Создание главного окна
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "ClientDH");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 150);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);


    // Создание кнопки
    GtkWidget *setIDButton = gtk_button_new_with_label("Confirm");
    GtkWidget *connectButton = gtk_button_new_with_label("Connect");
    GtkWidget *initButton = gtk_button_new_with_label("INIT");
    GtkWidget *disconnectButton = gtk_button_new_with_label("Disconnect");


    // Создание поля для ввода текста
    GtkWidget *setIDEntry = gtk_entry_new();
    GtkWidget *connectEntry = gtk_entry_new();
    // Установка подсказки
    gtk_entry_set_placeholder_text(GTK_ENTRY(setIDEntry), "Enter ID Here");  
    gtk_entry_set_placeholder_text(GTK_ENTRY(connectEntry), "Enter server IP Here");  


    // Создание метки для сообщений
    GtkWidget *setIDLabel = gtk_label_new(NULL);
    GtkWidget *connectLabel = gtk_label_new(NULL);
    GtkWidget *initLabel = gtk_label_new(NULL);

    // Подключение обработчика событий для кнопки
    g_signal_connect(setIDButton, "clicked", G_CALLBACK(setID), setIDEntry);
    g_signal_connect(connectButton, "clicked", G_CALLBACK(connectS), connectEntry);
    g_signal_connect(initButton, "clicked", G_CALLBACK(initF), initLabel);
    g_signal_connect(disconnectButton, "clicked", G_CALLBACK(disconnectS), NULL);

    // Размещение элементов в окне
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_attach(GTK_GRID(grid), setIDEntry, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), setIDButton, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), setIDLabel, 2, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), connectEntry, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), connectButton, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), connectLabel, 2, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), initButton, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), initLabel, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), disconnectButton, 1, 2, 1, 1);

    // Установка метки в пользовательские данные кнопки
    g_object_set_data(G_OBJECT(setIDButton), "setID", setIDLabel);
    g_object_set_data(G_OBJECT(connectButton), "connect", connectLabel);

    // Размещение сетки в окне
    gtk_container_add(GTK_CONTAINER(window), grid);

    // Отображение всех виджетов
    gtk_widget_show_all(window);

    // Запуск цикла обработки событий
    gtk_main();

    return 0;
}




// int main() {
//     long long int g;
//     long long int p;
//     int id;
//     Client client;
//     cout << "Введите id для инициалицации соединения:\n";
//     cin >> id;
//     client.setID(id);
//     client.connectTo("127.0.0.1", 12345);
//     client.init();
//     client.print();
//     client.disconnect();
// }