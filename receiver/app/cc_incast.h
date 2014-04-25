#include <udt.h>
#include <ccc.h>
#include<iostream>
using namespace std;
#include <sys/time.h>
class CTCP: public CCC
{

public:

int pktNum;
   void init()

   {  first_time=1;
      m_bSlowStart = true;
      m_issthresh = 83333;
      old_seq=0;
     pktNum=0;
 m_dPktSndPeriod = 0.0;
      m_dCWndSize = 2.0;
      flag_receive=1;
      setACKInterval(1);
      setRTO(9000000);
      hastimeout=0;

   }

   virtual void onACK(const int& ack)
   {
      if (ack == m_iLastACK)
      {
         if (3 == ++ m_iDupACKCount)
            DupACKAction();
         else if (m_iDupACKCount > 3)
            m_dCWndSize += 1.0;
         else
            ACKAction();
      }
      else
      {
         if (m_iDupACKCount >= 3)
            m_dCWndSize = m_issthresh;

         m_iLastACK = ack;
         m_iDupACKCount = 1;

         ACKAction();
      }
   }

   virtual void onTimeout()
   {
     cout<<"DINGDING@"<<endl;
     m_issthresh = getPerfInfo()->pktFlightSize / 2;
      if (m_issthresh < 2)
         m_issthresh = 2;

      m_bSlowStart = true;
      m_dCWndSize = 2.0;
   }

protected:
   virtual void ACKAction()
   {
      if (m_bSlowStart)
      {
         m_dCWndSize += 1.0;

         if (m_dCWndSize >= m_issthresh)
            m_bSlowStart = false;
      }
      else
         m_dCWndSize += 1.0/m_dCWndSize;
   }

   virtual void DupACKAction()
   {
      m_bSlowStart = false;

      m_issthresh = getPerfInfo()->pktFlightSize / 2;
      if (m_issthresh < 2)
         m_issthresh = 2;

      m_dCWndSize = m_issthresh + 3;
   }
int count;
struct timeval show;

 virtual void onPktReceived(const CPacket* p){ //cout<<p->m_iTimeStamp<<endl;
if(first_time){
old_seq=p->m_iSeqNo-1;
first_time=0;
start=p->m_iSeqNo;
count=0;
gettimeofday(&begintime,NULL);
}

gettimeofday(&endtime ,NULL);

}
public:
   int m_issthresh;
   bool m_bSlowStart;
   int flag_receive;
   int m_iDupACKCount;
   int m_iLastACK;
   int32_t old_seq;
   int first_time;
   int32_t start;
   int32_t ends;
   int hastimeout;
   struct timeval endtime;
   struct timeval begintime;
};

class CUDPBlast: public CCC
{
public:
   CUDPBlast()
   {
      m_dPktSndPeriod = 1000000; 
      m_dCWndSize = 83333.0; 
   }

public:
   void setRate(double mbps)
   {
      m_dPktSndPeriod = (m_iMSS * 8.0) / mbps;
   }
};
