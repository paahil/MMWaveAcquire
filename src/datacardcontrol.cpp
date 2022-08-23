#include <iostream>
#include <mutex>
#include <string.h>
#include <iomanip>
#include <windows.h>

//GLOBAL VARIABLES
std::timed_mutex contacquire;

//Default ports
int DATAPORT = 4098;
int CONFIGPORT = 4096;

//STATIC DEFINES
namespace PACKET{
    static UINT16 HEADER = 0xA55A;
    static UINT16 FOOTER = 0xEEAA;
    static UINT16 ZERO = 0x0;
};

namespace CONFIGS{
    static UINT16 RAW = 0x01;
    static UINT16 TWOLANE = 0x02;
    static UINT16 BIT16 = 0x03;
};

enum CMDS{
RESET,
CONNECT,
START,
STOP,
CFGFPGA,
CFGPACK
};

//STRUCTS
struct Config{
    UINT8 bitdepth = 0x3; //16-Bit
    UINT8 packetformat = 0x1; //Raw
    UINT8 lanecount = 0x2; //2-Lane
    UINT8 transfertype = 0x1; //Capture
    UINT8 capturetype = 0x2; //Ethernet Stream
    UINT8 timer = 30; //LVDS timeout
    UINT16 packetdelay = 30*1000/8; //Packet delay
    UINT16 maxpacketsize = 1470; //Maximum bytes in a packet
};

struct Packet{
    UINT8* data;
    int size;
};


Packet* GetCMDPacket(CMDS cmd, Config* cnfg = NULL){
    Packet* packet = (Packet*) malloc(sizeof(Packet));
    packet->size = 8;
    UINT8* data;
    UINT16 cmdcommand;
    UINT16 datasize = PACKET::ZERO;
    Config usedconfig;

    if (cnfg){
        usedconfig = *cnfg;
    }

    switch(cmd){
        case CMDS::RESET:
        cmdcommand = 0x01;
        break;

        case CMDS::CONNECT:
        cmdcommand = 0x09;
        break;

        case CMDS::START:
        cmdcommand = 0x05;
        break;

        case CMDS::STOP:
        cmdcommand = 0x06;
        break;

        case CMDS::CFGFPGA:
        cmdcommand = 0x03;
        datasize = 0x06;
        packet->size += datasize;
        data = (UINT8*) malloc(datasize);
        memcpy(data, &usedconfig.packetformat, sizeof(UINT8));
        data += sizeof(UINT8);
        memcpy(data, &usedconfig.lanecount, sizeof(UINT8));
        data += sizeof(UINT8);
        memcpy(data, &usedconfig.transfertype, sizeof(UINT8));
        data += sizeof(UINT8);
        memcpy(data, &usedconfig.capturetype, sizeof(UINT8));
        data += sizeof(UINT8);
        memcpy(data, &usedconfig.bitdepth, sizeof(UINT8));
        data += sizeof(UINT8);
        memcpy(data, &usedconfig.timer, sizeof(UINT8));
        data += sizeof(UINT8);
        data -= datasize;
        break;

        case CMDS::CFGPACK:
        cmdcommand = 0x0B;
        datasize = 0x06;
        packet->size += datasize;
        data = (UINT8*) malloc(datasize);
        memcpy(data, &usedconfig.maxpacketsize, sizeof(UINT16));
        data += sizeof(UINT16);
        memcpy(data, &usedconfig.packetdelay, sizeof(UINT16));
        data += sizeof(UINT16);
        memcpy(data, &PACKET::ZERO, sizeof(UINT16));
        data += sizeof(UINT16);
        data -= datasize;
        break;
    }
    packet->data = (UINT8*) malloc(packet->size);
    memcpy(packet->data,&PACKET::HEADER, sizeof(UINT16));
    packet->data += sizeof(UINT16);
    memcpy(packet->data, &cmdcommand, sizeof(UINT16));
    packet->data += sizeof(UINT16);
    memcpy(packet->data, &datasize, sizeof(UINT16));
    packet->data += sizeof(UINT16);
    if(datasize != PACKET::ZERO){
        memcpy(packet->data, data, datasize);
        packet->data += datasize;
        free(data);
    }
    memcpy(packet->data,&PACKET::FOOTER, sizeof(UINT16));
    packet->data += sizeof(UINT16);
    packet->data = packet->data - packet->size;
    return packet;
}

void DeleteCMDPacket(Packet* packet){
    free(packet->data);
    free(packet);
}

DWORD WINAPI ProcessStream(void* arg){
    SOCKET datasoc = socket(AF_INET, SOCK_DGRAM,
    IPPROTO_UDP);
    struct sockaddr_in recvaddr, bindaddr;
    int recvsize = sizeof(recvaddr);
    int recvbufsize = 0xFFFFFFF;
    char recvbuf[4096];

    //Prepare the sockaddr_in structure 
    bindaddr.sin_family = AF_INET;
    bindaddr.sin_addr.s_addr = inet_addr("0.0.0.0");
    bindaddr.sin_port = htons(DATAPORT);

    // Bind                                                                 
    if (bind(datasoc,
        (struct sockaddr *)&bindaddr,
        sizeof(bindaddr)) < 0)
    {
        return(-1);
    }

    if (setsockopt(datasoc, SOL_SOCKET, SO_RCVBUF,
        (char*)&recvbufsize, recvbufsize) == -1)
    {
        return(-1);
    }
    fd_set rset;
    struct timeval read_timeout;
    read_timeout.tv_sec = 1;
    read_timeout.tv_usec = 0;
    FD_ZERO(&rset);
    FD_SET(datasoc,&rset);
    int pcktcnt = 0;
    FILE* fptr;
    for(;;){
    if (select(datasoc + 1, &rset, NULL, NULL, &read_timeout) > 0){
        int recsize = recvfrom(datasoc, recvbuf, sizeof(recvbuf), 0 , (struct sockaddr *)&recvaddr, &recvsize);
        char* addrstr;
        addrstr = inet_ntoa(recvaddr.sin_addr); 
        std::cout << "Recieved " << recsize << " Bytes from: " << addrstr << ", ";
        int seqnum;
        long long int bytecnt;
        memcpy(&seqnum, recvbuf, 4);
        memcpy(&bytecnt, &recvbuf[4], 6);
        char* fname = (char*) malloc(100);
        snprintf(fname, 99, "meas_%d.txt", pcktcnt);
        fptr = fopen(fname, "w");
        for(int i = 10; i<recsize; i+=2){
            INT16 val;
            memcpy(&val, &recvbuf[i], sizeof(INT16));
            
            fprintf(fptr, "%d\n", val);
        }
        fclose(fptr);
        free(fname);
        pcktcnt++;
    }
    else{
        printf("bwah\n");
    }
    
    if (contacquire.try_lock_for(std::chrono::microseconds(1))){
        contacquire.unlock();
        break;
    }
    }
    printf("%d\n", pcktcnt);
    return 0;
}


//TESTED MAIN

int main(){
    #if defined _WIN32
    WSADATA wsaData;
    if(WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR)
    {
        return (-1);
    }
    #endif

    struct sockaddr_in confaddr, confport, senderaddr;
    int recvbufsize = 0x7FFFFFFF;
    int sendbufsize = 0xFFFFF;
    char recv_buf[8] = {0};
    char acquire = 1;
    contacquire.lock();
    HANDLE datathr = CreateThread(NULL, 0, ProcessStream, NULL, 0, NULL);
    SOCKET confsoc = socket(AF_INET, SOCK_DGRAM,
    IPPROTO_UDP);
    if (confsoc < 0)
    {
        return(-1);
    }
    //Prepare the sockaddr_in structure 
    confaddr.sin_family = AF_INET;
    confaddr.sin_addr.s_addr = inet_addr("192.168.33.180");
    confaddr.sin_port = htons(CONFIGPORT);

    //Prepare the sockaddr_in structure                                    
    confport.sin_family = AF_INET;
    confport.sin_addr.s_addr = inet_addr("0.0.0.0");
    confport.sin_port = htons(CONFIGPORT);


    // Bind                                                                 
    if (bind(confsoc,
        (struct sockaddr *)&confport,
        sizeof(confport)) < 0)
    {
        return(-1);
    }

    if (setsockopt(confsoc, SOL_SOCKET, SO_RCVBUF,
        (char*)&recvbufsize, recvbufsize) == -1)
    {
        return(-1);
    }

    if (setsockopt(confsoc, SOL_SOCKET, SO_SNDBUF,
        (char*)&sendbufsize, sendbufsize) == -1)
    {
        return(-1);
    }
    struct timeval tout;
    tout.tv_sec = 10;
    if (setsockopt(confsoc, SOL_SOCKET, SO_RCVTIMEO,
        (char *) &tout, sizeof(tout)) == -1)
    {
        return(-1);
    }
    int addrsize = sizeof(senderaddr);
    int recsize, sendsize;
    Packet* packet;

    packet = GetCMDPacket(CMDS::RESET);
    sendsize = sendto(confsoc, (char*) packet->data, packet->size, 0, (sockaddr*)&confaddr, sizeof(confaddr));
    printf("%d Bytes Sent...\n", sendsize);
    recsize = recvfrom(confsoc, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *)&senderaddr, &addrsize);
    if (recsize < 0){
        std::cout << "ERROR, " << WSAGetLastError() << "\n";
    }
    else{
        char* addrstr;
        addrstr = inet_ntoa(senderaddr.sin_addr); 
        std::cout << "Recieved " << recsize << " Bytes, from: " << addrstr << "\n";
    }
    DeleteCMDPacket(packet);

    packet = GetCMDPacket(CMDS::CFGFPGA);
    std::cout<<sizeof(packet)<<std::endl;
    sendsize = sendto(confsoc, (char*) packet->data, packet->size, 0, (sockaddr*)&confaddr, sizeof(confaddr));
    printf("%d Bytes Sent...\n", sendsize);
    recsize = recvfrom(confsoc, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *)&senderaddr, &addrsize);
    if (recsize < 0){
        std::cout << "ERROR, " << WSAGetLastError() << "\n";
    }
    else{
        char* addrstr;
        addrstr = inet_ntoa(senderaddr.sin_addr); 
        std::cout << "Recieved " << recsize << " Bytes, from: " << addrstr << "\n";
    }
    DeleteCMDPacket(packet);

    packet = GetCMDPacket(CMDS::CFGPACK);
    sendsize = sendto(confsoc, (char*) packet->data, packet->size, 0, (sockaddr*)&confaddr, sizeof(confaddr));
    printf("%d Bytes Sent...\n", sendsize);
    recsize = recvfrom(confsoc, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *)&senderaddr, &addrsize);
    if (recsize < 0){
        std::cout << "ERROR, " << WSAGetLastError() << "\n";
    }
    else{
        char* addrstr;
        addrstr = inet_ntoa(senderaddr.sin_addr); 
        std::cout << "Recieved " << recsize << " Bytes, from: " << addrstr << "\n";
    }
    DeleteCMDPacket(packet);

    packet = GetCMDPacket(CMDS::CONNECT);
    sendsize = sendto(confsoc, (char*) packet->data, packet->size, 0, (sockaddr*)&confaddr, sizeof(confaddr));
    printf("%d Bytes Sent...\n", sendsize);
    recsize = recvfrom(confsoc, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *)&senderaddr, &addrsize);
    if (recsize < 0){
        std::cout << "ERROR, " << WSAGetLastError() << "\n";
    }
    else{
        char* addrstr;
        addrstr = inet_ntoa(senderaddr.sin_addr); 
        std::cout << "Recieved " << recsize << " Bytes, from: " << addrstr << "\n";
    }
    DeleteCMDPacket(packet);

    packet = GetCMDPacket(CMDS::START);
    sendsize = sendto(confsoc, (char*) packet->data, packet->size, 0, (sockaddr*)&confaddr, sizeof(confaddr));
    printf("%d Bytes Sent...\n", sendsize);
    recsize = recvfrom(confsoc, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *)&senderaddr, &addrsize);
    if (recsize < 0){
        std::cout << "ERROR, " << WSAGetLastError() << "\n";
    }
    else{
        char* addrstr;
        addrstr = inet_ntoa(senderaddr.sin_addr); 
        std::cout << "Recieved " << recsize << " Bytes, from: " << addrstr << "\n";
    }
    DeleteCMDPacket(packet);
    Sleep(1000);
    packet = GetCMDPacket(CMDS::STOP);
    sendsize = sendto(confsoc, (char*) packet->data, packet->size, 0, (sockaddr*)&confaddr, sizeof(confaddr));
    printf("%d Bytes Sent...\n", sendsize);
    recsize = recvfrom(confsoc, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *)&senderaddr, &addrsize);
    if (recsize < 0){
        std::cout << "ERROR, " << WSAGetLastError() << "\n";
    }
    else{
        char* addrstr;
        addrstr = inet_ntoa(senderaddr.sin_addr); 
        std::cout << "Recieved " << recsize << " Bytes, from: " << addrstr << "\n";
    }
    contacquire.unlock();
    DeleteCMDPacket(packet);
    WaitForSingleObject(datathr, 1000);
    if(!CloseHandle(datathr)){
        std::cout << "ERROR Thread running\n";
    }
}