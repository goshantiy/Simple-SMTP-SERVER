#include <iostream>
#include <WinSock2.h>
#include "SMTP.h"
#include <fstream>
#include <list>
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable: 4996)

enum class commands
{
	HELO,
	MAIL,
	RCPT,
	DATA,
	QUIT,
	ERR
};


commands hashit(std::string &inString)
{
	if (inString == "HELO") return commands::HELO;
	if (inString == "MAIL") return commands::MAIL;
	if (inString == "RCPT") return commands::RCPT;
	if (inString == "DATA") return commands::DATA;
	if (inString == "QUIT") return commands::QUIT;
	return commands::ERR;
	
}
std::string getLine(SOCKET connection)
{
	std::string result;
	char buffer[1];
	ZeroMemory(buffer, sizeof(buffer));
	int res = recv(connection, buffer, 1, 0);
	while (*buffer != '\n'&&res)
	{
		result += *buffer;
		res = recv(connection, buffer, 1, 0);
	}
	result += "\r\n";
	return result;
}
int main()
{
	int res;
	WSAData wsaData;
	WORD DLLVersion = MAKEWORD(2, 2);
	if (WSAStartup(DLLVersion, &wsaData) != 0) {
		std::cout << "Error" << std::endl;
		exit(1);
	}
	//устанавливаем адрес и порт
	SOCKADDR_IN address;
	int sizeofaddr = sizeof(address);
	address.sin_addr.s_addr = inet_addr("127.0.0.1");
	address.sin_port = htons(1234);
	address.sin_family = AF_INET;
	//создаем сокет и устанавливаем на прослушивание
	SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL);
	bind(sListen, (SOCKADDR*)&address, sizeof(address));
	listen(sListen, SOMAXCONN);
	std::cout << "Waiting for client.\n";
	//принимаем клиент
	SOCKET connection;
	connection = accept(sListen, (SOCKADDR*)&address, &sizeofaddr);


	if (connection == 0) {
		std::cout << "Error" << std::endl;
	}
	else
	{
		closesocket(sListen);
		std::cout << "Client Connected!\n";
		char msg[256] = "Connected\n\r";
		send(connection, msg, sizeof(msg), NULL);//отправляем сообщение клиенту
		ZeroMemory(msg, sizeof(msg));
		int bytes_received = 0;
		std::string mailbuffer;
		bool res = true;
		std::list<std::string> rcpt;
		do
		{
			
			std::string receive_buffer;
			receive_buffer = getLine(connection);
			std::string buf = receive_buffer.substr(0, 4);
						
			
			
			switch (hashit(buf))
			{
			case commands::HELO:
				{
					char msg[256] = "220 OK\n";
					send(connection, msg, sizeof(msg), NULL);
					break;
				}
			case commands::QUIT:
				{
					char msg[256] = "goodbye:)))";
					send(connection, msg, sizeof(msg), NULL);//
					res = shutdown(connection, SD_SEND);
					closesocket(connection);
					res = false;
					break;
					
				}
			case commands::MAIL:
			{
				std::string buff = receive_buffer.substr(receive_buffer.find_first_of(':') + 1);
				mailbuffer = buff;
				char msg[256] = "OK 220\n";
				send(connection, msg, sizeof(msg), NULL);//
				break;
			}
			case commands::RCPT:
			{
				std::string buff = receive_buffer.substr(receive_buffer.find_first_of(':') + 1);
				buff = buff.substr(0,buff.find_first_of('\r'));
		
					buff += ".txt";
					std::ofstream file(buff);
					mailbuffer = mailbuffer.substr(0, mailbuffer.find_first_of('\r'));
					file << mailbuffer << ':';
					file.close();
					rcpt.push_back(buff);
					char msg[256] = "OK 220\n";
					send(connection, msg, sizeof(msg), NULL);//
					break;
			}
			case commands::DATA:
			{
				std::list<std::string>::iterator it;
				std::string buff = receive_buffer.substr(receive_buffer.find_first_of(':') + 1);
				it = rcpt.begin();
				int sizeofarr = rcpt.size();
				std::string msgbuf;
				msgbuf = '\n';
				std::string line=buff;
				while (line[0] != '.' || line[1] != '\r')
				{

					msgbuf +=line;
					msgbuf= msgbuf.substr(0,msgbuf.find_last_of('\n'));
					line = getLine(connection);
				}
				
				while (sizeofarr > 0)
				{
					std::ofstream file(*it,std::ios_base::app);
					file <<msgbuf;
					it++;
					sizeofarr--;
					file.close();
				}
				char msg[256] = "OK 220\n";
				send(connection, msg, sizeof(msg), NULL);//
				break;
			}
			case commands::ERR: 
			{
				char msg[256] = "550\n";
				send(connection, msg, sizeof(msg), NULL);
			}
			}
		} while (res);
			
	}
	res = shutdown(connection, SD_SEND);
	if (connection == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(connection);
		WSACleanup();
		return 1;
	}
	system("pause");
	return 0;
}

