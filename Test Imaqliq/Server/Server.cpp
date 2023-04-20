#include <iostream>
#include <string>
#include <cstring>
#include <syslog.h>
#include <dirent.h>
#include <vector>
#include <fstream>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
using namespace std;

int socketDesc;

char *getTime()
{
    time_t now = time(0);
    char *date_time = ctime(&now);
    return date_time;
}
auto ParseHeader(char *recieved_frame, int recieved_frame_len)
{
    struct result
    {
        int rest_bytes;
        string msg;
        int msg_lenght;
    };
    int i = 0;
    for (i = 0; recieved_frame[i] != '|'; i++)
        ;
    char *rest_bytes_str = new char[i];
    strncpy(rest_bytes_str, recieved_frame, i);
    int rest_bytes = atoi(rest_bytes_str);

    auto RR = rest_bytes < recieved_frame_len ? rest_bytes + 1 : recieved_frame_len - i;
    string msg = string(recieved_frame + i + 1, recieved_frame + i + RR);
    int msg_lenght = msg.length();
    return result{rest_bytes, msg, msg_lenght};
}
template <typename T>
int SaveToFile(T const text, char *fileName)
{

    fstream fileStream(fileName, fstream::app);
    if (fileStream.fail())
    {
        throw string("No such file");
    }
    fileStream << endl
               << getTime() << endl;
    fileStream << text;
    fileStream.close();
    return 0;
}

int Bind(string port = "69999")
{
    int socketDesc;
    sockaddr_in addrSpec;
    socketDesc = socket(AF_INET, SOCK_STREAM, 0);
    if (socketDesc == -1)
        return -1;

    addrSpec.sin_family = AF_INET;
    addrSpec.sin_addr.s_addr = INADDR_ANY;
    addrSpec.sin_port = htons(stoi(port));

    int bindError = bind(socketDesc, (struct sockaddr *)&addrSpec, sizeof(addrSpec));
    if (bindError == -1)
        return -1;

    return socketDesc;
}

int Listen(int socketDesc, char *filePath)
{
    int listen_return = listen(socketDesc, 3), newSocket, adrrSize = sizeof(sockaddr);
    int buff_size = 1000;
    char rcvBuff[buff_size] = {0};
    sockaddr address;
    if (listen_return == -1)
        return -1;
    while (true && socketDesc != -1)
    {
        newSocket = accept(socketDesc, (struct sockaddr *)&address, (socklen_t *)&adrrSize);
        string recieved_msg_str;
        int rcvCount = read(newSocket, rcvBuff, buff_size);
        auto recieved_msg = ParseHeader(rcvBuff, rcvCount);
        recieved_msg_str += recieved_msg.msg;
        while (recieved_msg.rest_bytes >= buff_size)
        {
            rcvCount = read(newSocket, rcvBuff, buff_size);
            recieved_msg = ParseHeader(rcvBuff, rcvCount);
            recieved_msg_str += recieved_msg.msg;
        }

        SaveToFile(recieved_msg_str, filePath);
    }

    return true;
}
void StopReceiver(int signum)
{
    if (signum == SIGTERM || signum == SIGHUP)
    {
        // SaveToFile((to_string(signum)).c_str(), "text.txt");
        close(socketDesc);
        socketDesc = -1;
        exit(0);
    }
}
int StartService(string port, char *filePath = "text.txt")
{
    signal(SIGTERM, StopReceiver);
    signal(SIGHUP, StopReceiver);
    try
    {
        if ((socketDesc = Bind(port)) == -1)
        {
            cout << "Error binding port" << endl;
            return -1;
        }
        while (true)
        {

            try
            {
                Listen(socketDesc, filePath);
            }
            catch (exception &err)
            {
            }
        }
    }
    catch (exception &err)
    {
        std::cout << "Exception was caught:" << err.what() << "\nExiting.\n";
    }
    return 0;
}
void Daemon()
{
    pid_t pid;

    pid = fork();
    if (pid < 0)
    {
        exit(EXIT_FAILURE);
    }
    if (pid > 0)
    {
        exit(EXIT_SUCCESS);
    }
    if (setsid() < 0)
    {
        exit(EXIT_FAILURE);
    }
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    pid = fork();
    if (pid < 0)
    {
        exit(EXIT_FAILURE);
    }
    if (pid > 0)
    {
        exit(EXIT_SUCCESS);
    }
}
int main(int argc, char *argv[])
{

    cout << "Start" << endl;
    if (argc < 2)
    {
        cout << "Usage: daemon [1]mode:-d for daemon -e for executable -s to stop [2]server_port [3]File path" << endl;
    }
    string port = "626262";
    if (argc >= 3)
        port = string(argv[2]);
    if (argc > 1 && argv[1] == string("-d"))
    {
        cout << "Daemon mode" << endl;
        Daemon();
    }
    argc == 4 ? StartService(port, argv[3]) : StartService(port);

    return 0;
}