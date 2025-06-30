#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define NOMINMAX

#include <iostream>
#include <WinSock2.h>
#include "flatbuffers/flatbuffers.h"
#include "Input_generated.h"
#include "Controller_generated.h"

#pragma comment(lib, "ws2_32")

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

    int x = 0;
    int y = 0;

    while (true) {
        // 1. 길이 수신
        int PacketSize = 0;
        int recvBytes = recv(ClientSocket, (char*)&PacketSize, sizeof(PacketSize), MSG_WAITALL);
       
        PacketSize = ntohl(PacketSize);

        // 2. 본문 수신
        char RecvBuffer[4000] = { 0 };
        recvBytes = recv(ClientSocket, RecvBuffer, PacketSize, MSG_WAITALL);
       

        // 3. 파싱
        auto inputObj = Input::GetPlayerInput(RecvBuffer);
        std::string input = inputObj->input()->str();

        if (input == "q") break;
        else if (input == "w") y--;
        else if (input == "s") y++;
        else if (input == "a") x--;
        else if (input == "d") x++;

        // 4. 응답 생성
        flatbuffers::FlatBufferBuilder builder;
        auto state = Controller::CreatePlayerState(builder, x, y);
        builder.Finish(state);

        int sendSize = htonl(builder.GetSize());
        send(ClientSocket, (char*)&sendSize, sizeof(sendSize), 0);
        send(ClientSocket, (char*)builder.GetBufferPointer(), builder.GetSize(), 0);

        // 5. 출력
        system("cls");  // 선택 사항
        COORD Cur = { (SHORT)x, (SHORT)y };
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), Cur);
        std::cout << "*";
    }


	closesocket(ListenSocket);

	WSACleanup();

	return 0;
}
