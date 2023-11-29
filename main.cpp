#include <iostream>
#include <winsock2.h>
#include <string>
#include <thread>
#include <windows.h>

/**
 * Stiahnutá knižnica z github.com
 * URL: https://github.com/d-bahr/CRCpp.git
*/
#include "CRC.h"

using namespace std;
static string role;
static int dataLenght = 1400;
static int listening_port = 12345;
static bool endConnection = false, sendData = false, receiveData = false;

static SOCKADDR_IN serverAddress;
static sockaddr_in clientAdd;
static SOCKET clientS, serverS;

struct Header {
    unsigned char type;
    unsigned short lenght;
    unsigned short numberOfFragments;
    unsigned short fragmentInSequence;
    unsigned short crc;
};

void codeMessage(Header * header, char text[], int text_size, char message[]);
void decodeMessage(Header *header1, char text[], int text_size, char message[]);

int main() {


    bool correctRole = false;
    cout << "Zadaj ci chces figurovat ako server alebo klient" << endl;
    cin >> role;
    while(!correctRole){
        if(strcmpi(role.c_str(),"klient") == 0){
            cout << "Vybral si rolu klienta" << endl;
            role = "klient";
            correctRole = true;
        }
        else if(strcmpi(role.c_str(),"server") == 0){
            cout << "Vybral si rolu servera" << endl;
            role = "server";
            correctRole = true;
        }
        else {
            cout << "Zadal si neplatnu rolu tak skus este raz" << endl;
            cin >> role;
        }
    }

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Nepodarilo sa inicializovat winsock." << std::endl;
        return 1;
    }



    if(role == "klient"){
        string add;
        int port;
        cout << "Zadaj adresu servera" << endl;
        cin >> add;
        cout << "Zadaj cislo portu " << endl;
        cin >> port;

        // Vytvorenie socketu
        clientS = socket(AF_INET,SOCK_DGRAM, 0);
        if (clientS == INVALID_SOCKET) {
            std::cerr << "Nepodaril sa vytvorit socket." << std::endl;
            WSACleanup();
            return 1;
        }
        //memset(&serverAddress,0,sizeof(SOCKADDR_IN ));
        //SOCKADDR_IN serverAddress;
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(port);
        serverAddress.sin_addr.s_addr = inet_addr(add.c_str());

        if (connect(clientS, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
            std::cerr << "Neporadilo sa pripojit na server" << std::endl;
            closesocket(clientS);
            WSACleanup();
            return 1;
        }

        // Send a message to the server
        char text[] = "Chcem nadviazat spojenie";
        Header header {0b00000001,25,1,1,0};
        char message[sizeof(text) + sizeof(header)];

        codeMessage(&header, text, sizeof(text), message);
        sendto(clientS, message, sizeof(message), 0,reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress));

        // Receive the response from the server
        char buffer[1500];
        sockaddr_in serverAdd;
        int size = sizeof(serverAdd);

        int bytesReceived = recvfrom(clientS, buffer, sizeof(buffer), 0, reinterpret_cast<SOCKADDR *>(&serverAdd),&size);
        if (bytesReceived > 0) {
            char data[bytesReceived - 9];
            Header header1;
            decodeMessage(&header1,data, sizeof(data),buffer);
            std::cout << "Received from server: " << data << std::endl;
        }

    }
    else if(role == "server"){
    // Vytvorenie socketu
        serverS = socket(AF_INET,SOCK_DGRAM, 0);
        if (serverS == INVALID_SOCKET) {
            cout << "Nepodaril sa vytvorit socket." << endl;
            WSACleanup();
            return 1;
        }

        //memset(&serverAddress,0,sizeof(SOCKADDR_IN ));
        //SOCKADDR_IN serverAddress;
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(listening_port);
        serverAddress.sin_addr.s_addr = INADDR_ANY;

        if (bind(serverS, reinterpret_cast<SOCKADDR *>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
            cout << "Nepodaril sa nastavit socket!" << endl;
            closesocket(serverS);
            WSACleanup();
            return 1;
        }

        char message[1500];
//        sockaddr_in clientAdd;
        int size = sizeof(clientAdd);

        int recievedByt = recvfrom(serverS, message, sizeof(message), 0, reinterpret_cast<SOCKADDR *>(&clientAdd),&size);
        if (recievedByt > 0) {
            char data[recievedByt - 9];
            Header header1;
            decodeMessage(&header1,data, sizeof(data),message);
            std::cout << "Received from klient: " << data << std::endl;

            char text[] = "Nadviazane spojenie";
            Header header {0b00000001,25,1,1,0};
            char message1[sizeof(text) + sizeof(header)];
            codeMessage(&header,text,sizeof(text),message1);
            sendto(serverS, message1, sizeof(message1), 0,reinterpret_cast<sockaddr*>(&clientAdd), sizeof(clientAdd));

        }



    }

    return 0;
}

void codeMessage(Header * header, char text[], int text_size, char message[]){
    message[0] = (char) header->type;
    message[1] = (char) (0xff & (header->lenght >> 8));
    message[2] = (char) (0xff & header->lenght);
    message[3] = (char) (0xff & (header->numberOfFragments >> 8));
    message[4] = (char) (0xff & header->numberOfFragments);
    message[5] = (char) (0xff & (header->fragmentInSequence >> 8));
    message[6] = (char) (0xff & header->fragmentInSequence);
    message[7] = (char) (0xff & (header->crc >> 8));
    message[8] = (char) (0xff & header->crc);
    int position = 9;
    for (int i = 0; i < text_size ; i++) {
        message[position] = text[i];
        position++;
    }
}

void decodeMessage(Header *header1, char text[], int text_size, char message[]){
    header1->type = message[0];
    header1->lenght = (unsigned char)message[1];
    header1->lenght <<= 8;
    header1->lenght += (unsigned char)message[2];
    header1->numberOfFragments = (unsigned char)message[3];
    header1->numberOfFragments <<= 8;
    header1->numberOfFragments += (unsigned char)message[4];
    header1->fragmentInSequence = (unsigned char)message[5];
    header1->fragmentInSequence <<= 8;
    header1->fragmentInSequence += (unsigned char)message[6];
    header1->crc = (unsigned char)message[7];
    header1->crc <<= 8;
    header1->crc += (unsigned char)message[8];

    int d = 9;
    for(int i = 0 ; i < text_size ; i++){
        text[i] = message[d];
        d++;
    }
}

void send(char message[]){
    if(role == "klient"){
        sendto(clientS, message, sizeof(message), 0,reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress));
    }
    else {
        sendto(serverS, message, sizeof(message), 0,reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress));

    }
}

void receive(){

}
