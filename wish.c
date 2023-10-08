#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <ctype.h>
#include "wish_utils.h"

#define MAX_HISTORY 10
#define MAX_LINE_LENGTH 256

void set_terminal_raw_mode(struct termios *old_term, struct termios *new_term)
{
    // Save the current terminal settings
    tcgetattr(STDIN_FILENO, old_term);
    *new_term = *old_term;

    // Set the terminal to raw mode
    new_term->c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, new_term);
}
void restore_terminal_mode(struct termios *old_term)
{
    // Restore the terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, old_term);
}
void navigate_command_history(char *history[], char *line, int *history_count, int *history_idx, int *line_idx, char *ch)
{
    *ch = getchar(); // '[' character
    *ch = getchar(); // Arrow key code

    if (*ch == 'A')
    { // Up arrow key
        if (*history_count > 0 && *history_idx > 0)
        {
            (*history_idx)--;
            memset(line, 0, MAX_LINE_LENGTH);
            strncpy(line, history[*history_idx], MAX_LINE_LENGTH - 1);
            *line_idx = strlen(line);
            printf("\033[2K\r%s", line);
            fflush(stdout);
        }
    }
    else if (*ch == 'B')
    { // Down arrow key
        if (*history_count > 0 && *history_idx < *history_count - 1)
        {
            (*history_idx)++;
            memset(line, 0, MAX_LINE_LENGTH);
            strncpy(line, history[*history_idx], MAX_LINE_LENGTH - 1);
            *line_idx = strlen(line);
            printf("\033[2K\r%s", line);
            fflush(stdout);
        }
    }
}
void handle_backspace_key(char *line, int *line_idx)
{
    if (*line_idx > 0)
    {
        putchar('\b');
        putchar(' ');
        putchar('\b');
        (*line_idx)--;
        line[*line_idx] = 0;
    }
}
void add_command_to_history(char *history[], char *line, int *line_idx, int *history_count, int *history_idx)
{
    putchar('\n');

    if (*line_idx > 0)
    {
        // Copy the current line to the history
        if (*history_count == MAX_HISTORY)
        {
            free(history[0]);
            memmove(history, history + 1, (MAX_HISTORY - 1) * sizeof(char *));
            (*history_count)--;
        }
        history[*history_count] = malloc((*line_idx + 1) * sizeof(char));
        strncpy(history[*history_count], line, *line_idx);
        history[*history_count][*line_idx] = '\0';
        (*history_count)++;
        *history_idx = *history_count;
    }

    *line_idx = 0;
}
void handle_input(char *line, int *line_idx, char *ch)
{
    if (*line_idx < MAX_LINE_LENGTH - 1)
    {
        putchar(*ch);
        line[*line_idx] = *ch;
        (*line_idx)++;
    }
}
char *trimString(char *str)
{
    int start = 0, end = strlen(str) - 1;
    while (isspace(str[start]))
    {
        start++;
    }
    while ((end >= start) && isspace(str[end]))
    {
        end--;
    }
    str[end + 1] = '\0';
    return &str[start];
}

char *mypath[] = {"/bin/", "", NULL, NULL, NULL, NULL};
char error_message[30] = "An error has occurred\n";

int closedRedirection = 0; // flag para devolver el control al usuario y que pueda seguir interactuando con la terminal
int stdout_copy = 0;       // Varaible usada para la salida del comando a un archivo

// Función que nos permite imprimir el error
void printError()
{
    write(STDERR_FILENO, error_message, strlen(error_message));
    exit(0);
}

// Función para la redirección
void redirection(char *line)
{
    int a = 0;
    char *redirections[sizeof(char) * 512];
    redirections[0] = strtok(strdup(line), " \n\t>");
    while (redirections[a] != NULL)
    {
        a++;
        redirections[a] = strtok(NULL, " \n\t>");
    }
    if (a == 1)
    {
        printError();
        exit(0);
    }
    int i = 0;
    char *arguments[sizeof(line)];
    arguments[0] = strtok(line, "\n\t>");
    while (arguments[i] != NULL)
    {
        i++;
        arguments[i] = strtok(NULL, " \n\t>");
    }
    if (i > 2)
    {
        printError();
        exit(0);
    }
    int x = 0;
    char *command[sizeof(arguments[1])];
    command[0] = strtok(arguments[1], " \n\t");
    while (command[x] != NULL)
    {
        x++;
        command[x] = strtok(NULL, " \n\t");
    }
    char *command_out = strdup(command[0]);
    stdout_copy = dup(1);
    int file_out = open(command_out, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    int file_error = open(command_out, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    fflush(stdout);
    dup2(file_out, STDOUT_FILENO);
    dup2(file_error, STDERR_FILENO);
    close(file_out);
    close(file_error);
    closedRedirection = 1;
    if (file_out == -1 || file_error == -1 || x > 1 || i > 2)
    {
        printError();
        exit(0);
    }
    arguments[i + 1] = NULL;
    command[x + 1] = NULL;
    strcpy(line, arguments[0]);
}
// void executeConcurrencyCommands(char *line)
// {

//     int processArray[j];
//     for (int i = 0; i < j; i++)
//     {
//         // ejecutar comando y guardar el id del proceso
//         int pid = executeCommand(commands[i]);
//         processArray[i] = pid;
//         for (int x = 0; x < j; x++)
//         {
//             int processStatus = 0;
//             // suspende la ejecucion actual hasta que todos los hijos hayan terminado
//             waitpid(processArray[x], &processStatus, 0);
//             if (processStatus == 1)
//             {
//                 printError();
//             }
//         }
//     }
// }

int main(int argc, char *argv[])
{
    if (argc > 2)
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }
    char *command_string;
    char *command_args;
    int fd;

    char *line = (char *)malloc(sizeof(char) * MAX_LINE_LENGTH);

    // Atributos para el command history
    char ch;
    char *history[MAX_HISTORY] = {NULL};
    int history_idx = 0;
    int line_idx = 0;
    int history_count = 0;
    struct termios old_term, new_term;

    //  Atributos  para el batch mode
    FILE *fp;
    size_t len = 0;
    ssize_t read;

    int newline_pos;

    char *tokens[4];
    char *token;
    int i;
    if (argc == 2) // Si hay 2 argumentos, significa que se entro un archivo para el batchmode
    {
        fp = fopen(argv[1], "r"); // Se intenta abrir el archivo que se entro como argumento
        if (fp == NULL)
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }
    }
    // Si no está en batch mode, imprime wish>
    else if (argc == 1)
    {
        set_terminal_raw_mode(&old_term, &new_term);
        printf("wish> ");
    }
    // Unending loop to read input
    while (1)
    {
        if (argc == 1) // Se ejecuta esta linea en modo interactivo (no se ingreso un argumento para el batchmode)
        {
            ch = getchar();

            if (ch == '\033')
            { // Escape sequence
                navigate_command_history(history, line, &history_count, &history_idx, &line_idx, &ch);
            }
            else if (ch == '\n')
            { // Enter key
                add_command_to_history(history, line, &line_idx, &history_count, &history_idx);
                fflush(stdout);
            }
            else if (ch == 127)
            { // Backspace key
                handle_backspace_key(line, &line_idx);
            }
            else if (ch >= 32 && ch <= 126)
            { // Printable character
                handle_input(line, &line_idx, &ch);
            }
        }
        else if (argc == 2) // Si hay 2 argumentos, significa que se entro un archivo para el batchmode
        {
            read = getline(&line, &len, fp); // Se lee cada linea del archivo batch
            if (read == EOF)
            {
                break;
            }
        }
        // Modo de ejecución de comando (line). Se entra cuando se presiona Enter o cuando es Batchmode (argc==2)
        if (argc == 2 || ch == '\n')
        {
            // Si no está en batch mode, imprime "wish> "
            if (argc == 1)
            {
                printf("wish> ");
            }

            // Reemplaza el último caracter de la línea por null (\0)
            newline_pos = strcspn(line, "\n");
            line[newline_pos] = '\0';
            // inicializar array separando los comandos por el token &

            // Creamos un array para almacenar los tokens
            memset(tokens, 0, sizeof(tokens));
            i = 0;

            // Utilizamos strtok para dividir la cadena en tokens
            token = strtok(line, "&");

            while (token != NULL)
            {
                tokens[i++] = trimString(token);
                token = strtok(NULL, "&");
            }

            // Imprimimos cada token
            for (int j = 0; j < i; j++)
            {

                if (strstr(line, ">") != NULL)
                {
                    redirection(line);
                }
                // Se separa el comando del argumento
                command_args = trimString(tokens[j]);
                command_string = strtok_r(command_args, " ", &command_args);
                if (command_string == NULL) // Si no se entra ningún comando, empieza el loop de nuevo
                {
                    continue;
                }
                else if (command_args[0] == 0 || command_args[0] == 32) // Si no se entra ningún argumento al comando, se coloca argumento "."
                {
                    command_args = malloc(strlen(".") + 1);
                    strcpy(command_args, ".");
                }
                if (strcmp(command_string, "exit") == 0)
                {
                    if (strcmp(command_args, ".") != 0)
                    {
                        write(STDERR_FILENO, error_message, strlen(error_message));
                        continue;
                    }
                    execute_exit(0);
                }
                else if (strcmp(command_string, "cd") == 0)
                {
                    // Al no tener argumentos, se coloca el argumento "." y se imprime un error
                    if (strcmp(command_args, ".") == 0)
                    {
                        write(STDERR_FILENO, error_message, strlen(error_message));
                        continue;
                    }
                    execute_cd(command_args);
                }
                else if (strcmp(command_string, "path") == 0)
                {
                    command_args = trimString(command_args);
                    execute_path(command_args);
                }
                else
                {
                    fd = -1;
                    char **mp = mypath;
                    char specificpath[MAX_LINE_LENGTH];
                    while ((strcmp(*mp, "") != 0) && fd != 0)
                    {
                        strcpy(specificpath, *mp++);
                        strncat(specificpath, command_string, strlen(command_string));
                        fd = access(specificpath, X_OK);
                    }
                    if (fd == 0)
                    {
                        int subprocess = fork();
                        if (subprocess < 0)
                        {
                            printf("Error launching the subprocess");
                        }
                        else if (subprocess == 0)
                        {
                            char *myargs[3];
                            myargs[0] = strdup(specificpath);
                            myargs[1] = strdup(command_args);
                            myargs[2] = NULL;
                            // Si el comando es /bin/ls, se ejecuta "ls"
                            if (strcmp(myargs[0], "/bin/ls") == 0)
                            {
                                myargs[0] = "ls";
                            }
                            execvp(myargs[0], myargs);
                        }
                        else
                        {
                            wait(NULL);
                        }
                    }
                    else
                    {
                        write(STDERR_FILENO, error_message, strlen(error_message));
                    }
                }
            }
            // Si no está en batch mode, limpia el anterior comando del input
            if (argc == 1)
            {
                memset(line, 0, MAX_LINE_LENGTH);
            }
        }
        if (closedRedirection == 1)
        {
            dup2(stdout_copy, 1);
            close(stdout_copy);
            stdout_copy = 0;
            closedRedirection = 0;
        }
    }
    if (argc == 1)
    {
        restore_terminal_mode(&old_term);
    }
    free(command_args);
    free(line);
    fclose(fp);
    return 0;
}