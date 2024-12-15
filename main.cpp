#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <iostream>
#include <string>
#include <array>
#include <algorithm>

bool pipeRead(int stream, int& value)
{
    int readed = 0;
    readed = read(stream, &value, sizeof(value));
    if (readed == -1)
    {
        perror("Error reading pipe.");
        exit(EXIT_FAILURE);
    }
    else if (readed == 0)
        return false;
    return true;
}

void pipeWrite(int stream, int value)
{
    if (write(stream, &value, sizeof(value)) == -1)
    {
        perror("Error writing pipe.");
        exit(EXIT_FAILURE);
    }
}

std::vector<std::array<int, 2>> createPipes(int count)
{
    std::vector<std::array<int, 2>> pipes;
    for (int i = 0; i < count; ++i)
    {
        std::array<int, 2> newPipe;
        if (pipe(newPipe.data()) == -1)
        {
            perror("Error creating pipe.");
            exit(EXIT_FAILURE);
        }
        pipes.push_back(newPipe);
    }
    return pipes;
}

int main(int argc, char* argv[])
{
    auto pipes = createPipes(2); // Create two pipes

    if (fork() == 0)
    {
        // First child: reads from stdin and writes to the first pipe
        close(pipes[0][0]);
        close(pipes[1][0]);
        close(pipes[1][1]);

        int value;
        do
        {
            std::cin >> value;
            if (value > 0)
                pipeWrite(pipes[0][1], value);
        } while (value > 0);

        close(pipes[0][1]);
        exit(EXIT_SUCCESS);
    }
    else if (fork() == 0)
    {
        // Second child: mediates between the two pipes and calculates the average
        close(pipes[0][1]);
        close(pipes[1][0]);

        int value, sum = 0, count = 0;
        while (pipeRead(pipes[0][0], value))
        {
            sum += value;
            count++;
            pipeWrite(pipes[1][1], value);
        }

        close(pipes[0][0]);
        close(pipes[1][1]);

        if (count > 0)
        {
            double average = static_cast<double>(sum) / count;
            std::cout << "(Mediator:" << getpid() << "): Average = " << average << std::endl;
        }
        else
        {
            std::cout << "(Mediator:" << getpid() << "): No numbers received." << std::endl;
        }

        exit(EXIT_SUCCESS);
    }
    else
    {
        // Parent: reads from the second pipe and prints the values
        close(pipes[0][0]);
        close(pipes[0][1]);
        close(pipes[1][1]);

        int value;
        while (pipeRead(pipes[1][0], value))
        {
            std::cout << "(Parent:" << getpid() << "): " << value << std::endl;
        }

        close(pipes[1][0]);

        // Wait for both child processes to finish
        wait(nullptr);
        wait(nullptr);
    }

    return 0;
}
