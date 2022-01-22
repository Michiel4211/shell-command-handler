#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <string.h>
#define STDIN 0
#define STDOUT 1
#define PIPE_RD 0
#define PIPE_WR 1
int handler_int = 1;
void handler(int foo)
{
    handler_int = foo;
}
void init(void)
{
    if (prompt)
        prompt = "MichielShell$ ";
}

void handle_detch(part_t *part)
{
    pid_t pid = fork();
    if (pid == 0)
    {
        run_command(part->detach.child);
        exit(EXIT_FAILURE);
    }
    else if (pid == -1)
        perror("Forking failed");
    if (part->type == part_sbshl)
    {
        waitpid(pid, NULL, 0);
    }
}
void handle_cmnd(part_t *part)
{
    char *program = part->command.program;
    char **argv = part->command.argv;
    // Built-in exit
    if (strcmp(program, "exit") == 0)
    {
        if (part->command.argc < 2)
        {
            perror("I'm missing an error code");
            return;
        }
        exit(atoi(argv[1]));
    }
    // Built-in cd
    if (strcmp(program, "cd") == 0)
    {
        if (part->command.argc < 2)
        {
            perror("I'm missing a path");
            return;
        }
        if (chdir(argv[1]) < 0)
        {
            perror("Changing the path failed!");
        }
        return;
    }

    if (strcmp(program, "set") == 0)
    {
        char * left;
        char * right;
        left = strtok(argv[1], "=");
        right = strtok(NULL, "=");
        setenv(left, right, 1);
        return;
    }

    if (strcmp(program, "unset") == 0)
    {
        unsetenv(argv[1]);
        return;
    }
    pid_t pid = fork();
    if (pid == 0)
    {
        if (execvp(program, argv) < 0)
        {
            perror("Fail when running program");
        }
    }
    else if (pid == -1)
        perror("Forking failed");
    waitpid(pid, NULL, 0);
}
void handle_sqnc(part_t *part)
{
    run_command(part->sqnc.first);
    run_command(part->sqnc.second);
}
void handle_pipe(part_t *part)
{
    int fd[part->pipe.n_pieces - 1][2];
    pid_t pid[part->pipe.n_pieces];
    for (int i = 0; i < (int)part->pipe.n_pieces; i++)
    {
        pipe(fd[i]);
    }
    for (int i = 0; i < (int)part->pipe.n_pieces; i++)
    {
        char **argv = part->pipe.piece[i]->command.argv;
        pid[i] = fork();
        if (pid[i] < 0)
        {
            perror("Fork failed");
            return;
        }
        // First pipe
        if (i == 0)
        {
            if (pid[i] == 0)
            {
                dup2(fd[i][PIPE_WR], STDOUT);
                for (int j = 0; j < (int)part->pipe.n_pieces; j++)
                {
                    close(fd[j][PIPE_RD]);
                    close(fd[j][PIPE_WR]);
                }
                execvp(argv[0], argv);
            }
        }
        // Last pipe
        else if (i == (int)part->pipe.n_pieces - 1)
        {
            if (pid[i] == 0)
            {
                dup2(fd[i - 1][PIPE_RD], STDIN);
                for (int j = 0; j < (int)part->pipe.n_pieces; j++)
                {
                    close(fd[j][PIPE_RD]);
                    close(fd[j][PIPE_WR]);
                }
                execvp(argv[0], argv);
            }
        }
        // Middle parts
        else
        {
            if (pid[i] == 0)
            {
                dup2(fd[i - 1][PIPE_RD], STDIN);
                dup2(fd[i][PIPE_WR], STDOUT);
                for (int j = 0; j < (int)part->pipe.n_pieces; j++)
                {
                    close(fd[j][PIPE_RD]);
                    close(fd[j][PIPE_WR]);
                }
                execvp(argv[0], argv);
            }
        }
    }
    for (int i = 0; i < (int)part->pipe.n_pieces; i++)
    {
        close(fd[i][PIPE_RD]);
        close(fd[i][PIPE_WR]);
    }
    for (int i; i < (int)part->pipe.n_pieces; i++)
    {
        waitpid(pid[i], NULL, 0);
    }
}
void runner(part_t *part)
{
    signal(SIGINT, handler);
    if (part->type == part_dtc)
        handle_dtch(part);
    if (part->type == part_cmd)
        handle_cmnd(part);
    if (part->type == part_sqnc)
        handle_sqnc(part);
    if (part->type == part_pipe)
        handle_pipe(part);
    if (part->type == part_sbshl)
        handle_detch(part);
}
