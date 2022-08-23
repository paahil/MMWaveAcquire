#include <iostream>

int main(){
    int* test = (int*)calloc(100, sizeof(int));
    std::cout << sizeof(test) << "\n";
    unsigned short header = 0xA55A;
    unsigned char bytes[2];
    unsigned short newheader;
    memcpy(bytes, &header, 2);
    printf("Bytes: %X and %X", bytes[0], bytes[1]);
    memcpy(&newheader, bytes, 2);
    printf("Recovered: %X", newheader);
    return 0;
}