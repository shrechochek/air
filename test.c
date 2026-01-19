#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


bool is_print(char *token) {
    char token_cut[7];

    strncpy(token_cut, token, 6);
    token_cut[6] = '\0';

    bool result = strcmp(token_cut,"print(");
    
    return !result;
}

char* skip_whitespace(char *str) {
    while (*str == ' ' || *str == '\n' || *str == '\t' || *str == '\r') {
        str++;
    }
    return str;
}

int skip_to_bracket(char *str) {
    int index = 0;
    while (*str != ')') { //add length check
        index ++;
        str ++;
    }
    return index;
}

int main(void) {
    FILE *file = fopen("test.air", "r");
    if (!file) {
        perror("Не удалось открыть файл");
        return 1;
    }

    // Определяем размер файла
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    // Читаем файл в строку
    char *buffer = malloc(size + 1);
    if (!buffer) {
        fclose(file);
        return 1;
    }

    fread(buffer, 1, size, file);
    buffer[size] = '\0';
    fclose(file);

    // Разбиение по ;
    char *token = strtok(buffer, ";");
    int index = 0;

    
    // printf("%c\n", token[0]);
    while (token != NULL) {
        token = skip_whitespace(token); // clean all spaces and /n
        if(is_print(token)) {
            char *print_token = token + 6;
            index = skip_to_bracket(token);
            token[index] = '\0';
            printf("%s\n", print_token);
        } else {
            // printf("NO\n");
        }
        token = strtok(NULL, ";");
    }

    free(buffer);
    return 0;
}
