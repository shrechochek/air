#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>


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

#define MAX_VARS 100
#define VAR_NAME_LEN 63

typedef enum { VAR_INT, VAR_STRING } VarType; //var types

typedef struct {
    char name[VAR_NAME_LEN + 1];
    VarType type;
    int ivalue;
    char svalue[256];
    bool in_use;
} Variable;

static Variable vars[MAX_VARS];

void remove_end(char *str) {
    if (!str || !*str) return;
    char *end = str + strlen(str) - 1;
    while (end >= str && isspace((unsigned char)*end)) {
        *end-- = '\0';
    }
}

int find_var(const char *name) {
    for (int i = 0; i < MAX_VARS; i++) {
        if (strcmp(vars[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

bool set_var_int(const char *name, int value) {
    int idx = find_var(name);
    if (idx >= 0) {
        vars[idx].type = VAR_INT;
        vars[idx].ivalue = value;
        return true;
    }
    for (int i = 0; i < MAX_VARS; i++) {
        if (!vars[i].in_use) {
            vars[i].in_use = true;
            strncpy(vars[i].name, name, VAR_NAME_LEN);
            vars[i].name[VAR_NAME_LEN] = '\0';
            vars[i].type = VAR_INT;
            vars[i].ivalue = value;
            return true;
        }
    }
    return false;
}

bool set_var_string(const char *name, const char *value) {
    int idx = find_var(name);
    if (idx < 0) {
        for (int i = 0; i < MAX_VARS; i++) {
            if (!vars[i].in_use) {
                idx = i;
                vars[i].in_use = true;
                strncpy(vars[i].name, name, VAR_NAME_LEN);
                vars[i].name[VAR_NAME_LEN] = '\0';
                break;
            }
        }
        if (idx < 0) return false;
    }
    vars[idx].type = VAR_STRING;
    strncpy(vars[idx].svalue, value, sizeof(vars[idx].svalue) - 1);
    vars[idx].svalue[sizeof(vars[idx].svalue) - 1] = '\0';
    return true;
}

bool get_var(const char *name, int *out) {
    int idx = find_var(name);
    if (idx < 0) return false;
    *out = vars[idx].ivalue;
    return true;
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

    
    // printf("%c\n", token[0]);
    while (token != NULL) {
        token = skip_whitespace(token); // clean all spaces and /n
        if (is_print(token)) { //print function
            char *print_token = token + 6; // after "print("
            char *closing = strchr(print_token, ')'); //find ( symbol
            if (closing) *closing = '\0';
            print_token = skip_whitespace(print_token);
            remove_end(print_token);

            if (*print_token == '"') { //if string
                // string literal: print without surrounding quotes
                char *end_quote = strrchr(print_token + 1, '"'); //find string end
                if (end_quote) *end_quote = '\0';
                printf("%s\n", print_token + 1);
            } else { //if var
                int idx = find_var(print_token);
                if (idx >= 0) {
                    if (vars[idx].type == VAR_INT) {
                        printf("%d\n", vars[idx].ivalue);
                    } else if (vars[idx].type == VAR_STRING) {
                        printf("%s\n", vars[idx].svalue);
                    }
                } else {
                    fprintf(stderr, "Unknown variable: %s\n", print_token);
                }
            }
        } else if (strncmp(token, "int", 3) == 0 && isspace((unsigned char)token[3])) { // if int var
            char *name_start = skip_whitespace(token + 3);
            char *eq = strchr(name_start, '=');
            if (!eq) {
                token = strtok(NULL, ";");
                continue;
            }
            *eq = '\0';
            remove_end(name_start);
            char *value_start = skip_whitespace(eq + 1);
            int value = atoi(value_start);
            if (!set_var_int(name_start, value)) {
                fprintf(stderr, "Variable storage full\n");
            }
        } else if (strncmp(token, "string", 3) == 0 && isspace((unsigned char)token[6])) { // if string var
            // string myStr = "hello";
            char *name_start = skip_whitespace(token + 6);
            char *eq = strchr(name_start, '=');
            *eq = '\0';
            remove_end(name_start);
            char *value_start = skip_whitespace(eq + 1);

            // обрезаем кавычки
            if (*value_start == '"') {
                char *end_quote = strrchr(value_start + 1, '"');
                if (end_quote) *end_quote = '\0';
                value_start++; // теперь указывает на первый символ внутри кавычек
            }

            set_var_string(name_start, value_start);
        } 
        else {
            // add new event?
        }
        token = strtok(NULL, ";");
    }

    free(buffer);
    return 0;
}
