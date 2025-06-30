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
    // 1. WinSock �ʱ�ȭ
    WSAData wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET ClientSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    SOCKADDR_IN ServerAddr = { 0 };
    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    ServerAddr.sin_port = htons(30303);

    if (connect(ClientSock, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)) != 0) {
        std::cerr << "���� ���� ����" << std::endl;
        return -1;
    }

    std::cout << "������ ����Ǿ����ϴ�. WSAD �Է����� �̵�, Q ����" << std::endl;

    int x = 0, y = 0;

    while (true) {
        char input = _getch();
        if (input == 'q') break;

        // 2. FlatBuffer�� �Է� �޽��� ����ȭ
        flatbuffers::FlatBufferBuilder builder;
        auto inputOffset = builder.CreateString(std::string(1, input));
        auto inputPacket = Input::CreatePlayerInput(builder, inputOffset);
        builder.Finish(inputPacket);

        int size = htonl(builder.GetSize());
        send(ClientSock, (char*)&size, sizeof(size), 0);
        send(ClientSock, (char*)builder.GetBufferPointer(), builder.GetSize(), 0);

        // 3. ���� ���� ����
        int recvSize = 0;
        int ret = recv(ClientSock, (char*)&recvSize, sizeof(recvSize), MSG_WAITALL);
      

        recvSize = ntohl(recvSize);
        char recvBuffer[1024] = { 0 };
        ret = recv(ClientSock, recvBuffer, recvSize, MSG_WAITALL);
       

        // 4. FlatBuffer �Ľ�
        auto state = Controller::GetPlayerState(recvBuffer);
        x = state->x();
        y = state->y();

        // 5. ȭ�鿡 �÷��̾� ǥ��
        system("cls");
        COORD pos = { (SHORT)x, (SHORT)y };
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
        std::cout << "*";
    }

    closesocket(ClientSock);
    WSACleanup();

    return 0;
}