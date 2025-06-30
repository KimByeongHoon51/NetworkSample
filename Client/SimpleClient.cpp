#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define NOMINMAX


#include <iostream>
#include <conio.h>
#include <WinSock2.h>
#include <Windows.h>
#include "flatbuffers/flatbuffers.h"
#include "Input_generated.h"
#include "Controller_generated.h"

#pragma comment(lib, "ws2_32")

int main() {
    // 1. WinSock 초기화
    WSAData wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET ClientSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    SOCKADDR_IN ServerAddr = { 0 };
    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    ServerAddr.sin_port = htons(30303);

    if (connect(ClientSock, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)) != 0) {
        std::cerr << "서버 연결 실패" << std::endl;
        return -1;
    }

    std::cout << "서버에 연결되었습니다. WSAD 입력으로 이동, Q 종료" << std::endl;

    int x = 0, y = 0;

    while (true) {
        char input = _getch();
        if (input == 'q') break;

        // 2. FlatBuffer로 입력 메시지 직렬화
        flatbuffers::FlatBufferBuilder builder;
        auto inputOffset = builder.CreateString(std::string(1, input));
        auto inputPacket = Input::CreatePlayerInput(builder, inputOffset);
        builder.Finish(inputPacket);

        int size = htonl(builder.GetSize());
        send(ClientSock, (char*)&size, sizeof(size), 0);
        send(ClientSock, (char*)builder.GetBufferPointer(), builder.GetSize(), 0);

        // 3. 서버 응답 수신
        int recvSize = 0;
        int ret = recv(ClientSock, (char*)&recvSize, sizeof(recvSize), MSG_WAITALL);
      

        recvSize = ntohl(recvSize);
        char recvBuffer[1024] = { 0 };
        ret = recv(ClientSock, recvBuffer, recvSize, MSG_WAITALL);
       

        // 4. FlatBuffer 파싱
        auto state = Controller::GetPlayerState(recvBuffer);
        x = state->x();
        y = state->y();

        // 5. 화면에 플레이어 표시
        system("cls");
        COORD pos = { (SHORT)x, (SHORT)y };
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
        std::cout << "*";
    }

    closesocket(ClientSock);
    WSACleanup();

    return 0;
}