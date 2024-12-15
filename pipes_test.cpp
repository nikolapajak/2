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
        return false;  // Zakończenie, brak więcej danych
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
    auto pipes = createPipes(2);  // Tworzymy dwa potoki

    pid_t firstChildPid = fork();
    if (firstChildPid == 0)
    {
        // Pierwszy proces potomny - zapisuje liczby do potoku
        close(pipes[0][0]);  // Zamykamy nieużywany koniec potoku
        close(pipes[1][0]);
        close(pipes[1][1]);

        int value;
        do
        {
            std::cin >> value;
            if (value > 0)
                pipeWrite(pipes[0][1], value);  // Zapisujemy do potoku
        } while (value > 0);

        close(pipes[0][1]);  // Zamykamy końcowy potok po zakończeniu zapisywania
        exit(0);
    }
    else
    {
        pid_t secondChildPid = fork();
        if (secondChildPid == 0)
        {
            // Drugi proces potomny - oblicza średnią
            close(pipes[0][1]);  // Zamykamy końce nieużywane
            close(pipes[1][0]);

            int sum = 0;
            int count = 0;
            int value;
            while (pipeRead(pipes[0][0], value))  // Odczytujemy liczby z pierwszego potoku
            {
                sum += value;
                count++;
            }

            int average = (count > 0) ? sum / count : 0;  // Obliczamy średnią
            pipeWrite(pipes[1][1], average);  // Zapisujemy średnią do drugiego potoku

            close(pipes[0][0]);
            close(pipes[1][1]);
            exit(0);
        }
        else
        {
            // Proces macierzysty - odczytuje wynik z drugiego potoku
            close(pipes[0][0]);
            close(pipes[0][1]);
            close(pipes[1][1]);

            int average;
            if (pipeRead(pipes[1][0], average))  // Odczytujemy średnią z drugiego potoku
            {
                std::cout << "Obliczona średnia: " << average << std::endl;  // Wyświetlamy średnią
            }

            close(pipes[1][0]);  // Zamykanie końca odczytu z drugiego potoku
            waitpid(firstChildPid, nullptr, 0);  // Czekamy na zakończenie pierwszego procesu
            waitpid(secondChildPid, nullptr, 0);  // Czekamy na zakończenie drugiego procesu
        }
    }
    return 0;
}
