#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define NOMINMAX


#include <iostream>
#include <WinSock2.h>
#include <Windows.h>
#include <conio.h>

#include "flatbuffers/flatbuffers.h"
#include "UserEvents_generated.h"


#pragma comment(lib, "ws2_32")

uint32_t MyPlayerId = 0;

uint64_t GetTimeStamp()
{
	return (uint64_t)time(NULL);
}

void SendPacket(SOCKET Socket, flatbuffers::FlatBufferBuilder& Builder)
{
	int PacketSize = (int)Builder.GetSize();
	PacketSize = ::htonl(PacketSize);
	//header, 길이
	int SentBytes = ::send(Socket, (char*)&PacketSize, sizeof(PacketSize), 0);
	//자료 
	SentBytes = ::send(Socket, (char*)Builder.GetBufferPointer(), Builder.GetSize(), 0);
	if (SentBytes <= 0)
	{
		std::cout << "Send failed: " << WSAGetLastError() << std::endl;
	}
}

void RecvPacket(SOCKET Socket, char* Buffer)
{
	int PacketSize = 0;
	int RecvBytes = recv(Socket, (char*)&PacketSize, sizeof(PacketSize), MSG_WAITALL);
	if (RecvBytes <= 0)
	{
		std::cout << "Header Recv failed: " << WSAGetLastError() << std::endl;
		return;
	}
	PacketSize = ntohl(PacketSize);
	RecvBytes = recv(Socket, Buffer, PacketSize, MSG_WAITALL);
	if (RecvBytes <= 0)
	{
		std::cout << "Body Recv failed: " << WSAGetLastError() << std::endl;
		return;
	}
}
void CreateC2S_Login(flatbuffers::FlatBufferBuilder& Builder)
{
	if (MyPlayerId > 0)
	{
		return;
	}
	auto LoginEvent = UserEvents::CreateC2S_Login(Builder, Builder.CreateString("username"), Builder.CreateString("password"));
	auto EventData = UserEvents::CreateEventData(Builder, GetTimeStamp(), UserEvents::EventType_C2S_Login, LoginEvent.Union());
	Builder.Finish(EventData);
}
void CreateC2S_Logout(flatbuffers::FlatBufferBuilder& Builder)
{
	if (MyPlayerId <= 0)
	{
		return;
	}
	auto LogoutEvent = UserEvents::CreateC2S_Logout(Builder, MyPlayerId);
	auto EventData = UserEvents::CreateEventData(Builder, GetTimeStamp(), UserEvents::EventType_C2S_Logout, LogoutEvent.Union());
	Builder.Finish(EventData);
}
void CreateC2S_PlayerMoveData(flatbuffers::FlatBufferBuilder& Builder, uint8_t KeyCode)
{
	if (MyPlayerId <= 0)
	{
		return;
	}
	auto PlayerMoveEvent = UserEvents::CreateC2S_PlayerMoveData(Builder, MyPlayerId, KeyCode);
	auto EventData = UserEvents::CreateEventData(Builder, GetTimeStamp(), UserEvents::EventType_C2S_PlayerMoveData, PlayerMoveEvent.Union());
	Builder.Finish(EventData);
}
void ProcessPacket(const char* RecvBuffer)
{
	//root_type
	auto RecvEventData = UserEvents::GetEventData(RecvBuffer);
	//std::cout << RecvEventData->timestamp() << std::endl; //타임스탬프

	switch (RecvEventData->data_type())
	{
	case UserEvents::EventType_S2C_Login:
	{
		auto LoginData = RecvEventData->data_as_S2C_Login();
		MyPlayerId = LoginData->player_id();
		system("cls");
		COORD Cur;
		Cur.X = LoginData->position_x();
		Cur.Y = LoginData->position_y();
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), Cur);
		std::cout << "*";
	}
	break;
	case UserEvents::EventType_S2C_Logout:
	{
		auto LogoutData = RecvEventData->data_as_S2C_Logout();
		MyPlayerId = 0;
		system("cls");
	}
	break;
	case UserEvents::EventType_S2C_PlayerMoveData:
	{
		auto MoveData = RecvEventData->data_as_S2C_PlayerMoveData();
		system("cls");
		COORD Cur;
		Cur.X = MoveData->position_x();
		Cur.Y = MoveData->position_y();
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), Cur);
		std::cout << "*";
	}
	break;
	}
}


int main()
{
	WSAData wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	SOCKET ServerSocket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	SOCKADDR_IN ServerSockAddr;
	memset(&ServerSockAddr, 0, sizeof(ServerSockAddr));
	ServerSockAddr.sin_family = PF_INET;
	ServerSockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	ServerSockAddr.sin_port = htons(30303);

	connect(ServerSocket, (SOCKADDR*)&ServerSockAddr, sizeof(ServerSockAddr));
	while (true)
	{
		char Input;
		if (_kbhit())
		{
			Input = _getch();
		}
		else
		{
			continue;
		}
		char RecvBuffer[10240] = { 0 };
		flatbuffers::FlatBufferBuilder Builder;

		switch (Input)
		{
		case 'w':
		case 's':
		case 'a':
		case 'd':
			CreateC2S_PlayerMoveData(Builder, Input);
			break;
		case 'i':
			CreateC2S_Login(Builder);
			break;
		case 'o':
			CreateC2S_Logout(Builder);
			break;
		default:
			break;
		}

		if (Builder.GetSize() <= 0)
		{
			continue;
		}

		SendPacket(ServerSocket, Builder);

		RecvPacket(ServerSocket, RecvBuffer);

		ProcessPacket(RecvBuffer);
	}

	closesocket(ServerSocket);

	WSACleanup();

	return 0;
}