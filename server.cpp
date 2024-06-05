#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <fstream>

#define PORT 8000

std::string urlDecode(const std::string &SRC) {
    std::string ret;
    char ch;
    int i, ii;
    for (i=0; i<SRC.length(); i++) {
        if (SRC[i]=='%') {
            sscanf(SRC.substr(i+1,2).c_str(), "%x", &ii);
            ch=static_cast<char>(ii);
            ret+=ch;
            i=i+2;
        } else {
            ret+=SRC[i];
        }
    }
    return (ret);
}

std::string get_html_content(const std::string& resource)
{
    std::ifstream file(resource);
    if (file)
    {
        std::string html((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        return html;
    }
    return "404";
}

void handle_client(int client_socket)
{
    char buffer[1024] = {0};
    read(client_socket, buffer, 1024);
    std::string request{buffer};
    std::cout << "Received request:\n"
              << buffer << std::endl;

    const auto first_space = request.find(' ') +1;
    const auto second_space = request.find(' ',first_space);
    const auto first_point = request.find('.',first_space+1);
    const auto second_slash = request.find('/',first_point);

    const auto end = std::min(second_space,second_slash);
    const auto resource = request.substr(first_space+1, end - first_space-1);
    // Prepare the HTTP response
    std::string html_content = get_html_content(resource);
    const auto res = request.substr(0, request.find('\n'));
    html_content += "<footer> Your request was:<br>"+urlDecode(res)+" </footer>" ;
    std::string http_response = "HTTP/1.1 200 OK\r\n";
    http_response += "Content-Length: " + std::to_string(html_content.length()) + "\r\n";
    http_response += "Content-Type: text/html\r\n";
    http_response += "\r\n";
    http_response += html_content;

    // Send the response back to the client
    send(client_socket, http_response.c_str(), http_response.length(), 0);
    close(client_socket);
}

int main(int argc, char const *argv[])
{
    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8000
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Binding the socket to the network address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listening for incoming connections
    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    std::cout << "Server is up and running on port " << PORT << std::endl;

    // Accepting incoming connections in an infinite loop
    while (true)
    {
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("accept");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        handle_client(client_socket);
    }

    close(server_fd);
    return 0;
}
