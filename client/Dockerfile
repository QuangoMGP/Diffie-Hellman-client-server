# Используем базовый образ с необходимыми зависимостями
FROM arm64v8/ubuntu:latest

# EXPOSE 12345

# Копируем файлы приложения в контейнер
COPY . /app

# Устанавливаем зависимости и выполняем необходимые команды
RUN apt-get update && \
    apt-get install -y build-essential g++ libgtk-3-dev

# Компилируем сервер
RUN g++ ./app/clientDH.cpp -o ./app/clientDH `pkg-config --cflags --libs gtk+-3.0`

# Очищаем кеш apt
RUN apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Указываем команду, которая будет запущена при старте контейнера
CMD ["./app/clientDH"]