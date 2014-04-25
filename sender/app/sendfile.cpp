#ifndef WIN32
   #include <cstdlib>
   #include <netdb.h>
#else
   #include <winsock2.h>
   #include <ws2tcpip.h>
#endif
#include <fstream>
#include <iostream>
#include <cstring>
#include <udt.h>
#include "cc.h"
using namespace std;

#ifndef WIN32
void* sendfile(void*);
#else
DWORD WINAPI sendfile(LPVOID);
#endif

int main(int argc, char* argv[])
{

   if ((argc != 4) || (0 == atoi(argv[2])))
   {
      cout << "usage: recvfile server_ip server_port file_to_send" << endl;
      return -1;
   }
   UDT::startup();

   struct addrinfo hints, *peer;

   memset(&hints, 0, sizeof(struct addrinfo));
   hints.ai_flags = AI_PASSIVE;
   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_STREAM;

   UDTSOCKET fhandle = UDT::socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);
   UDT::setsockopt(fhandle, 0, UDT_CC, new CCCFactory<BBCC>, sizeof(CCCFactory<BBCC>));

   if (0 != getaddrinfo(argv[1], argv[2], &hints, &peer))
   {
      cout << "incorrect server/peer address. " << argv[1] << ":" << argv[2] << endl;
      return -1;
   }

   // connect to the server, implict bind
   if (UDT::ERROR == UDT::connect(fhandle, peer->ai_addr, peer->ai_addrlen))
   {
      cout << "connect: " << UDT::getlasterror().getErrorMessage() << endl;
      return -1;
   }

   freeaddrinfo(peer);
     // using CC method
   BBCC* cchandle = NULL;
   int temp;
   UDT::getsockopt(fhandle, 0, UDT_CC, &cchandle, &temp);
   if (NULL != cchandle)
      cchandle->setRate(1);
 
   char file[1024];
   strcpy(file,argv[3]);
   fstream ifs(file, ios::in | ios::binary);

   ifs.seekg(0, ios::end);
   int64_t size = ifs.tellg();
   ifs.seekg(0, ios::beg);
   cout<<"size is "<<size<<endl;
   // send file size information
   if (UDT::ERROR == UDT::send(fhandle, (char*)&size, sizeof(int64_t), 0))
   {
      cout << "send: " << UDT::getlasterror().getErrorMessage() << endl;
      return 0;
   }

   UDT::TRACEINFO trace;
   UDT::perfmon(fhandle, &trace);

   // send the file
   int64_t offset = 0;
   int64_t sentsize = 0;

   sentsize = UDT::sendfile(fhandle, ifs, offset, size);
   UDT::perfmon(fhandle, &trace);
   cout<<"sentsize is "<<trace.pktTotalBytes<<endl;
     sleep(5);
   UDT::close(fhandle);

   ifs.close();

   #ifndef WIN32
      return 0;
   #else
      return 0;
   #endif
}
