#ifndef WIN32
   #include <unistd.h>
   #include <cstdlib>
   #include <cstring>
   #include <netdb.h>
   #include <time.h>
   #include <math.h>

#else
   #include <winsock2.h>
   #include <ws2tcpip.h>
   #include <wspiapi.h>
#endif
#include <iostream>
#include <udt.h>
#include "cc_incast.h"
#include <sys/time.h>

using namespace std;
int windowsize;
char argv1[50],argv2[50];
int Interval_avg;
int tt;
int trap;
int safe;
#ifndef WIN32
void* monitor(void*);
void* worker(void*);
#else
DWORD WINAPI monitor(LPVOID);
#endif
int count=0;
double avg_fluc;
struct timeval current;
int start_s, start_us;
int data_length;
int main(int argc, char* argv[])
{
   //sendinginterval=0;
   start_s = atoi(argv[4]);
   start_us = atoi(argv[5]);
   pthread_t t;
   tt=0;
   Interval_avg=50;
   avg_fluc=0.25;
strcpy(argv1,argv[1]);
strcpy(argv2,argv[2]);
data_length = atoi(argv[3]);
   // use this function to initialize the UDT library
int i=0;
while(i<1){
  i++;
  pthread_create(&t, NULL, worker, NULL);
  pthread_join(t,NULL);

}
   return 1;
}
void * worker(void * s)
{
cout<<endl;
srand((unsigned)time(NULL));
windowsize=data_length;//rand()%900+100;


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

   // for rendezvous connection, enable the code below
   /*
   UDT::setsockopt(client, 0, UDT_RENDEZVOUS, new bool(true), sizeof(bool));
   if (UDT::ERROR == UDT::bind(client, local->ai_addr, local->ai_addrlen))
   {
      cout << "bind: " << UDT::getlasterror().getErrorMessage() << endl;
      return 0;
   }
   */

   freeaddrinfo(local);

   if (0 != getaddrinfo(argv1, argv2, &hints, &peer))
   {
      cout << "incorrect server/peer address. " << argv1 << ":" << argv2 << endl;
      return 0;
   }


   // connect to the server, implict bind
   if (UDT::ERROR == UDT::connect(client, peer->ai_addr, peer->ai_addrlen))
   {
      cout << "connect: " << UDT::getlasterror().getErrorMessage() << endl;
      return 0;
   }

;
   freeaddrinfo(peer);

   // using CC method
   BBCC* cchandle = NULL;
   int temp;
   UDT::getsockopt(client, 0, UDT_CC, &cchandle, &temp);
 //   if (NULL != cchandle)
 //     cout<<"notNULL!"<<endl;
 //   cchandle->setRate(5);
//cchandle->m_dCWndSize=windowsize+100;
//cout<<cchandle->m_dCWndSize<<endl;


//cout<<"k"<<cchandle->m_dCWndSize<<endl;
   int usize = 1456;
   int size=0;





	   size=usize*windowsize;
   //cout<<"size"<<size<<endl;
char* data = new char[size];

   #ifndef WIN32

  // count++;
//cout<<count<<endl;
   #else
      CreateThread(NULL, 0, monitor, &client, 0, NULL);
   #endif





      int ssize = 0;
      int ss=0;
	do {
		gettimeofday(&current, NULL);
		if (current.tv_sec>=start_s+2 && current.tv_usec>=start_us)
			break;
	} while(1);

//	cout<<start.tv_sec<<"."<<start.tv_usec<<endl;
	cout<<"Start time "<<current.tv_sec<<"."<<current.tv_usec<<endl;
         if (UDT::ERROR == (ss = UDT::send(client, data + ssize, size - ssize, 0)))
         {
            cout << "send:" << UDT::getlasterror().getErrorMessage() << endl;
  //break;

       }
//cout<<ss<<endl;
sleep(5);
//cout<<cchandle->count2<<endl;

//cout<<cchandle->m_dCWndSize<<endl;
//cout<<"YES!!"<<cchandle->haslost<<endl;
/*if(cchandle->haslost)
  {cchandle->haslost=0; windowsize-=4;}
if(cchandle->hasloss)
{cchandle->hasloss=0;
  windowsize-=7;

}
  windowsize+=2;

   if(ss==size)
   //   cout<<"FINISH!"<<endl;



*/
//cout<<windowsize<<endl;
  //sleep(5);

   delete [] data;



UDT::close(client);
   // use this function to release the UDT library
   UDT::cleanup();
//cout<<"TIME,"<<time<<endl;
sleep(3);
return NULL;
}
#ifndef WIN32
void* monitor(void* s)
#else
DWORD WINAPI monitor(LPVOID s)
#endif
{
   UDTSOCKET u = *(UDTSOCKET*)s;

   UDT::TRACEINFO perf;

   cout << "SendRate(Mb/s)\tRTT(ms)\tCWnd\tPktSndPeriod(us)\tRecvACK\tLoss" << endl;

   while (true)
   {
      #ifndef WIN32
	   sleep(100);
      #else
         Sleep(100);
      #endif

      if (UDT::ERROR == UDT::perfmon(u, &perf))
      {
         cout << "perfmon: " << UDT::getlasterror().getErrorMessage() << endl;
         break;
      }

      cout
           << perf.pktCongestionWindow
           << endl;
   }

   #ifndef WIN32
      return NULL;
   #else
      return 0;
   #endif
}
