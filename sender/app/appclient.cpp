#ifndef WIN32
   #include <unistd.h>
   #include <cstdlib>
   #include <cstring>
   #include <netdb.h>
   #include <signal.h>
#else
   #include <winsock2.h>
   #include <ws2tcpip.h>
   #include <wspiapi.h>
#endif
#include <iostream>
#include <udt.h>
#include "cc.h"

using namespace std;

#ifndef WIN32
void* monitor(void*);
#else
DWORD WINAPI monitor(LPVOID);
#endif

// for logging
char* file_name;
std::ofstream logger;
void segfault_sigaction(int signal, siginfo_t *si, void *arg);
void writeToLog(std::ofstream &logFile, int64_t relativeTime, double rate, double rtt,
                     int window, int64_t sent, int loss);

int main(int argc, char* argv[])
{
   // catching segmentation faults
   struct sigaction sa;
   memset(&sa, 0, sizeof(struct sigaction));
   sigemptyset(&sa.sa_mask);
   sa.sa_sigaction = segfault_sigaction;
   sa.sa_flags   = SA_SIGINFO;
   sigaction(SIGSEGV, &sa, NULL);

   if ((4 != argc) || (0 == atoi(argv[2])))
   {
      cout << "usage: appclient server_ip server_port log_file" << endl;
      return 0;
   }
   //sleep(1500);
   // use this function to initialize the UDT library
   UDT::startup();

   struct addrinfo hints, *local, *peer;

   memset(&hints, 0, sizeof(struct addrinfo));

   hints.ai_flags = AI_PASSIVE;
   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_STREAM;
   //hints.ai_socktype = SOCK_DGRAM;

   if (0 != getaddrinfo(NULL, "9000", &hints, &local))
   {
      cout << "incorrect network address.\n" << endl;
      return 0;
   }

   UDTSOCKET client = UDT::socket(local->ai_family, local->ai_socktype, local->ai_protocol);

   // UDT Options
   UDT::setsockopt(client, 0, UDT_CC, new CCCFactory<BBCC>, sizeof(CCCFactory<BBCC>));
   //UDT::setsockopt(client, 0, UDT_MSS, new int(9000), sizeof(int));
   //UDT::setsockopt(client, 0, UDT_SNDBUF, new int(10000000), sizeof(int));
   //UDT::setsockopt(client, 0, UDP_SNDBUF, new int(10000000), sizeof(int));

   // Windows UDP issue
   // For better performance, modify HKLM\System\CurrentControlSet\Services\Afd\Parameters\FastSendDatagramThreshold
   #ifdef WIN32
      UDT::setsockopt(client, 0, UDT_MSS, new int(1052), sizeof(int));
   #endif

   /*
   UDT::setsockopt(client, 0, UDT_RENDEZVOUS, new bool(true), sizeof(bool));
   if (UDT::ERROR == UDT::bind(client, local->ai_addr, local->ai_addrlen))
   {
      cout << "bind: " << UDT::getlasterror().getErrorMessage() << endl;
      return 0;
   }
   */

   freeaddrinfo(local);

   if (0 != getaddrinfo(argv[1], argv[2], &hints, &peer))
   {
      cout << "incorrect server/peer address. " << argv[1] << ":" << argv[2] << endl;
      return 0;
   }
   file_name = argv[3];

   // connect to the server, implict bind
   if (UDT::ERROR == UDT::connect(client, peer->ai_addr, peer->ai_addrlen))
   {
      cout << "connect: " << UDT::getlasterror().getErrorMessage() << endl;
      return 0;
   }
   freeaddrinfo(peer);

   // using CC method
   BBCC* cchandle = NULL;
   int temp;
   UDT::getsockopt(client, 0, UDT_CC, &cchandle, &temp);
//   if (NULL != cchandle)
//      cchandle->setRate(1);

   int size = 100000;
   char* data = new char[size];

   #ifndef WIN32
      pthread_create(new pthread_t, NULL, monitor, &client);
   #else
      CreateThread(NULL, 0, monitor, &client, 0, NULL);
   #endif

   for (int i = 0; i < 1000000; i ++)
   {
      int ssize = 0;
      int ss;
      while (ssize < size)
      {
         if (UDT::ERROR == (ss = UDT::send(client, data + ssize, size - ssize, 0)))
         {
            cout << "send:" << UDT::getlasterror().getErrorMessage() << endl;
            break;
         }

         ssize += ss;
      }

      if (ssize < size)
         break;
   }
   logger.close();
   UDT::close(client);

   delete [] data;

   // use this function to release the UDT library
   UDT::cleanup();

   return 1;
}

#ifndef WIN32
void* monitor(void* s)
#else
DWORD WINAPI monitor(LPVOID s)
#endif
{
   logger.open(file_name);
    logger << "time," << "rate," << "rtt," << "pktsflight," << "total," << "tloss\n";
   UDTSOCKET u = *(UDTSOCKET*)s;

   UDT::TRACEINFO perf;
   int i=0;
   while (true)
   {
      #ifndef WIN32
         usleep(10000);
      #else
         Sleep(1000);
      #endif
      i++;
      if(i > 10000)
      {
        exit(1); 
      }
      if (UDT::ERROR == UDT::perfmon(u, &perf))
      {
         cout << "perfmon: " << UDT::getlasterror().getErrorMessage() << endl;
         break;
      }
      writeToLog(logger, perf.msTimeStamp, perf.mbpsSendRate, perf.msRTT,
                           perf.pktFlightSize, perf.pktSentTotal, perf.pktSndLossTotal);

   }

   #ifndef WIN32
      return NULL;
   #else
      return 0;
   #endif
}


void writeToLog(std::ofstream &logFile, int64_t relativeTime, double rate, double rtt,
                     int window, int64_t sent, int loss)
{
   logFile << relativeTime << "," << rate;
   logFile << "," << rtt;
   logFile << "," << window;
   logFile << "," << sent;
   logFile << "," << loss;
   logFile << "\n";

   return;
}

void segfault_sigaction(int signal, siginfo_t *si, void *arg)
{
   logger.close();
   exit(0);
}