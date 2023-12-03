#include <iostream>
#include <winsock2.h>
#include <string>
#include <thread>
#include <windows.h>
#include <fstream>

/**
 * Stiahnutá knižnica z github.com
 * URL: https://github.com/d-bahr/CRCpp.git
*/
#include "CRC.h"

using namespace std;
static string role;
static int dataLenght = 1400;
static int listening_port = 20000;
static bool endConnection = false,changedRoles = false, sendData = false, receiveData = false;

static SOCKADDR_IN serverAddress;
static sockaddr_in clientAdd, serverAdd;
static SOCKET clientS, serverS;

static time_t start;

struct Header {
    unsigned char type;
    unsigned short lenght;
    unsigned short numberOfFragments;
    unsigned short fragmentInSequence;
    unsigned short crc;
};

void codeMessage(Header * header, char text[], int text_size, char message[]);
void decodeMessage(Header *header1, char text[], int text_size, char message[]);
void receiveM(bool * rec, bool * connection, bool * keepalive, bool *recievFr, bool  *changeRole, bool *correctData);
void sendM(bool * rec, bool *connection, bool *keepalive, bool *recievFr, bool  *changeRole,  bool *correctData);
void changeRoleTo(string newRole, bool *rec , bool *connection, bool *keepalive , bool *recievFr, bool *changeRole , bool * correctData);
string toBinary(int number);

int main() {


    bool correctRole = false;
    cout << "Zadaj ci chces figurovat ako server alebo klient" << endl;
    cin >> role;
    while(!correctRole){
        if(strcmpi(role.c_str(),"klient") == 0){
            cout << "ROLA KLIENTA " << endl;
            role = "klient";
            correctRole = true;
        }
        else if(strcmpi(role.c_str(),"server") == 0){
            cout << "ROLA SERVERA" << endl;
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
        serverS = clientS;
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

        bool rec = true, connection = false , keepalive = true, recievFr = false, changeRole = false ,correctData = false;
        thread t1(sendM, &rec ,&connection, &keepalive, &recievFr, &changeRole , &correctData);
        thread t2(receiveM,&rec, &connection, &keepalive, &recievFr, &changeRole, &correctData);
        t1.join();
        t2.join();
//        while (!endConnection) {
//            if(changedRoles) {
//                changedRoles = false;
//                if(role == "klient"){
//                    role = "server";
//                }
//                else role = "klient";
//
////                if(!rec) rec = true;
////                else rec = false;
//            }
//
//        }


    }
    else if(role == "server"){
        cout << "Zadaj port na akom budes pocuvat" << endl;
        cin >> listening_port;
        // Vytvorenie socketu
        serverS = socket(AF_INET,SOCK_DGRAM, 0);
        clientS = serverS;
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
        bool rec = false, connection = false, keepalive = true, recievFr = false, changeRole = false, correctData = false;
        thread t1(sendM, &rec, &connection, &keepalive, &recievFr, &changeRole , &correctData);
        thread t2(receiveM, &rec, &connection, &keepalive, &recievFr, &changeRole , &correctData);

        t1.join();
        t2.join();
//        while (!endConnection) {
//            cout << "hladam sa";
//            if(changedRoles) {
//
//                changedRoles = false;
////                if(role == "klient"){
////                    role = "server";
////                }
////                else role = "klient";
//
////                if(!rec) rec = true;
////                else rec = false;
//            }
//
//
//
//        }
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

void sendM(bool * rec, bool * connection, bool *keepalive, bool *recievFr , bool *changeRole , bool *correctData){
    if(role == "klient"){
        int i = 0;
        while(*keepalive && !changedRoles){

            if((time(nullptr)-start) >= 5 && *connection && i < 3){
                i++;
                char text[] = "Posielam keep alive";
                Header header{0b001000000, sizeof(text) + 9, 1, 1, 0};
                char message[sizeof(text) + sizeof(header)];

                codeMessage(&header, text, sizeof(text), message);
                sendto(clientS, message, sizeof(message), 0, reinterpret_cast<sockaddr *>(&serverAddress),
                       sizeof(serverAddress));
                *rec = false;
                start = time(nullptr);
            }
            if(*rec && !*connection) {
                char text[] = "Chcem nadviazat spojenie";
                Header header{0b000000001, sizeof(text) + 9, 1, 1, 0};
                char message[sizeof(text) + sizeof(header)];

                codeMessage(&header, text, sizeof(text), message);
                sendto(clientS, message, sizeof(message), 0, reinterpret_cast<sockaddr *>(&serverAddress),
                       sizeof(serverAddress));
                *rec = false;
                start = time(nullptr);
            }
            if(*rec && *connection){
                int choice;
                cout << "rola " << role;
                cout << "Vyber co chces spravit :" << endl << "- poslat spravu(1)" <<endl << "- poslat subor(2)?" <<endl << "- inicializovat vymenu roli(3)" << endl << "ukoncenie spojenia(4) << endl";
                cin >> choice;
                if(choice != 1 && choice != 2 && choice != 3 && choice !=4){
                    while(choice != 1 && choice != 2 && choice != 3 && choice != 4){
                        cout << "Zadal si nespravny typ, pre spravu napis 1 , pre subor 2 , vymena roli 3 , ukoncenie spojenia 4";
                        cin >> choice;
                    }
                }
                // ideme posielat spravu
                if(choice == 1){
                    string message;
                    int fragmentSize;
                    this_thread::sleep_for(10ms);
                    std::cin.clear();
                    std::cin.sync();
                    cout << "Teraz napis prosim ta spravu (moznost cez viacej riadkov, pre ukoncenie napis znak #) ";
                    cin.clear();
                    getline(cin,message,'#');
                    int text_size = message.size();
//                    message.append("\0");
                    cout << "Zadaj velkost fragmentu (max 1462B)" << endl;
                    cin >> fragmentSize;
                    if(fragmentSize > 1462){
                        while(fragmentSize > 1462){
                            cout << "Zadal si prilis velku velkost fragmentu zvol (do 1462)";
                            cin >> fragmentSize;
                        }
                    }
                    char text[message.size()+1];
                    for(int i = 0; i < message.size(); i++){
                        text[i] = message[i];
                    }
                    text[message.size()] = '\0';

                    if(message.size() > fragmentSize){
                        int number;
                        if((message.size()%fragmentSize == 0)){
                            number = ((int)message.size()/fragmentSize);
                        }
                        else number = (((int)message.size()/fragmentSize) + 1);

                        int a = 0;
                        char toSend[fragmentSize + 1];
                        for(int i = 0; i < fragmentSize ; i++){
                            toSend[i] = text[i];
                        }
                        toSend[fragmentSize] = '\0';

                        uint16_t crc = CRC::Calculate(toSend, sizeof(toSend),CRC::CRC_16_ARC());
                        cout <<  "crc " <<crc << " " << sizeof(toSend) << endl;
                        Header header{0b000000100, static_cast<unsigned short>(sizeof(toSend) + 9), static_cast<unsigned short>(number), 1, crc};
                        char message[sizeof(toSend) + sizeof(header)];
                        codeMessage(&header, toSend, sizeof(toSend), message);
                        sendto(clientS, message, sizeof(message), 0, reinterpret_cast<sockaddr *>(&serverAddress),
                               sizeof(serverAddress));
                        *rec = false;
                        *recievFr = false;
                        start = time(nullptr);
                        a++;

                        while(a < number){

                            if(*recievFr){
                                char toSend[fragmentSize + 1];
                                for(int i = (a*fragmentSize); i < ((a+1)*fragmentSize) ; i++){
                                    toSend[i - a*fragmentSize] = text[i];
                                }
                                toSend[fragmentSize] = '\0';
                                a++;
                                int s;

                                if( text_size > fragmentSize) s = fragmentSize;
                                else s = text_size;
                                text_size -= fragmentSize;
                                cout << "text size " << text_size << "fragme " << fragmentSize;



                                cout <<  "crc " <<crc << " " << sizeof(toSend) << " s " <<s << endl;
                                if(s < fragmentSize){
                                    char sendLast[s+1];
                                    for (int i = 0 ; i < s ; i++){
                                        sendLast[i] = toSend[i];
                                    }
                                    sendLast[s] =  '\0';
                                    uint16_t crc = CRC::Calculate(sendLast, s + 1,CRC::CRC_16_ARC());
                                    Header header{0b000000100, static_cast<unsigned short>(s + 9), static_cast<unsigned short>(number),static_cast<unsigned short>(a) , crc};
                                    char message[sizeof(sendLast) + sizeof(header)];
                                    codeMessage(&header, sendLast, sizeof(sendLast), message);
                                    sendto(clientS, message, sizeof(message), 0, reinterpret_cast<sockaddr *>(&serverAddress),
                                           sizeof(serverAddress));
                                    *rec = false;
                                    *recievFr = false;
                                    start = time(nullptr);
                                }
                                else {
                                    uint16_t crc = CRC::Calculate(toSend, s + 1,CRC::CRC_16_ARC());
                                    Header header{0b000000100, static_cast<unsigned short>(s + 9), static_cast<unsigned short>(number),static_cast<unsigned short>(a) , crc};
                                    char message[sizeof(toSend) + sizeof(header)];
                                    codeMessage(&header, toSend, s, message);
                                    sendto(clientS, message, sizeof(message), 0, reinterpret_cast<sockaddr *>(&serverAddress),
                                           sizeof(serverAddress));
                                    *rec = false;
                                    *recievFr = false;
                                    start = time(nullptr);
                                }


                            }
                        }



                    }
                    else {
                        //char text[] = "Posielam celu textovu spravu";
                        uint16_t crc = CRC::Calculate(text, sizeof(text),CRC::CRC_16_ARC());
                        cout <<  "crc " <<crc << " " << sizeof(text) << endl;
                        Header header{0b000000100, static_cast<unsigned short>(sizeof(text) + 9), 1, 1, crc};
                        char message[sizeof(text) + sizeof(header)];

                        codeMessage(&header, text, sizeof(text), message);
                        sendto(clientS, message, sizeof(message), 0, reinterpret_cast<sockaddr *>(&serverAddress),
                               sizeof(serverAddress));
                        *rec = false;
                        start = time(nullptr);
                    }
                    //cout << message << endl << message.size();
                }
                // posielanie suboru
                else if(choice == 2){
                    cout << "Zadaj cestu/nazov suboru" << endl;
                    string path;
                    cin >> path;
                    char file[path.size() + 1];
                    strcpy(file, path.c_str());

                    ifstream send_file;
                    send_file.open (file, ios::in | ios :: binary);
                    send_file.seekg(0, ios::end);
                    long long file_size = send_file.tellg();
                    send_file.seekg(0, ios::beg);
                    cout << "FILE SIZE " << file_size;
                    // citanie suboru
                    char file_data[file_size + 1];
                    for(int i = 0 ; i < file_size; i++){
                        file_data[i] = send_file.get();
                    }
                    send_file.close();
                    file_data[file_size] = '\0';

                    int fragmentSize;
                    cout << "Zadaj velkost fragmentu (max 1462B)" << endl;
                    cin >> fragmentSize;
                    if(fragmentSize > 1462){
                        while(fragmentSize > 1462){
                            cout << "Zadal si prilis velku velkost fragmentu zvol (do 1462)";
                            cin >> fragmentSize;
                        }
                    }
                    if(file_size> fragmentSize){
                        int number;
                        int rest = 0;
                        if((file_size%fragmentSize == 0)){
                            number = ((int)file_size/fragmentSize);
                        }
                        else {
                            number = (((int)file_size/fragmentSize) + 1);
                            rest = ((number - 1 )*fragmentSize)-file_size;
                            cout << "REST " << rest;
                        }

                        int a = 0;
//                        char toSend[fragmentSize];
//                        for(int i = 0; i < fragmentSize ; i++){
//                            toSend[i] = text[i];
//                        }

                        // posielanie nazvu suboru
                        uint16_t crc = CRC::Calculate(file, sizeof(file),CRC::CRC_16_ARC());
                        Header header{0b00000100, static_cast<unsigned short>(sizeof(file) + 9), static_cast<unsigned short>(number), 0, crc};
                        char message[sizeof(file) + sizeof(header)];
                        codeMessage(&header, file, sizeof(file), message);
                        sendto(clientS, message, sizeof(message), 0, reinterpret_cast<sockaddr *>(&serverAddress),
                               sizeof(serverAddress));
                        *rec = false;
                        *recievFr = false;
                        start = time(nullptr);
//                        a++;

                        while(a < number){

                            if(*recievFr){
                                char toSend[fragmentSize + 1];

                                for(int i = (a*fragmentSize); i < ((a+1)*fragmentSize) ; i++){
                                    toSend[i - a*fragmentSize] = file_data[i];

                                }
                                toSend[fragmentSize] = '\0';
                                a++;

                                int s;

                                if(file_size > fragmentSize) s = fragmentSize;
                                else s = file_size;
                                file_size -= fragmentSize;
//                                if(rest != 0 && (a == (number-1))){
//                                    cout <<"prvq";
//                                    Header header{0b000000100, static_cast<unsigned short>(rest + 10), static_cast<unsigned short>(number),static_cast<unsigned short>(a) , 0};
//                                    char message[rest + sizeof(header) +1 ];
//                                    codeMessage(&header, toSend, rest + 1, message);
//                                    sendto(clientS, message, sizeof(message), 0, reinterpret_cast<sockaddr *>(&serverAddress),
//                                           sizeof(serverAddress));
//                                    *rec = false;
//                                    *recievFr = false;
//                                    start = time(nullptr);
//                                }
//                                else {
//                                    cout << a;


                                if(s < fragmentSize){
                                    char sendLast[s+1];
                                    for (int i = 0 ; i < s ; i++){
                                        sendLast[i] = toSend[i];
                                    }
                                    sendLast[s] =  '\0';
                                    uint16_t crc = CRC::Calculate(sendLast, s + 1 ,CRC::CRC_16_ARC());
                                    Header header{0b000000100, static_cast<unsigned short>(s + 9), static_cast<unsigned short>(number),static_cast<unsigned short>(a) , crc};
                                    char message[sizeof(sendLast) + sizeof(header)];
                                    codeMessage(&header, sendLast, sizeof(sendLast), message);
                                    sendto(clientS, message, sizeof(message), 0, reinterpret_cast<sockaddr *>(&serverAddress),
                                           sizeof(serverAddress));
                                    *rec = false;
                                    *recievFr = false;
                                    start = time(nullptr);
                                }
                                else {
                                    uint16_t crc = CRC::Calculate(toSend, s + 1 ,CRC::CRC_16_ARC());
                                    Header header{0b000000100, static_cast<unsigned short>(s + 9), static_cast<unsigned short>(number),static_cast<unsigned short>(a) , crc};
                                    char message[sizeof(toSend) + sizeof(header)];
                                    codeMessage(&header, toSend, s, message);
                                    sendto(clientS, message, sizeof(message), 0, reinterpret_cast<sockaddr *>(&serverAddress),
                                           sizeof(serverAddress));
                                    *rec = false;
                                    *recievFr = false;
                                    start = time(nullptr);
                                }

//                                    uint16_t crc = CRC::Calculate(toSend, s,CRC::CRC_16_ARC());
//                                    Header header{0b000000100, static_cast<unsigned short>(s + 9), static_cast<unsigned short>(number),static_cast<unsigned short>(a) , crc};
//                                    char message[sizeof(toSend) + sizeof(header)];
//                                    codeMessage(&header, toSend, sizeof(toSend), message);
//                                    sendto(clientS, message, sizeof(message), 0, reinterpret_cast<sockaddr *>(&serverAddress),
//                                           sizeof(serverAddress));
//                                    *rec = false;
//                                    *recievFr = false;
//                                    start = time(nullptr);
//                                }

//                                sendto(clientS, message, sizeof(message), 0, reinterpret_cast<sockaddr *>(&serverAddress),
//                                       sizeof(serverAddress));
//                                *rec = false;
//                                *recievFr = false;
//                                start = time(nullptr);
                            }
                        }



                    }
                    else {
                        uint16_t crc = CRC::Calculate(file, sizeof(file),CRC::CRC_16_ARC());
                        Header header{0b00000100, static_cast<unsigned short>(sizeof(file) + 9), 1, 0, crc};
                        char message[sizeof(file) + sizeof(header)];
                        codeMessage(&header, file, sizeof(file), message);
                        sendto(clientS, message, sizeof(message), 0, reinterpret_cast<sockaddr *>(&serverAddress),
                               sizeof(serverAddress));
                        *rec = false;
                        *recievFr = false;
                        start = time(nullptr);
                        int a = 0;

                        while(a != 1){
                            if(*recievFr){
                                //char text[] = "Posielam cely subor";
                                uint16_t crc = CRC::Calculate(file, sizeof(file),CRC::CRC_16_ARC());
                                Header header{0b000000100, static_cast<unsigned short>(sizeof(file_data) + 9), 1, 1, crc};
                                char message[sizeof(file_data) + sizeof(header)];

                                codeMessage(&header, file_data, sizeof(file_data), message);
                                sendto(clientS, message, sizeof(message), 0, reinterpret_cast<sockaddr *>(&serverAddress),
                                       sizeof(serverAddress));
                                *rec = false;
                                start = time(nullptr);
                                a++;
                            }
                        }



                    }




                }
                // vymena roli
                else if(choice == 3){
                    char text[] = "Chcel by som si vymenit rolu ";
                    Header header{0b10000000, static_cast<unsigned short>(sizeof(text) + 9), 1, 1, 0};
                    char message[sizeof(text) + sizeof(header)];

                    codeMessage(&header, text, sizeof(text), message);
                    sendto(clientS, message, sizeof(message), 0, reinterpret_cast<sockaddr *>(&serverAddress),
                           sizeof(serverAddress));
                    *rec = false;
                    start = time(nullptr);
                    *changeRole = true;
                    cout << "rola " << role;
                }
                // ukoncenie spojenia
                else if(choice == 4){

                }
                start = time(nullptr);
            }
        }
//        if (changedRoles) {
//            sendM(rec,connection,keepalive,recievFr,changeRole);
//            changedRoles = false;
//        }
    }
    else {
        while(*keepalive && !changedRoles){
            if(*rec && *connection && !*recievFr && !*changeRole){
                char text[] = "Nadviazane spojenie";
                Header header {0b00000010,sizeof(text) + 9,1,1,0};
                char message[sizeof(text) + sizeof(header)];
                codeMessage(&header,text,sizeof(text),message);
                sendto(serverS, message, sizeof(message), 0,reinterpret_cast<sockaddr*>(&clientAdd), sizeof(clientAdd));
                *rec = false;
                start = time(nullptr);
            }

            else if(*rec && *recievFr && *correctData){
                char text[] = "Spravny fragment";
                Header header {0b00010000,sizeof(text) + 9,1,1,0};
                char message[sizeof(text) + sizeof(header)];
                codeMessage(&header,text,sizeof(text),message);
                sendto(serverS, message, sizeof(message), 0,reinterpret_cast<sockaddr*>(&clientAdd), sizeof(clientAdd));
                *rec = false;
                //*recievFr = false;
                start = time(nullptr);
            }
            else if(*rec && *recievFr && !*correctData){
                char text[] = "Nespravny fragment";
                Header header {0b00001000,sizeof(text) + 9,1,1,0};
                char message[sizeof(text) + sizeof(header)];
                codeMessage(&header,text,sizeof(text),message);
                sendto(serverS, message, sizeof(message), 0,reinterpret_cast<sockaddr*>(&clientAdd), sizeof(clientAdd));
                *rec = false;
                //*recievFr = false;
                start = time(nullptr);
            }
            else if(*rec && *recievFr){
                char text[] = "Dostal som data";
                Header header {0b00000010,sizeof(text) + 9,1,1,0};
                char message[sizeof(text) + sizeof(header)];
                codeMessage(&header,text,sizeof(text),message);
                sendto(serverS, message, sizeof(message), 0,reinterpret_cast<sockaddr*>(&clientAdd), sizeof(clientAdd));
                *rec = false;
                //*recievFr = false;
                start = time(nullptr);
            }

            else if(*connection && *rec && *changeRole){
                cout <<"ack";
                char text[] = "ACK na vymenu roli";
                Header header {0b00000010,sizeof(text) + 9,1,1,0};
                char message[sizeof(text) + sizeof(header)];
                codeMessage(&header,text,sizeof(text),message);
                sendto(serverS, message, sizeof(message), 0,reinterpret_cast<sockaddr*>(&clientAdd), sizeof(clientAdd));
                //*rec = false;
                //*recievFr = false;
                start = time(nullptr);
                role = "klient";
                serverAdd = clientAdd;
                cout << "rola " << role;
                *changeRole = false;
                //changedRoles= false;
               // this_thread::sleep_for(1000ms);
                changeRoleTo("klient",rec,connection,keepalive,recievFr,changeRole,correctData);
                //sendM(rec,connection,keepalive,recievFr,changeRole);
            }
            //cout << "huhuhuhuhuhuh" << endl;
//            cout << start <<endl << time(0) <<endl << (time(0)-start)<<endl;
//            time(0);
            this_thread::sleep_for(10ms);
            time_t d = (time(nullptr)-start);
            if((d) > 120 && *connection){
                *keepalive = false;
            }
        }

        if(!changedRoles){
            cout << "end connecrion";
            char text[] = "Ukoncenie spojenia";
            Header header {0b01000000,sizeof(text) + 9,1,1,0};
            char message[sizeof(text) + sizeof(header)];
            codeMessage(&header,text,sizeof(text),message);
            sendto(serverS, message, sizeof(message), 0,reinterpret_cast<sockaddr*>(&clientAdd), sizeof(clientAdd));
            //*rec = false;
            closesocket(serverS);
            closesocket(clientS);
            WSACleanup();
            endConnection = true;
            return;
        }


    }
}

void receiveM(bool * rec, bool * connection, bool *keepalive ,bool *recievFr , bool  *changeRole ,  bool *correctData){
    if(role == "klient"){
        while(*keepalive && !changedRoles){
            char buffer[1500];
            int size = sizeof(serverAdd);

            int bytesReceived = recvfrom(clientS, buffer, sizeof(buffer), 0, reinterpret_cast<SOCKADDR *>(&serverAdd),&size);
            if (bytesReceived > 0) {
                char data[bytesReceived - 9];
                Header header1;
                decodeMessage(&header1,data, sizeof(data),buffer);
                if(toBinary((int)header1.type) == "00000010" && !*connection){
                    *connection = true;
                    *rec = true;
                    cout << "Received from server: " << data << endl;
                }
                // ukoncenie spojenia
                else if(toBinary((int)header1.type) == "01000000" && *connection){
                    *keepalive = false;
                    *rec = true;
                    cout << "Received from server: " << data << endl;
                }
                else if(toBinary((int)header1.type) == "00000010" && *connection && !*changeRole){
                    if(!*recievFr) *recievFr = true;
                    *rec = true;
                    cout << "Received from server: " << data << header1.fragmentInSequence << "/" << header1.numberOfFragments << endl;
                }
                else if(toBinary((int)header1.type) == "00010000" && *connection && !*changeRole){
                    if(!*recievFr) *recievFr = true;
                    *rec = true;
                    cout << "Received from server: " << data << header1.fragmentInSequence << "/" << header1.numberOfFragments << endl;
                }
                else if(toBinary((int)header1.type) == "00001000" && *connection && !*changeRole){
                    if(!*recievFr) *recievFr = true;
                    *rec = true;
                    cout << "Received from server: " << data << header1.fragmentInSequence << "/" << header1.numberOfFragments << endl;
                }
                //*rec = true;
                else if(toBinary((int)header1.type) == "00000010" && *connection && *changeRole){
                    cout << "mozeme zo spravit";
                   // role = "server";
                    *changeRole = false;
                    //changedRoles = false;
                    clientAdd = serverAdd;
                    *connection = false;
                    *rec = false;
                    changeRoleTo("server",rec,connection,keepalive,recievFr,changeRole,correctData);
                   // receiveM(rec,connection,keepalive,recievFr,changeRole);
                }
                cout << "Received from server: " << data << endl;

            }
        }

        if(!changedRoles){
            closesocket(serverS);
            closesocket(clientS);
            WSACleanup();
            return;
        }

    }
    else {
        //start = time(0);
        char file_name1[500];
        string textMess,file_data;
        boolean file = false;
        int sizeOfFile = 0;
        while(*keepalive && !changedRoles){

            char message[1500];
            int size = sizeof(clientAdd);
            string dat ;

            int recievedByt = recvfrom(serverS, message, sizeof(message), 0, reinterpret_cast<SOCKADDR *>(&clientAdd),&size);

            if (recievedByt > 0) {
                char data[recievedByt - 9];
                Header header1;

                decodeMessage(&header1,data, sizeof(data)-1,message);
                uint16_t crc = CRC::Calculate(data, sizeof(data)-1,CRC::CRC_16_ARC());
                cout << "crc " << crc << " " << header1.crc <<" s " << sizeof(data);
                if(toBinary((int)header1.type) == "00000001"){
                    *connection = true;
                    start = time(nullptr);
                    std::cout << "Received from klient: " << data << std::endl;
                }
                else if(toBinary((int)header1.type) == "00000100"){
                    //*connection = true;
                    crc += 1;
                    if(crc == header1.crc) {
                        *correctData = true;
                        cout << "spravny fragment";
                    }else{
                        *correctData = false;
                        cout <<  "nespravny fragment";
                    }
                    if(header1.fragmentInSequence == 0){
                        file = true;
                        strcpy(file_name1,data);
                        cout << "Received file name from klient: " << data << endl;
                    }
                    if(file){
                        if(header1.fragmentInSequence > 0){
                           // cout << header1.lenght << endl;
                            file_data.append(data, (header1.lenght - 9));
                            cout << "Prijaty fragment " << header1.fragmentInSequence << "/" <<header1.numberOfFragments << endl;
                            sizeOfFile += (header1.lenght - 9);
                        }


                        if(header1.fragmentInSequence == header1.numberOfFragments){
                            ofstream writeFile;
                           // sizeOfFile += header1.lenght;
                            string path;
                            cout << "Zadaj kam chces ulozit subor" << endl;
                            cin >>  path;
                            path.append(file_name1);
                            writeFile.open(path, ios::binary | ios::out);
                            const char* str = file_data.c_str();
                            writeFile.write(str,file_data.size());

                            cout << "Received file from klient s nazvom " << file_name1  << " s velkostou "<< file_data.size() << " " <<  sizeOfFile<< endl;
                            file_data.clear();
                            sizeOfFile = 0;
                            file = false;
                        }
                    }
                    else {
                        textMess.append(data);

                        if(header1.fragmentInSequence == header1.numberOfFragments){
                            cout << "Received MESSAGE from klient: " << textMess << endl;
                            textMess.clear();
                        }
                    }
                    start = time(nullptr);
                    *recievFr =  true;


                }
                else if(toBinary((int)header1.type) == "10000000"){
                    //*connection = true;
                    string choice;
                    cout << "Received message from klient: "<< data << endl;
//                    cin >> choice;
//                    if(choice == "ano"){
//                        *changeRole = true;
//                    }
                    *changeRole = true;
                    //changeRoleTo("klient",rec,connection,keepalive,recievFr,changeRole,correctData);
                    start = time(nullptr);
                   // *recievFr =  true;
                }
                *rec = true;
                start = time(nullptr);
            }
        }

//        if (changedRoles){
//            *changeRole = false;
//            changedRoles = false;
//            receiveM(rec, connection, keepalive, recievFr, changeRole);
//        }
    }
}

string toBinary(int number){
    string binary;
    while(number != 0){
        if(number%2 == 0){
            binary ="0" + binary;
        }
        else binary ="1" + binary;
        number /= 2;
    }
    if(binary.size() != 8){
        while(binary.size() != 8){
            binary = "0" + binary;
        }
    }
    return binary;

}

void changeRoleTo(string newRole, bool *rec , bool *connection, bool *keepalive , bool *recievFr, bool *changeRole , bool *correctData){
    *changeRole = false;
    if (newRole == "klient") {
        // Zatvorenie existujúceho spojenia (ak existuje)
        closesocket(clientS);

        // Vytvorenie nového soketu pre klienta
        clientS = socket(AF_INET, SOCK_DGRAM, 0);

        // Nastavenie adresy a portu servera, ku ktorému sa chce pripojiť
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(listening_port);
        string add = "192.168.1.15";
        serverAddress.sin_addr.s_addr = inet_addr(add.c_str());

        // Pripojenie k serveru
        if (connect(clientS, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
            std::cerr << "Nepodarilo sa pripojiť na server" << std::endl;
            closesocket(clientS);
            WSACleanup();
            exit (1);
        }


         //Nastavenie role
        role = "klient";
        thread t1(sendM, rec ,connection, keepalive, recievFr, changeRole,correctData);
        thread t2(receiveM, rec, connection, keepalive, recievFr, changeRole, correctData);
        t1.join();
        t2.join();
//        // Spustenie vlákna na odosielanie a prijímanie správ
//        thread t1(sendM,&rec);
//        thread t2(receiveM, ...);
//        t1.join();
//        t2.join();
    }
    else{
        // Zatvorenie existujúceho spojenia (ak existuje)
        closesocket(serverS);

        // Vytvorenie nového soketu pre server
        serverS = socket(AF_INET, SOCK_DGRAM, 0);

        // Nastavenie adresy a portu, na ktorom server bude naslúchať
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(listening_port);
        serverAddress.sin_addr.s_addr = INADDR_ANY;

        // Pripojenie soketu k určenej adrese a porte
        if (bind(serverS, reinterpret_cast<SOCKADDR*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
            std::cerr << "Nepodarilo sa nastaviť soket!" << std::endl;
            closesocket(serverS);
            WSACleanup();
            exit(1);
        }

         //Nastavenie role
        role = "server";
        thread t1(sendM, rec ,connection, keepalive, recievFr, changeRole, correctData);
        thread t2(receiveM, rec, connection, keepalive, recievFr, changeRole, correctData);
        t1.join();
        t2.join();
    }
}
