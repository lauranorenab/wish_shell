#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "wish_utils.h"

extern char *mypath[];
extern char error_message[];
#define MAX_FILENAME 1024

void execute_exit(int value)
{
    exit(0);
}

void execute_cd(char *newpath)
{
    char *path = strtok_r(newpath, " ", &newpath);
    ;
    // Intenta ir a la dirección, si la dirección no es valida, imprime un error.
    if (chdir(path) != 0)
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
        return;
    }
    chdir(path);
}

void execute_path(char *newpath)
{
    char *tokens[4];
    char *token;
    memset(tokens, 0, sizeof(tokens));
    int c= 0;
    token = strtok(newpath, " ");
    while (token != NULL)
    {
        tokens[c] = token;
        token = strtok(NULL, " ");
        c++;
    }
    for (int j = 0; j < c; j++)
    {
        strcpy(newpath, tokens[j]);
        int fd = -1;
        char **mp = mypath;
        int i = 0;

        while ((strcmp(*mp, "") != 0) && fd != 0)
        {
            mp++;
            i++;
        }
        // Si el objeto "newpath" no existe en la variable "mypath", se inserta
        mypath[i] = malloc(strlen(newpath) + 1);
        strcpy(mypath[i], newpath);
                // Valida que el formato del new path sea correcto (Debe terminar con "/")
        if (strchr(mypath[i], '/') == NULL)
        {
            // Si el formato del new path es incorrecto, añade un "/" al final
            mypath[i] = strcat(mypath[i], "/");
        }
        mypath[i + 1] = "";
    }
}