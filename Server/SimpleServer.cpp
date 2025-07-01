#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define NOMINMAX

#include <iostream>
#include <WinSock2.h>

#include "flatbuffers/flatbuffers.h"
#include "UserEvent_generated.h"

#pragma comment(lib, "ws2_32")

const int MAX_PLAYERS = 10;  // 최대 10명
uint16_t playerX[MAX_PLAYERS] = { 0 };
uint16_t playerY[MAX_PLAYERS] = { 0 };

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

void CreateS2C_Login(flatbuffers::FlatBufferBuilder& Builder, bool IsSuccess, std::string Message)
{
	auto LoginEvent = UserEvent::CreateS2C_Login(Builder, IsSuccess, Builder.CreateString(Message));
	auto EventData = UserEvent::CreateEventData(Builder, GetTimeStamp(), UserEvent::EventType_S2C_Login, LoginEvent.Union());
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
	auto RecvEventData = UserEvent::GetEventData(RecvBuffer);
	std::cout << RecvEventData->timestamp() << std::endl; //타임스탬프

	flatbuffers::FlatBufferBuilder Builder;

	switch (RecvEventData->data_type())
	{
	case UserEvent::EventType_C2S_Login:
	{
		auto LoginData = RecvEventData->data_as_C2S_Login();
		if (LoginData->userid() && LoginData->password())
		{
			std::cout << "Login Request success: " << LoginData->userid()->c_str() << ", " << LoginData->password()->c_str() << std::endl;
			CreateS2C_Login(Builder, true, "Login Success");
		}
		else
		{
			CreateS2C_Login(Builder, false, "empty id, password");
		}
		SendPacket(ClientSocket, Builder);
	}
	break;

	case UserEvent::EventType_C2S_PlayerMoveData:
	{
		auto moveData = RecvEventData->data_as_C2S_PlayerMoveData();
		uint32_t id = moveData->player_id();
		char key = moveData->key_code();

		uint16_t x = playerX[id];
		uint16_t y = playerY[id];

		switch (tolower(key)) {
		case 'w': y--; break;
		case 's': y++; break;
		case 'a': x--; break;
		case 'd': x++; break;
		}

		playerX[id] = x;
		playerY[id] = y;

		// 응답 전송
		auto response = UserEvent::CreateS2C_PlayerMoveData(Builder, id, x, y);
		auto eventData = UserEvent::CreateEventData(Builder, GetTimeStamp(), UserEvent::EventType_S2C_PlayerMoveData, response.Union());
		Builder.Finish(eventData);
		SendPacket(ClientSocket, Builder);
	}
	break;


	case UserEvent::EventType_C2S_Logout:
	{
		auto logoutData = RecvEventData->data_as_C2S_Logout();
		std::cout << "Logout Request from ID: " << logoutData->userid() << std::endl;

		auto message = Builder.CreateString("Logout Success");
		auto logoutEvent = UserEvent::CreateS2C_Logout(Builder, logoutData->userid(), true, message);
		auto eventData = UserEvent::CreateEventData(Builder, GetTimeStamp(), UserEvent::EventType_S2C_Logout, logoutEvent.Union());
		Builder.Finish(eventData);
		SendPacket(ClientSocket, Builder);
	}
	break;


	
	}
	return 0;
}