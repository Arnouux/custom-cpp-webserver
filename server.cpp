#include <iostream>
#include <winsock2.h>
#include <future>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <time.h>
#include <regex>

std::vector<char> getHeader(int size, std::string content_type) {
    char buf[1000];
    time_t now = time(0);
    struct tm tm = *gmtime(&now);
    strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S %Z", &tm);

    std::string s = "HTTP/1.1 200 OK\r\n"
"Content-Length: " + std::to_string(size) + "\r\n" 
"Connection: close\r\n"
"Date: " + buf + "\r\n"
"Content-type: " + content_type + "\r\n"
"\r\n";
    std::vector<char> vec(s.begin(), s.end());
    return vec;
}

std::vector<char> getDataWithHeader(std::string path, std::string content_type) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    std::vector<char> result;

    std::vector<char> vec_header = getHeader(file.tellg(), content_type);
    std::vector<char> vec_data;

    if (!file.eof() && !file.fail()) {
        file.seekg(0, std::ios_base::end);
        std::streampos fileSize = file.tellg();
        vec_data.resize(fileSize);

        file.seekg(0, std::ios_base::beg);
        file.read(&vec_data[0], fileSize);
    }
    result.insert( result.begin(), vec_header.begin(), vec_header.end() );
    result.insert( result.end(), vec_data.begin(), vec_data.end() );


    return result;
}

std::vector<std::string> addresses;
int nb_visits;

int main() {
    freopen( "logs.lst", "w", stdout );
    std::cout << "hello" << std::endl;

    // todo change if unix env (#DEFINE ?)
	WSADATA wsa_data;
	WSAStartup(MAKEWORD(2, 2), &wsa_data);

    SOCKADDR_IN address, client_addr;

    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_family = AF_INET;
    address.sin_port = htons(5555);
    SOCKET server = socket(AF_INET, SOCK_STREAM, 0);
    bind(server, reinterpret_cast<SOCKADDR *>(&address), sizeof(address));
    if (listen(server, 0)) {
        std::cout << "Error on listen" << std::endl;
        return 1;
    }

    int client_addr_size = sizeof(client_addr);
    char buf[1024] = { 0 };
    for(;;) {
		SOCKET client;

		if ((client = accept(server, reinterpret_cast<SOCKADDR *>(&client_addr), &client_addr_size)) != INVALID_SOCKET)
		{
            std::cout << "client accepted:" << inet_ntoa(client_addr.sin_addr) << std::endl;
            if(addresses.size() > 50) {
                // todo add timer per address for anti spam
                std::cout << addresses.size() << std::endl;
            } else {

                int received = recv(client, buf, 1024, 0);
                std::string data(buf, buf + 1024);
                char *html_needed = NULL;
                char *css_needed = NULL;
                const std::regex reg("/imgs/[^ ]*");
                std::smatch match;

                html_needed = strstr (buf, "text/html");
                css_needed = strstr (buf, "text/css");
                bool img_needed = std::regex_match(buf, reg);
                if(html_needed) {
                    std::vector<char> result = getDataWithHeader("site.html", "text/html");
                    send(client, &result[0], result.size(), 0);
                    std::cout << "sent html" << std::endl;
                } else if(css_needed) {
                    // TODO: generalize css (as img)
                    std::vector<char> result = getDataWithHeader("site.css", "text/css");
                    send(client, &result[0], result.size(), 0);
                    std::cout << "sent css" << std::endl;
                } else if (std::regex_search(data, match, reg)) {
                    std::vector<char> result = getDataWithHeader("." + std::string(match[0]), "image/png");
                    send(client, &result[0], result.size(), 0);
                    std::cout << "sent img" << std::endl;
                }

                addresses.push_back(inet_ntoa(client_addr.sin_addr));
                nb_visits += 1;
            }
        }

		const auto last_error = WSAGetLastError();
		
		if(last_error > 0)
		{
			std::cout << "Error: " << last_error << std::endl;
		}

    }
    
}