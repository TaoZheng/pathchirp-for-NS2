/* pathchirpSnd.cc
 *
 * Author: Vinay Ribeiro, Rice University, vinay@rice.edu.
 *
 * Last modification: August 20, 2003
 *
 * This code is based on code of the ns-2 simulator (copyright produced below)
 */

/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1997 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Daedalus Research
 *	Group at the University of California Berkeley.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include "pathChirpSnd.h"


int hdr_pathChirpSnd::offset_;

static class pathChirpHeaderClass : public PacketHeaderClass {
public:
	pathChirpHeaderClass() : PacketHeaderClass("PacketHeader/pathChirpSnd", 
					      sizeof(hdr_pathChirpSnd)) {
		bind_offset(&hdr_pathChirpSnd::offset_);
	}
} class_pathChirphdrSnd;

/*int hdr_pathChirpRcv::offset_;

static class pathChirpHeaderClass : public PacketHeaderClass {
public:
	pathChirpHeaderClass() : PacketHeaderClass("PacketHeader/pathChirpRcv", 
					      sizeof(hdr_pathChirpRcv)) {
		bind_offset(&hdr_pathChirpRcv::offset_);
	}
} class_pathChirphdrRcv;
*/

static class pathChirpSndClass : public TclClass {
public:
	pathChirpSndClass() : TclClass("Agent/pathChirpSnd") {}
	TclObject* create(int, const char*const*) {
		return (new pathChirpAgentSnd());
	}
} class_pathChirpSnd;

/* constructor*/
pathChirpAgentSnd::pathChirpAgentSnd() : Agent(PT_PATHCHIRPSND),running_(0), timer_(this)
{  	
    
      bind("packetSize_", &size_);
      bind("lowrate_", &lowrate_);
      bind("highrate_", &highrate_);
      bind("spreadfactor_", &spreadfactor_);
      bind("avgrate_", &avgrate_);
}


/* send out a packet with timestamp and appropriate  packet number */
void pathChirpAgentSnd::sendpkt()
{

      // Create a new packet
      Packet* pkt = allocpkt();
      

      // Access the pathChirp header for the new packet:
      hdr_pathChirpSnd* hdr = hdr_pathChirpSnd::access(pkt);

      // Store the current time in the 'send_time' field
      hdr->send_time = Scheduler::instance().clock();
      //hdr->size=packetSize_;
      hdr->pktnum_ = pktnum_;

      hdr->lowrate_ = lowrate_;
      hdr->spreadfactor_ = spreadfactor_;
      hdr->num_pkts_per_chirp = num_pkts_per_chirp;
      hdr->packetSize_ = packetSize_;

      // Send the packet
      send(pkt, 0);
      //   return (TCL_OK);
      return;
}
     
/* if timer expires just do a timeout */
void pathChirpAgentSndTimer::expire(Event*)
{
        t_->timeout();
}

/* initialization */
void pathChirpAgentSnd::start()
{
        
        packetSize_=size_;
        running_ = 1;
	pktnum_ = 0;


	if ((lowrate_>highrate_) || (lowrate_<0) || (highrate_<0) || (avgrate_<0))
	  {
	    fprintf(stderr,"pathChirpSnd: error in rate specification\n");
	    exit(0);
	  }
	if (spreadfactor_<=1.0)
	  {
	    fprintf(stderr,"pathChirpSnd: spreadfactor_ too low\n");
	    exit(0);
	  }

	interchirpgap_=computeinterchirpgap();
	if (interchirpgap_<0)
	  interchirpgap_=0;

	lowcount=0;
	highcount=0;
	losscount=0;
	resetflag=0;

	//NOTE: (TO DO) put in random start time to avoid synchronization
	curriat_ = interchirpgap_;
	timer_.sched(curriat_);
}

/* reset to new chirp parameters */
void pathChirpAgentSnd::resetting()
{
  lowrate_=newlowrate_;
  highrate_=newhighrate_;

  /* make sure average rate is not too high  */
  if (avgrate_>lowrate_/3.0)
    avgrate_=lowrate_/3.0;
  else
    if (avgrate_<lowrate_/10.0)
      {
	avgrate_=lowrate_/10.0;
      }
  /*make sure avgrate_ is less than 10Mbps*/
  if(avgrate_>10000000.0)
    avgrate_=10000000.0;
  
  /* making sure the average probing rate is at least 5kbps */
  if (avgrate_<5000.0) avgrate_=5000.0;
  
  interchirpgap_=computeinterchirpgap();
  if (interchirpgap_<0)
    interchirpgap_=0;
  
  resetflag=0;
  
  return;
}

/* stop the pathChirp agent sending packets */
void pathChirpAgentSnd::stop()
{
        running_ = 0;
	return;
}

/* sends a packet and timesout for the appropriate time*/
void pathChirpAgentSnd::timeout()
{
  if (running_==1) 
    {
      //if new params and pktnum
      if (resetflag==1 && pktnum_==0)
	{
	  resetting();
	  curriat_ = interchirpgap_;

	}

      sendpkt();
      /* reschedule the timer */
      curriat_ = next(curriat_);
      
      timer_.resched(curriat_);
    }
}

/* returns the next inter packet spacing, set pkt number of next packet */
double pathChirpAgentSnd::next(double iat)
{  

     pktnum_++;
     if (pktnum_>num_pkts_per_chirp-1)
       {
	 pktnum_=0;
	 iat=interchirpgap_;
       }
     else
       {
	 if (pktnum_==1)
	   iat=8*(double)packetSize_/lowrate_;
	 else
	   iat=iat/spreadfactor_; 
       }

     return iat;
}

/*time between chirps to maintain the average specified probing rate */
double pathChirpAgentSnd::computeinterchirpgap()
{
	int num_pkts=1;
	double rate;
	double chirptime=0.0;
	double interchirptime=0.0;

	rate=lowrate_;
	
	while(rate<highrate_)
	  {
	    num_pkts++;
	    chirptime+=1/rate;
	    rate=rate*spreadfactor_;
	  }
	
	chirptime=chirptime*(double)(8*packetSize_);

	interchirptime=(double)(num_pkts)*8.0*(double)(packetSize_)/avgrate_ - chirptime;
	if (interchirptime<0.0) interchirptime=0.0;
	
	num_pkts_per_chirp=num_pkts;
	return interchirptime;
}


/* used by tcl script */
int pathChirpAgentSnd::command(int argc, const char*const* argv)
{
	if (argc == 2) {
	  if (strcmp(argv[1], "start") == 0) {
	    start();
	    return (TCL_OK);
	  }
	  else if(strcmp(argv[1], "stop") == 0)
	    {
	      stop();
	      return (TCL_OK);
	    }
	}
	return (Agent::command(argc, argv));
}


/* on receiving packet run the pathchirp algorithm */
void pathChirpAgentSnd::recv(Packet* pkt, Handler*)
{
 
  // Access the IP header for the received packet:
  hdr_ip* hdrip = hdr_ip::access(pkt);
  
  // Access the pathChirp header for the received packet:
  hdr_pathChirpRcv* hdr = hdr_pathChirpRcv::access(pkt);
  
  // check if in brdcast mode
  if ((u_int32_t)hdrip->daddr() == IP_BROADCAST) {
      Packet::free(pkt);
      return;
  }

  //ALGO TO CHANGE RATES
  if (hdr->loss==1)
    losscount++;
  else 
    losscount=0;

  if (hdr->inst_bw<1.5*lowrate_)
    lowcount++;
  else 
    lowcount=0;

  if (hdr->inst_bw>0.66*highrate_)
    highcount++;
  else 
    highcount=0;


  /* if estimate too low or high change probing rates */
  if (lowcount>2)
    {
      if (lowrate_<=1000000.0) /*less than 1Mbps, have smaller range of rates, that is fewer packets per chirp. This allows us to send more chirps per unit time.*/
	{
	  newhighrate_=lowrate_*4.0;
	  newlowrate_=lowrate_/4.0;
	}
      else{
	  newhighrate_=lowrate_*7.0;
	  newlowrate_=lowrate_/7.0;
	}

      resetflag=1;
      lowcount=0;
    }

  if (highcount>2)
    {
      if (highrate_>1000000.0) 
	{
	  newlowrate_=highrate_/7.0;
	  newhighrate_=highrate_*7.0;
	}
      else{
	  newlowrate_=highrate_/4.0;
	  newhighrate_=highrate_*4.0;
	}
      resetflag=1;
      highcount=0;
    }


  if (losscount>2)
    {
      newlowrate_=lowrate_/10.0;
      newhighrate_=highrate_/10.0;
      resetflag=1;
      losscount=0;
    }


  Packet::free(pkt);
  return;

}
