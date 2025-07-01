#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define NOMINMAX

#include <iostream>
#include <conio.h>
#include <WinSock2.h>
#include <Windows.h>

#include "flatbuffers/flatbuffers.h"
#include "UserEvent_generated.h"


#pragma comment(lib, "ws2_32")

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
	auto LoginEvent = UserEvent::CreateC2S_Login(Builder, Builder.CreateString("username"), Builder.CreateString("password"));
	auto EventData = UserEvent::CreateEventData(Builder, GetTimeStamp(), UserEvent::EventType_C2S_Login, LoginEvent.Union());
	Builder.Finish(EventData);
}

void RunGameLoop(SOCKET ServerSocket)
{
	char key;
	int x = 0, y = 0;
	while (true)
	{
		key = _getch();

		// 'q', 'Q'일 때는 로그아웃 처리
		if (tolower(key) == 'q') 
		{
			flatbuffers::FlatBufferBuilder builder;
			auto logoutData = UserEvent::CreateC2S_Logout(builder, 1); // user id 예시
			auto eventData = UserEvent::CreateEventData(builder, GetTimeStamp(), UserEvent::EventType_C2S_Logout, logoutData.Union());
			builder.Finish(eventData);
			SendPacket(ServerSocket, builder);
			break;
		}

		// 이동 키일 경우 서버로 전송
		flatbuffers::FlatBufferBuilder builder;
		auto movePacket = UserEvent::CreateC2S_PlayerMoveData(builder, 1, 0, 0, key); // 좌표는 서버가 계산함
		auto eventData = UserEvent::CreateEventData(builder, GetTimeStamp(), UserEvent::EventType_C2S_PlayerMoveData, movePacket.Union());
		builder.Finish(eventData);
		SendPacket(ServerSocket, builder);

		// 이동 결과 수신
		char RecvBuffer[10240] = { 0 };
		RecvPacket(ServerSocket, RecvBuffer);
		auto event = UserEvent::GetEventData(RecvBuffer);
		if (event->data_type() == UserEvent::EventType_S2C_PlayerMoveData) {
			auto pos = event->data_as_S2C_PlayerMoveData();
			x = pos->position_x();
			y = pos->position_y();
		}

		// 화면 클리어 및 * 출력
		system("cls");
		for (int i = 0; i < y; i++) std::cout << "\n";
		for (int i = 0; i < x; i++) std::cout << " ";
		std::cout << "*\n";
	}
	char RecvBuffer[10240] = { 0 };
	RecvPacket(ServerSocket, RecvBuffer);

	auto RecvEventData = UserEvent::GetEventData(RecvBuffer);
	std::cout << "Recv Logout Response. EventType: " << RecvEventData->data_type() << std::endl;

	if (RecvEventData->data_type() == UserEvent::EventType_S2C_Logout)
	{
		auto logoutData = RecvEventData->data_as_S2C_Logout();
		std::cout << "Logout Result: " << logoutData->message()->c_str() << std::endl;
	}
}

void ProcessPacket(const char* RecvBuffer, SOCKET ServerSocket)
{
	//root_type
	auto RecvEventData = UserEvent::GetEventData(RecvBuffer);
	std::cout << RecvEventData->timestamp() << std::endl; //타임스탬프

	switch (RecvEventData->data_type())
	{
	case UserEvent::EventType_S2C_Login:
	{
		auto LoginData = RecvEventData->data_as_S2C_Login();
		if (LoginData->success())
		{
			std::cout << LoginData->message()->c_str() << std::endl;
			RunGameLoop(ServerSocket); // 게임 시작
		}
		else
		{
			std::cout << "Login Failed: " << LoginData->message()->c_str() << std::endl;
		}
	}

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

	// 로그인 요청
	flatbuffers::FlatBufferBuilder Builder;
	CreateC2S_Login(Builder);
	SendPacket(ServerSocket, Builder);

	// 로그인 응답 받기
	char RecvBuffer[10240] = { 0 };
	RecvPacket(ServerSocket, RecvBuffer);
	ProcessPacket(RecvBuffer, ServerSocket);

	auto RecvEventData = UserEvent::GetEventData(RecvBuffer);
	if (RecvEventData->data_type() == UserEvent::EventType_S2C_Login)
	{
		auto LoginData = RecvEventData->data_as_S2C_Login();
		if (LoginData->success())
		{
			std::cout << LoginData->message()->c_str() << std::endl;
		}
		else
		{
			std::cout << "Login Failed: " << LoginData->message()->c_str() << std::endl;
		}
	}

	closesocket(ServerSocket);
	WSACleanup();

	return 0;
}
