#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

int main() {
    int fd;
    char buffer[256];
    
    // Открываем файл в неблокирующем режиме
    fd = open("/tmp/text", O_RDONLY | O_NONBLOCK);
    if (fd == -1) {
        perror("Ошибка открытия файла");
        return 1;
    }
    
    printf("Программа запущена. Ожидание данных в /tmp/text...\n");
    printf("Выполните: echo 'Ваш текст' > /tmp/text\n");
    printf("Для выхода нажмите Ctrl+C\n\n");
    
    while (1) {
        ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
        
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            printf("Получено данных: %s", buffer);
            
            // Перемещаем указатель в начало файла
            lseek(fd, 0, SEEK_SET);
        } else if (bytes_read == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Нет данных - ждем немного
                usleep(100000); // 100ms
                continue;
            } else {
                perror("Ошибка чтения");
                break;
            }
        } else if (bytes_read == 0) {
            // Конец файла - перемещаемся в начало
            lseek(fd, 0, SEEK_SET);
            usleep(100000);
        }
    }
    
    close(fd);
    return 0;
}