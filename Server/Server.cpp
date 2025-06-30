#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define NOMINMAX
#define _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING

#include <iostream>
#include <WinSock2.h>
#include <algorithm>
#include "Position_generated.h"
#include "InputData_generated.h"

#pragma comment(lib, "ws2_32")

int main()
{
	WSAData wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	SOCKET ListenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	sockaddr_in ListenSockAddr;
	memset(&ListenSockAddr, 0, sizeof(ListenSockAddr));
	ListenSockAddr.sin_family = AF_INET;
	ListenSockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	ListenSockAddr.sin_port = htons(30303);

	bind(ListenSock, (sockaddr*)&ListenSockAddr, sizeof(ListenSockAddr));

	listen(ListenSock, 5);

	sockaddr_in ClientSockAddr;
	memset(&ClientSockAddr, 0, sizeof(ClientSockAddr));
	int ClientSockAddrLength = sizeof(ClientSockAddr);
	SOCKET ClientSock = accept(ListenSock, (sockaddr*)&ClientSockAddr, &ClientSockAddrLength);

	int X = 0;
	int Y = 0;
	std::cout << "*";
	while (true)
	{
		int PacketSize = 0;
		char RecvBuffer[4000] = { 0, };
		int RecvBytes = recv(ClientSock, (char*)&PacketSize, sizeof(PacketSize), MSG_WAITALL);
		PacketSize = ntohl(PacketSize);
		if (RecvBytes <= 0)
		{
			continue;
		}
		system("cls");
		RecvBytes = recv(ClientSock, RecvBuffer, PacketSize, MSG_WAITALL);

		auto InputData = InputData::GetData(RecvBuffer);

		switch (InputData->input())
		{
		case 'w':
			Y--;
			break;
		case 's':
			Y++;
			break;
		case 'a':
			X--;
			break;
		case 'd':
			X++;
			break;
		}
		X = std::clamp(X, 0, 10);
		Y = std::clamp(Y, 0, 10);
		COORD Cur;
		Cur.X = X;
		Cur.Y = Y;
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), Cur);
		std::cout << "*";

		flatbuffers::FlatBufferBuilder Builder(1024);
		auto Data = Position::CreateData(Builder, X, Y);
		Builder.Finish(Data);

		PacketSize = (int)Builder.GetSize();
		PacketSize = htonl(PacketSize);
		int SentBytes = send(ClientSock, (char*)&PacketSize, sizeof(PacketSize), 0);
		SentBytes = send(ClientSock, (char*)Builder.GetBufferPointer(), (int)Builder.GetSize(), 0);
	}

	closesocket(ListenSock);

	WSACleanup();

	return 0;
}