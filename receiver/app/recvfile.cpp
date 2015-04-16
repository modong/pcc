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

using namespace std;

#ifndef WIN32
void* sendfile(void*);
#else
DWORD WINAPI sendfile(LPVOID);
#endif

int main(int argc, char* argv[])
{
   if ((3 < argc) || ((2 == argc) && (0 == atoi(argv[1]))))
   {
      cout << "usage: recvfile [server_port] [bind_ip]" << endl;
      return 0;
   }

   // use this function to initialize the UDT library
   UDT::startup();

   addrinfo hints;
   addrinfo* res;

   memset(&hints, 0, sizeof(struct addrinfo));
   hints.ai_flags = AI_PASSIVE;
   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_STREAM;

   string service("9000");
   if (2 == argc)
      service = argv[1];

   char * node = NULL;
   if (3 == argc)
       node = argv[2];
       
   if (0 != getaddrinfo(node, service.c_str(), &hints, &res))
   {
      cout << "illegal port number or port is busy.\n" << endl;
      return 0;
   }

   UDTSOCKET serv = UDT::socket(res->ai_family, res->ai_socktype, res->ai_protocol);

   // Windows UDP issue
   // For better performance, modify HKLM\System\CurrentControlSet\Services\Afd\Parameters\FastSendDatagramThreshold
#ifdef WIN32
   int mss = 1052;
   UDT::setsockopt(serv, 0, UDT_MSS, &mss, sizeof(int));
#endif

   if (UDT::ERROR == UDT::bind(serv, res->ai_addr, res->ai_addrlen))
   {
      cout << "bind: " << UDT::getlasterror().getErrorMessage() << endl;
      return 0;
   }

   freeaddrinfo(res);

   cout << "server is ready at port: " << service << endl;

   UDT::listen(serv, 10);

   sockaddr_storage clientaddr;
   int addrlen = sizeof(clientaddr);

   UDTSOCKET fhandle;

   while (true)
   {
      if (UDT::INVALID_SOCK == (fhandle = UDT::accept(serv, (sockaddr*)&clientaddr, &addrlen)))
      {
         cout << "accept: " << UDT::getlasterror().getErrorMessage() << endl;
         return 0;
      }

      char clienthost[NI_MAXHOST];
      char clientservice[NI_MAXSERV];
      getnameinfo((sockaddr *)&clientaddr, addrlen, clienthost, sizeof(clienthost), clientservice, sizeof(clientservice), NI_NUMERICHOST|NI_NUMERICSERV);
      cout << "new connection: " << clienthost << ":" << clientservice << endl;

      #ifndef WIN32
         pthread_t filethread;
         pthread_create(&filethread, NULL, sendfile, new UDTSOCKET(fhandle));
         pthread_detach(filethread);
      #else
         CreateThread(NULL, 0, sendfile, new UDTSOCKET(fhandle), 0, NULL);
      #endif
   }

   UDT::close(serv);

   // use this function to release the UDT library
   UDT::cleanup();

   return 0;
}

#ifndef WIN32
void* sendfile(void* usocket)
#else
DWORD WINAPI sendfile(LPVOID usocket)
#endif
{
   UDTSOCKET fhandle = *(UDTSOCKET*)usocket;
   delete (UDTSOCKET*)usocket;

   // aquiring file name information from client
   char file[1024]="./file_received";

   fstream ofs(file, ios::out | ios::binary | ios::trunc);

   // get size information
   int64_t size;

   if (UDT::ERROR == UDT::recv(fhandle, (char*)&size, sizeof(int64_t), 0))
   {
      cout << "send: " << UDT::getlasterror().getErrorMessage() << endl;
   }

   if (size < 0)
   {
      cout << "no such file on the server\n";
   }
   cout<<"size is "<<size<<endl;
   int64_t recvsize;
   int64_t offset = 0;

   if (UDT::ERROR == (recvsize = UDT::recvfile(fhandle, ofs, offset, size)))
   {
      cout << "recvfile: " << UDT::getlasterror().getErrorMessage() << endl;
   }
   cout<<"recv size is"<<recvsize<<endl;

   UDT::close(fhandle);
   return NULL;
}
