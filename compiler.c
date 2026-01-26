#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

// Variable types
typedef enum { VAR_INT, VAR_STRING, VAR_BOOL, VAR_FLOAT } VarType;

// Dynamic variable structure
typedef struct {
    char *name;
    VarType type;
    int ivalue;
    char *svalue;
    bool bvalue;
    float fvalue;
} Variable;

// Global state for dynamic storage
Variable *vars = NULL;
size_t var_count = 0;
size_t var_capacity = 0;

// Function to skip all types of whitespace
char* skip_whitespace(char *str) {
    while (*str && isspace((unsigned char)*str)) str++;
    return str;
}

// Memory management: find or create a variable
Variable* get_or_create_var(const char *name) {
    for (size_t i = 0; i < var_count; i++) {
        if (strcmp(vars[i].name, name) == 0) return &vars[i];
    }

    // Expand capacity if needed (Dynamic Array)
    if (var_count >= var_capacity) {
        var_capacity = (var_capacity == 0) ? 10 : var_capacity * 2;
        vars = realloc(vars, sizeof(Variable) * var_capacity);
    }

    vars[var_count].name = strdup(name);
    vars[var_count].svalue = NULL;
    vars[var_count].type = VAR_INT; // default
    return &vars[var_count++];
}

// Clean up memory at the end
void free_all() {
    for (size_t i = 0; i < var_count; i++) {
        free(vars[i].name);
        if (vars[i].svalue) free(vars[i].svalue);
    }
    free(vars);
}

// Correctly remove comments without breaking strings
void remove_comments(char *buffer) {
    bool in_string = false;
    for (char *p = buffer; *p; p++) {
        if (*p == '"' && (p == buffer || *(p - 1) != '\\')) in_string = !in_string;
        if (!in_string && *p == '/' && *(p + 1) == '/') {
            while (*p && *p != '\n') *p++ = ' ';
        }
    }
}

// Advanced parser: finds the next ';' but respects double quotes
char* get_next_statement(char **cursor) {
    char *start = *cursor;
    char *p = start;
    bool in_string = false;

    while (*p) {
        if (*p == '"' && (p == start || *(p - 1) != '\\')) in_string = !in_string;
        if (*p == ';' && !in_string) {
            *p = '\0';
            *cursor = p + 1;
            return start;
        }
        p++;
    }
    *cursor = p;
    return (p == start) ? NULL : start;
}

void execute_statement(char *stmt) {
    stmt = skip_whitespace(stmt);
    if (!*stmt) return;

    // --- Handling PRINT ---
    if (strncmp(stmt, "print", 5) == 0) {
        char *open_paren = strchr(stmt, '(');
        char *close_paren = strrchr(stmt, ')');
        if (open_paren && close_paren && close_paren > open_paren) {
            *close_paren = '\0';
            char *arg = skip_whitespace(open_paren + 1);
            
            // Trim trailing spaces in argument
            char *end = arg + strlen(arg) - 1;
            while (end > arg && isspace((unsigned char)*end)) *end-- = '\0';

            if (*arg == '"') { // String literal
                char *end_q = strrchr(arg + 1, '"');
                if (end_q) *end_q = '\0';
                printf("%s\n", arg + 1);
            } else { // Variable lookup
                bool found = false;
                for (size_t i = 0; i < var_count; i++) {
                    if (strcmp(vars[i].name, arg) == 0) {
                        if (vars[i].type == VAR_INT) printf("%d\n", vars[i].ivalue);
                        else if (vars[i].type == VAR_STRING) printf("%s\n", vars[i].svalue);
                        else if (vars[i].type == VAR_BOOL) printf("%s\n", vars[i].bvalue ? "true" : "false");
                        else if (vars[i].type == VAR_FLOAT) printf("%f\n", vars[i].fvalue);
                        found = true;
                        break;
                    }
                }
                if (!found) fprintf(stderr, "Runtime Error: Variable '%s' not found\n", arg);
            }
        }
        return;
    }

    // --- Handling Declarations (int, string, bool) ---
    VarType type;
    char *name_start = NULL;

    if (strncmp(stmt, "int ", 4) == 0) { type = VAR_INT; name_start = stmt + 4; }
    else if (strncmp(stmt, "string ", 7) == 0) { type = VAR_STRING; name_start = stmt + 7; }
    else if (strncmp(stmt, "bool ", 5) == 0) { type = VAR_BOOL; name_start = stmt + 5; }
    else if (strncmp(stmt, "float ", 6) == 0) { type = VAR_FLOAT; name_start = stmt + 6; }
    else return;

    char *eq = strchr(name_start, '=');
    if (!eq) return;

    *eq = '\0';
    char *name = skip_whitespace(name_start);
    // Trim name end
    char *n_end = name + strlen(name) - 1;
    while (n_end > name && isspace((unsigned char)*n_end)) *n_end-- = '\0';

    if (isdigit(name[0])) {
        fprintf(stderr, "Syntax Error: Var name '%s' cannot start with digit\n", name);
        return;
    }

    char *val_str = skip_whitespace(eq + 1);
    Variable *v = get_or_create_var(name);
    v->type = type;

    if (type == VAR_INT) {
        v->ivalue = atoi(val_str);
    } else if (type == VAR_STRING) {
        if (*val_str == '"') {
            char *end_q = strrchr(val_str + 1, '"');
            if (end_q) *end_q = '\0';
            if (v->svalue) free(v->svalue);
            v->svalue = strdup(val_str + 1);
        }
    } else if (type == VAR_BOOL) {
        if (strncmp(val_str, "true", 4) == 0) v->bvalue = true;
        else if (strncmp(val_str, "false", 5) == 0) v->bvalue = false;
        else fprintf(stderr, "Warning: Invalid bool value '%s'\n", val_str);
    } else if (type == VAR_FLOAT) {
        v->fvalue = (float)atof(val_str);
    }
}

int main(void) {
    FILE *file = fopen("test.air", "r");
    if (!file) {
        perror("Error opening file");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    char *buffer = malloc(size + 1);
    fread(buffer, 1, size, file);
    buffer[size] = '\0';
    fclose(file);

    remove_comments(buffer);

    char *cursor = buffer;
    char *stmt;
    while ((stmt = get_next_statement(&cursor)) != NULL) {
        execute_statement(stmt);
    }

    free(buffer);
    free_all();
    return 0;
}