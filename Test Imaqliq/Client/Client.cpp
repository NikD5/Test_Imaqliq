#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <fstream>
#include <sstream>
#include <tuple>

using namespace std;
#include <unistd.h>
#include <netdb.h>

vector<string> PrepAgrs(int argc, char *argv[])
{
    vector<string> args;
    args = vector<string>(argv, argv + argc);
    return args;
}

auto File(string fileName)
{
    struct result
    {
        string file;
        int size;
    };
    string file;
    fstream fileStream(fileName);
    if (fileStream.fail())
    {
        throw string("No such file");
    }
    fileStream.seekg(0, ios::end);
    int size = fileStream.tellg();
    if (size == 0)
    {
        throw string("File is empty");
    }
    fileStream.seekg(0, ios::beg);
    stringstream strStream;
    strStream << fileStream.rdbuf();
    file = string(strStream.str());
    fileStream.close();
    return result{file, size};
}
int Connect(string ip, string port)
{
    addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    auto result = getaddrinfo(ip.c_str(), port.c_str(), &hints, &res);
    if (result != 0)
        throw(string("Error getting destination node. error ") + to_string(result));

    int sock = 0, socket_error;
    sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0)
        throw(string("Error opening socket"));
    socket_error = connect(sock, res->ai_addr, res->ai_addrlen);
    if (socket_error != 0)
        throw(string("Error opening connection"));
    return sock;
}


char *Framer(const char *msg_str, int msg_len, int buff_size, int *start)
{
    int rest_bytes = msg_len - *start, rest_frame;
    string rest_bytes_str = to_string(rest_bytes) + "|";
    rest_frame = buff_size - rest_bytes_str.length();
    char *frame = new char[buff_size];
    strcpy(frame, rest_bytes_str.c_str());
    strncpy(frame + rest_bytes_str.length(), msg_str + *start, rest_frame);
    *start = *start + rest_frame;
    return frame;
}

int Send(int socketDesc, const char *msg_str, int msg_len)
{
    int buff_size = 1000, start = 0;
    while (start < msg_len)
    {
        char *frame = Framer(msg_str, msg_len, buff_size, &start);
        int n = send(socketDesc, frame, buff_size, 0);
        if (start + buff_size> msg_len)
            cout<<start<<endl;
    }
    return 0;
}

void FileFiler()
{
    int size = 14000;
    string str;
    fstream fileStream("text.txt",fstream::out);
    if (fileStream.fail())
    {
        throw string("No such file");
        return;
    }

    for (int i = 0, total = 0; total < size; i++)
    {
        str+="line\t";
        fileStream<<to_string(i)<<" "<< str<<endl;
        total+=str.length()+1+to_string(i).length();
    }
    fileStream.close();
}

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        cout << "Usage: sendfile [1]server_ip [2]server_port [3]file_path" << endl;
    }
    if (argc != 4)
    {
        argv = new char *[4];
        argv[0] = "asda";
        argv[1] = "127.0.0.1";
        argv[2] = "626262";
        argv[3] = "text.txt";
        argc = 4;
    }
    vector<string> args = PrepAgrs(argc, argv);
    string ip = args[1], port = args[2], file;
    int size;
    try
    {
        auto res = File(args[3]);
        file = res.file;
        size = res.size;
    }
    catch (string e)
    {
        cout << e << endl;
        return 0;
    }

    int socketDesc;
    try
    {
        socketDesc = Connect(ip, port);
    }
    catch (string e)
    {
        cout << e << endl;
        return 0;
    }

    if (Send(socketDesc, file.c_str(), size) == -1)
        cout << "Sending error" << endl;

    close(socketDesc);
    return 0;
}