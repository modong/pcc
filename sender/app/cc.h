#include <udt.h>
#include <ccc.h>
#include<iostream>
#include<cmath>

#define MAXCOUNT 1000
#define GRANULARITY 0.01
#define MUTATION_TH 300
#define JUMP_RANGE 0.05
#define NUMBER_OF_PROBE 4
#define MAX_COUNTINOUS_GUESS 5
#define MAX_COUNTINOUS_SEND 1
#define MAX_MONITOR_NUMBER 100
//#define DEBUGCC
//#define UTILITY_TRACE
using namespace std;
class CTCP: public CCC
{

	int dir;

public:
	void init()
	{
		m_bSlowStart = true;
		m_issthresh = 83333;

		m_dPktSndPeriod = 10000.0;
		m_dCWndSize = 2.0;

		setACKInterval(1);
		setRTO(2000000);
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

protected:
	int m_issthresh;
	bool m_bSlowStart;

	int m_iDupACKCount;
	int m_iLastACK;
};


class BBCC: public CCC
{
public:
	double utility_array[MAXCOUNT];
	double rate_array[MAXCOUNT];
	int array_pointer;
	int target_monitor;
	//int starting_phase;
	int make_guess;
	int guess_result;
	int moving_phase;
	int moving_phase_initial;
	int find_max;
	int random_guess;
	int random_guess_result;
	int mutation_counter;
	double rate_new;
	int change_intense;
	double change_amount;
	int change_direction;
	double rate_bucket[NUMBER_OF_PROBE];
	int monitor_bucket[NUMBER_OF_PROBE];
	double utility_bucket[NUMBER_OF_PROBE];
	int recorded_number;
	double current_rate;
	double previous_rate;
	double current_utility;
	double previous_utility;
	int guess_time;
	int continous_guess_count;
	int continous_send;
	int continous_send_count;
	int recording_guess_result;
	int baseline;
	double utility_baseline;
        double start_rate_array[100];
        int start_previous_monitor;
        double start_previous_utility;
        double previous_rtt;




public:
	BBCC()

	{
		m_dPktSndPeriod = 10000;
		m_dCWndSize = 100000.0;
		setRTO(100000000);
		starting_phase=1;
		target_monitor = 0;
		make_guess = 0;
		guess_result = 0;
		moving_phase = 0;
		moving_phase_initial = 0;
		change_direction=0;
		change_intense=1;
		guess_time = 0;
		continous_guess_count = 0;
		continous_send = 0;
		continous_send_count =0;
		previous_utility = 0;
		previous_rate = 1;
                current_rate = 1;
		recording_guess_result = 0;
		recorded_number = 0;
		baseline = 0;
		utility_baseline = 0;
                for(int i=0;i<100;i++)
                  start_rate_array[i] = 0;
                start_previous_monitor = -1;
                start_previous_utility = -10000;
                previous_rtt = 0;


	}

public:

		virtual void onLoss(const int32_t*, const int&) {

	}
	virtual void onTimeout(){

	}
	/*	int findmax(double arr[]){
		int tmp=0;
		for(int i=0;i<1000;i++){
			if(arr[i]>arr[tmp]&&stale[i]<20)
			{
				tmp=i;

			}
		}
		return tmp;
	}*/

	virtual void onMonitorStart(int current_monitor){

                if(starting_phase){
                start_rate_array[current_monitor] = previous_rate*2;
#ifdef DEBUGCC
                cerr<<"double rate to "<<start_rate_array[current_monitor]<<endl;
#endif
                previous_rate = start_rate_array[current_monitor];
                setRate(start_rate_array[current_monitor]);
                return;
                }
		if(make_guess == 1){
#ifdef DEBUGCC
                   cerr<<"make guess!"<<continous_guess_count<<endl;
#endif
			if(guess_time == 0 && continous_guess_count == MAX_COUNTINOUS_GUESS)
            {
#ifdef DEBUGCC
                   cerr<<"skip guess"<<endl;
#endif
				continous_guess_count =0;
            }
				if(guess_time == 0){
					recording_guess_result = 1;
					continous_guess_count++;

					int rand_dir;

					for(int i = 0; i < NUMBER_OF_PROBE; i++){
						rand_dir = (rand()%2*2-1);
						rate_bucket[i] = current_rate + rand_dir*continous_guess_count*GRANULARITY*current_rate;
						rate_bucket[++i] = current_rate - rand_dir*continous_guess_count*GRANULARITY*current_rate;
#ifdef DEBUGCC
                       cerr<<"guess rate"<<rate_bucket[i-1]<<" "<<rate_bucket[i]<<endl;
#endif
					}
					for(int i = 0; i < NUMBER_OF_PROBE; i++){
						monitor_bucket[i] = (current_monitor + i) % MAX_MONITOR_NUMBER;
#ifdef DEBUGCC
                                             cerr<<"guess monitor"<<monitor_bucket[i]<<endl;
#endif
					}

				}

				setRate(rate_bucket[guess_time]);
#ifdef DEBUGCC
                cerr<<"setrate as "<<rate_bucket[guess_time]<<endl;
#endif
				guess_time++;
				//TODO:Here the sender stopped at a particular rate
				if(guess_time == NUMBER_OF_PROBE){
#ifdef DEBUGCC
cerr<<"Guess exit!"<<endl;
#endif
					make_guess = 0;
					guess_time = 0;

				}
		}
			if(continous_send ==  1){
#ifdef DEBUGCC
                                cerr<<"CONTINOUS send"<<endl;
#endif
                                if(continous_send_count == 1){
                                        setRate(current_rate);

                                }
				if(continous_send_count < MAX_COUNTINOUS_SEND){
					continous_send_count++;
#ifdef DEBUGCC
                                        cerr<<"continous send"<<endl;
#endif

				}else{
#ifdef DEBUGCC
cerr<<"clear continous send"<<endl;
#endif
					continous_send = 0;
                                        continous_send_count = 0;
                                        continous_guess_count = 0;
					make_guess = 1;
				}
			}

	}

	virtual void onMonitorEnds(int total, int loss, double time, int current, int endMonitor, double rtt){

		double utility;
		double t=total;
		double l=loss;
		int random_direciton;
		if(l<0)
			l=0;
//utility = ((t-l)/time-20*l/time);
cerr<<rtt<<endl;
if(rtt==0) {
cerr<<"RTT cannot be 0!!!"<<endl;
}
if(previous_rtt==0)
previous_rtt = rtt;
//utility = ((t-l)/time*(1-1/(1+exp(-100*(l/t-0.05))))-1*l/time);
utility = ((t-l)/time*(1-1/(1+exp(-100*(l/t-0.05))))* (1-1/(1+exp(-10*(1-previous_rtt/rtt)))) -1*l/time)/rtt*1000;
previous_rtt = rtt;
if(endMonitor == 0 && starting_phase)
utility /=2;
#ifdef UTILITY_TRACE
		cerr<<current_rate<<'\t'<<(t-l)*12/time/1000<<'\t'<<t<<'\t'<<l<<'\t'<<time<<"\t"<<utility<<"\t"<<m_iRTT<<endl;
#endif

//		utility = (t-l)/time*(1-l/t)*(1-l/t)*(1-l/t)*(1-l/t)*(1-l/t);(m_iRTT/1000);(m_iRTT/1000);
#ifdef DEBUGCC
                cerr<<"end number"<<endMonitor<<endl;
#endif
		if(starting_phase){
                        if(endMonitor - 1 > start_previous_monitor){
                             if(start_previous_monitor == -1){
#ifdef DEBUGCC
                               cerr<<"fall back to guess mode"<<endl;
#endif
                             starting_phase = 0;
                             make_guess = 1;
                             setRate(start_rate_array[0]);
                             current_rate = start_rate_array[0];
                             return;
                             }else{
#ifdef DEBUGCC
                             cerr<<"exit because of loss"<<endl;
                             cerr<<"in monitor"<<start_previous_monitor<<endl;
                             cerr<<"fall back to due to loss"<<start_rate_array[start_previous_monitor]<<endl;
#endif
                             starting_phase = 0;
                             make_guess = 1;
                             setRate(start_rate_array[start_previous_monitor]);
                             current_rate = start_rate_array[start_previous_monitor];
                             return;}
                        }
                        if (start_previous_utility < utility){
#ifdef DEBUGCC
                            cerr<<"moving forward"<<endl;
#endif
                            // do nothing
                            start_previous_utility = utility;
                            start_previous_monitor = endMonitor;
                            return;
                        } else{
                             starting_phase = 0;
                             make_guess = 1;
                             setRate(start_rate_array[start_previous_monitor]);
                             current_rate = start_rate_array[start_previous_monitor];
#ifdef DEBUGCC
                             cerr<<"fall back to "<<start_rate_array[start_previous_monitor]<<endl;
#endif
                             previous_rate = current_rate;
                             return;
                        }

		}

		if(recording_guess_result){
/*                if(current_rate>(t*12/time/1000+10) && current_rate > 200)
                   {
                current_rate=t*12/time/1000;

                make_guess = 1;
                moving_phase = 0;
                moving_phase_initial = 0;
                change_direction=0;
                change_intense=1;
                guess_time = 0;
                continous_guess_count = 0;
                continous_send = 0;
                continous_send_count =0;
                recording_guess_result = 0;
                recorded_number = 0;
                setRate(current_rate);
                cerr<<"trigger"<<endl;
                return;
}*/

			for(int i = 0; i<NUMBER_OF_PROBE; i++){
				if(endMonitor == monitor_bucket[i]){

					recorded_number++;
					utility_bucket[i]=utility;
				}
}
				//TODO to let the sender go back to the current sending rate, one way is to
				//let the decision maker stop for another monitor period,which might not be a good option, let's try this first
				if(recorded_number == NUMBER_OF_PROBE){
					recorded_number = 0;
					double decision = 0;
					for(int i=0; i < NUMBER_OF_PROBE; i++){
						// TODO: I don't use this because I am afraid of the calculation precision problem
						// decision += (utility_bucket[i]>utility_bucket[i+1])?(rate_bucket[i]-rate_bucket[i+1]):(rate_bucket[i+1]-rate_bucket[i]);
						//cerr<<"utility"<<utility_bucket[i]<<" "<<utility_bucket[i+1]<<endl;
                                                //cerr<<"rate"<<rate_bucket[i]<<" "<<rate_bucket[i+1]<<endl;
						if(((utility_bucket[i]>utility_bucket[i+1])&&(rate_bucket[i]>rate_bucket[i+1]))||((utility_bucket[i]<utility_bucket[i+1])&&(rate_bucket[i]<rate_bucket[i+1])))
							decision +=1;
						else
							decision -=1;
						i++;
					}
					if(decision == 0){
						make_guess = 1;
						recording_guess_result = 0;
#ifdef DEBUGCC
                        cerr<<"no decision"<<endl;
#endif
					}else{
						change_direction = decision>0?1:-1;
#ifdef DEBUGCC
                        cerr<<"change to the direction of"<<change_direction<<endl;
#endif
						recording_guess_result = 0;
						target_monitor = (current+1)%MAX_MONITOR_NUMBER;
						moving_phase_initial = 1;
						change_intense = 1;
						change_amount = (continous_guess_count/2+1)*change_intense*change_direction * GRANULARITY * current_rate;
						previous_utility = 0;
						continous_guess_count--; continous_guess_count=0;
						if(continous_guess_count < 0)
                                                   continous_guess_count = 0;
						previous_rate = current_rate;
						current_rate = current_rate + change_amount;
						target_monitor = (current+1)%MAX_MONITOR_NUMBER;
                                                setRate(current_rate);
					}
				}



		}

		if(moving_phase_initial && endMonitor == target_monitor){
                if(current_rate>(t*12/time/1000+30) && current_rate > 200)
                   {
                current_rate=t*12/time/1000;

                make_guess = 1;
                moving_phase = 0;
                moving_phase_initial = 0;
                change_direction=0;
                change_intense=1;
                guess_time = 0;
                continous_guess_count = 0;
                continous_send = 0;
                continous_send_count =0;
                recording_guess_result = 0;
                recorded_number = 0;
                setRate(current_rate);
#ifdef DEBUGCC
                cerr<<"system udp call speed limiting, resyncing rate"<<endl;
#endif
                return;
}

#ifdef DEBUGCC
cerr<<"first time moving"<<endl;
#endif
			target_monitor = (current+1)%MAX_MONITOR_NUMBER;
			previous_rate = current_rate;
			previous_utility = utility;
                        change_intense+=1;
			change_amount = change_intense * GRANULARITY * current_rate * change_direction;
            current_rate = current_rate + change_amount;
            setRate(current_rate);
            moving_phase_initial = 0;
            moving_phase = 1;

		}

		if(moving_phase && endMonitor == target_monitor){

                if(current_rate>(t*12/time/1000+30) && current_rate > 200)
                   {
                current_rate=t*12/time/1000;


                make_guess = 1;
                moving_phase = 0;
                moving_phase_initial = 0;
                change_direction=0;
                change_intense=1;
                guess_time = 0;
                continous_guess_count = 0;
                continous_send = 0;
                continous_send_count =0;
                recording_guess_result = 0;
                recorded_number = 0;
                setRate(current_rate);
#ifdef DEBUGCC
                cerr<<"system udp call speed limiting, resyncing rate"<<endl;
#endif
                return;
}

#ifdef DEBUGCC
                        cerr<<"moving faster"<<endl;
#endif
			current_utility = utility;
			if(current_utility>previous_utility){
				target_monitor = (current+1)%MAX_MONITOR_NUMBER;
				change_intense+=1;
				previous_utility = current_utility;
				previous_rate = current_rate;
				change_amount = change_intense * GRANULARITY * current_rate * change_direction;
				current_rate = current_rate + change_amount;
				setRate(current_rate);


			}else{
				moving_phase = 0;
				make_guess = 1;
				//change_intense+=1;
				previous_utility = current_utility;
				previous_rate = current_rate;
				change_amount =  change_intense * GRANULARITY * current_rate * change_direction;
				current_rate = current_rate -change_amount;
				setRate(current_rate);



			}
		}
	}

		virtual void onACK(const int& ack)
	{}

	void setRate(double mbps)
	{
		m_dPktSndPeriod = (m_iMSS * 8.0) / mbps;
	}
};
