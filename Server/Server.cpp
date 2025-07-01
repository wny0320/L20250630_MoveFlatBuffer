#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define NOMINMAX

#include <iostream>
#include <WinSock2.h>

#include "flatbuffers/flatbuffers.h"
#include "UserEvents_generated.h"


#pragma comment(lib, "ws2_32")

uint16_t PosX = 0;
uint16_t PosY = 0;

uint64_t GetTimeStamp()
{
	return (uint64_t)time(NULL);
}

int ProcessPacket(const char* RecvBuffer, SOCKET ClientSocket);


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


void CreateS2C_Login(flatbuffers::FlatBufferBuilder& Builder, bool IsSuccess, std::string Message, uint16_t PosX, uint16_t PosY, UserEvents::Color Color, uint32_t PlayerId)
{
	auto LoginEvent = UserEvents::CreateS2C_Login(Builder, IsSuccess, Builder.CreateString(Message), PosX, PosY, &Color, PlayerId);
	auto EventData = UserEvents::CreateEventData(Builder, GetTimeStamp(), UserEvents::EventType_S2C_Login, LoginEvent.Union());
	Builder.Finish(EventData);
}

void CreateS2C_Logout(flatbuffers::FlatBufferBuilder& Builder, uint32_t PlayerId, bool IsSuccess, std::string Message)
{
	auto LogoutEvent = UserEvents::CreateS2C_Logout(Builder, PlayerId, IsSuccess, Builder.CreateString(Message));
	auto EventData = UserEvents::CreateEventData(Builder, GetTimeStamp(), UserEvents::EventType_S2C_Logout, LogoutEvent.Union());
	Builder.Finish(EventData);
}

void CreateS2C_PlayerMoveData(flatbuffers::FlatBufferBuilder& Builder, uint32_t PlayerId, uint16_t PosX, uint16_t PosY)
{
	auto PlayerMoveEvent = UserEvents::CreateS2C_PlayerMoveData(Builder, PlayerId, PosX, PosY);
	auto EventData = UserEvents::CreateEventData(Builder, GetTimeStamp(), UserEvents::EventType_S2C_PlayerMoveData, PlayerMoveEvent.Union());
	Builder.Finish(EventData);
}

int main()
{
	WSAData wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	SOCKET ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	SOCKADDR_IN ListenSockAddr;
	memset(&ListenSockAddr, 0, sizeof(ListenSockAddr));
	ListenSockAddr.sin_family = PF_INET;
	ListenSockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	ListenSockAddr.sin_port = htons(30303);

	bind(ListenSocket, (SOCKADDR*)&ListenSockAddr, sizeof(ListenSockAddr));

	listen(ListenSocket, 5);
	SOCKADDR_IN ClientSockAddr;
	memset(&ClientSockAddr, 0, sizeof(ClientSockAddr));
	int ClientSockAddrLength = sizeof(ClientSockAddr);
	SOCKET ClientSocket = accept(ListenSocket, (SOCKADDR*)&ClientSockAddr, &ClientSockAddrLength);

	while (true)
	{
		char RecvBuffer[4000] = { 0, };
		RecvPacket(ClientSocket, RecvBuffer);

		int Result = ProcessPacket(RecvBuffer, ClientSocket);
	}

	closesocket(ListenSocket);

	WSACleanup();

	return 0;
}

int ProcessPacket(const char* RecvBuffer, SOCKET ClientSocket)
{
	//root_type
	auto RecvEventData = UserEvents::GetEventData(RecvBuffer);
	std::cout << RecvEventData->timestamp() << std::endl; //타임스탬프

	flatbuffers::FlatBufferBuilder Builder;

	switch (RecvEventData->data_type())
	{
	case UserEvents::EventType_C2S_Login:
	{
		auto LoginData = RecvEventData->data_as_C2S_Login();
		if (LoginData->userid() && LoginData->password())
		{
			std::cout << "Login Request success: " << LoginData->userid()->c_str() << ", " << LoginData->password()->c_str() << std::endl;
			CreateS2C_Login(Builder, true, "Login Success", PosX, PosY, UserEvents::Color(255, 255, 255), (uint32_t)ClientSocket);
		}
		else
		{
			CreateS2C_Login(Builder, false, "empty id, password", 0, 0, UserEvents::Color(0, 0, 0), (uint32_t)ClientSocket);
		}
		SendPacket(ClientSocket, Builder);
	}
	break;
	case UserEvents::EventType_C2S_Logout:
	{
		auto LogoutData = RecvEventData->data_as_C2S_Logout();
		if (LogoutData->player_id())
		{
			std::cout << "Logout Request success: " << LogoutData->player_id() << std::endl;
			CreateS2C_Logout(Builder, LogoutData->player_id(), true, "Logout Success");
		}
		else
		{
			CreateS2C_Logout(Builder, 0, false, "Logout Failed");
		}
		SendPacket(ClientSocket, Builder);
	}
	break;
	case UserEvents::EventType_C2S_PlayerMoveData:
	{
		auto MoveData = RecvEventData->data_as_C2S_PlayerMoveData();
		if (MoveData->key_code())
		{
			std::cout << "Move Request success: " << MoveData->key_code() << std::endl;
		}
		else
		{
			std::cout << "Move Request Failed" << std::endl;
			break;
		}
		int MoveX = 0;
		int MoveY = 0;
		switch (MoveData->key_code())
		{
		case 'w':
			MoveY--;
			break;
		case 's':
			MoveY++;
			break;
		case 'a':
			MoveX--;
			break;
		case 'd':
			MoveX++;
			break;
		default:
			break;
		}
		PosX = std::clamp((int)PosX + MoveX, 0, 10);
		PosY = std::clamp((int)PosY + MoveY, 0, 10);
		CreateS2C_PlayerMoveData(Builder, (uint32_t)ClientSocket, PosX, PosY);
		SendPacket(ClientSocket, Builder);
	}
	break;
	}

	return 0;
}