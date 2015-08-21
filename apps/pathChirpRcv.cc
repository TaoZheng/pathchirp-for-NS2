/* pathchirpRcv.cc
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

#include "pathChirpRcv.h"


/*int hdr_pathChirpSnd::offset_;

static class pathChirpHeaderClass : public PacketHeaderClass {
public:
	pathChirpHeaderClass() : PacketHeaderClass("PacketHeader/pathChirpSnd", 
					      sizeof(hdr_pathChirpSnd)) {
		bind_offset(&hdr_pathChirpSnd::offset_);
	}
} class_pathChirphdrSnd;
*/

int hdr_pathChirpRcv::offset_;

static class pathChirpHeaderClass : public PacketHeaderClass {
public:
	pathChirpHeaderClass() : PacketHeaderClass("PacketHeader/pathChirpRcv", 
					      sizeof(hdr_pathChirpRcv)) {
		bind_offset(&hdr_pathChirpRcv::offset_);
	}
} class_pathChirphdrRcv;


static class pathChirpRcvClass : public TclClass {
public:
	pathChirpRcvClass() : TclClass("Agent/pathChirpRcv") {}
	TclObject* create(int, const char*const*) {
		return (new pathChirpAgentRcv());
	}
} class_pathChirpRcv;

/* constructor*/
pathChirpAgentRcv::pathChirpAgentRcv() : Agent(PT_PATHCHIRPRCV) 
{ 
      bind("packetSize_", &size_);
      bind("num_inst_bw", &num_inst_bw);
      bind("decrease_factor", &decrease_factor);
      bind("busy_period_thresh", &busy_period_thresh);
  
}


/* initialization */
void pathChirpAgentRcv::start()
{
  running_=0;//initializing to zero
  inst_bw_estimates=(double *)calloc((int)(num_inst_bw),sizeof(double));
  inst_bw_count=0;

  total_inst_bw=0;
  inst_head=0;
  congestionflag_=0;
}

/* resets connection, will recalculate parameters etc with next received packet */
void pathChirpAgentRcv::resetting()
{
  running_=0;//initializing to zero

  congestionflag_=0;


  free(qing_delay);
  free(rates);
  free(iat);
  free(av_bw_per_pkt);
 return;
}

/* writes out smooted available bandwidth estimate */
void pathChirpAgentRcv::write_instant_bw()
{
  double inst_av_bw;
  double inst_mean=0.0;

  /*taking latest value and removing old one*/
  inst_av_bw=computeavbw();

  total_inst_bw+=inst_av_bw;
  total_inst_bw-=inst_bw_estimates[inst_head];
  
  inst_bw_estimates[inst_head]=inst_av_bw;
  
  inst_head++;
  /*circular buffer*/
  if (inst_head>=num_inst_bw)
    inst_head=0;
  
  if (inst_bw_count>num_inst_bw)
    {
      inst_mean=total_inst_bw/((double) num_inst_bw);
      /* write to file */
      fprintf(stdout,"%f %f\n",Scheduler::instance().clock(),inst_mean/1000000.0);
      sendpkt(inst_mean,congestionflag_);
    }
  else  
    inst_bw_count++;

  //reset congestion flag
  congestionflag_=0;

}


/* the pathChirp algorithm*/
double pathChirpAgentRcv::computeavbw()
{
  /* compute the instantaneous bandwidth during this chirp, new algorithm */
  
  int cur_loc=0;/* current location in chirp */
  int cur_inc=0;/* current location where queuing delay increases */
  int count;
  double inst_bw=0.0,sum_iat=0.0,max_q=0.0;
  
  
  if (congestionflag_==1)
    {
      inst_bw=0.0;
      return(inst_bw);//returning zero available bandwidth
    }
  
  /* set all av_bw_per_pkt to zero */
  memset(av_bw_per_pkt, 0, (int)(num_interarrival) * sizeof(double));
  
  /*find first place with an increase in queuing delay*/
  while((qing_delay[cur_inc]>=qing_delay[cur_inc+1]) && (cur_inc<num_interarrival))
    cur_inc++;
  
  cur_loc=cur_inc+1; 
  
  /* go through all delays */
  
  while(cur_loc<=num_interarrival)
    {
      /* find maximum queuing delay between cur_inc and cur_loc*/
      if (max_q<(qing_delay[cur_loc]-qing_delay[cur_inc]))
	max_q=qing_delay[cur_loc]-qing_delay[cur_inc];
      
      /* check if queuing delay has decreased to "almost" what it was at cur_inc*/
      if (qing_delay[cur_loc]-qing_delay[cur_inc]<(max_q/decrease_factor))
	{
	  if (cur_loc-cur_inc>=busy_period_thresh)
	    {
	      for (count=cur_inc;count<=cur_loc-1;count++)
		{
		  /*mark all increase regions between cur_inc and cur_loc as having
		    available bandwidth equal to their rates*/
		  if (qing_delay[count]<qing_delay[count+1])
		    av_bw_per_pkt[count]=rates[count];
		}
	    }
	  /* find next increase point */
	  cur_inc=cur_loc;
	  while(qing_delay[cur_inc]>=qing_delay[cur_inc+1] && cur_inc<num_interarrival)
	    cur_inc++;
	  cur_loc=cur_inc;
	  /* reset max_q*/
	  max_q=0.0;
	}

      cur_loc++;

    }
  /*mark the available bandwidth during the last excursion as the rate at its beginning*/
  if(cur_inc==num_interarrival)
    cur_inc--;
  
  for(count=cur_inc;count<num_interarrival;count++)
    {
      av_bw_per_pkt[count]=rates[cur_inc];
    }
  
  
  /* all unmarked locations are assumed to have available 
     bandwidth described by the last excursion */
  
  for(count=0;count<num_interarrival;count++)
    {

      if (av_bw_per_pkt[count]<1.0)
	av_bw_per_pkt[count]=rates[cur_inc];
      sum_iat+=iat[count];
      inst_bw+=av_bw_per_pkt[count]*iat[count];
    }

  inst_bw=inst_bw/sum_iat;

  return(inst_bw);
  
}

/* on receiving packet run the pathchirp algorithm */
void pathChirpAgentRcv::recv(Packet* pkt, Handler*)
{
 
  // Access the IP header for the received packet:
  hdr_ip* hdrip = hdr_ip::access(pkt);
  
  // Access the pathChirp header for the received packet:
  hdr_pathChirpSnd* hdr = hdr_pathChirpSnd::access(pkt);
  
  // check if in brdcast mode
  if ((u_int32_t)hdrip->daddr() == IP_BROADCAST) {
      Packet::free(pkt);
      return;
  }

  //on receipt of first packet allocate memory for variables
  if (running_==0)
    {
      num_interarrival=hdr->num_pkts_per_chirp-1;
      lowrate_=hdr->lowrate_;
      spreadfactor_=hdr->spreadfactor_;
      packetSize_ = hdr->packetSize_;

      allocatemem();
      running_=1;
    }
  else
    {
      //if there is a change in parameters then reset
      if ((lowrate_!=hdr->lowrate_) || (num_interarrival!=(hdr->num_pkts_per_chirp-1)))
	{
	  resetting();
      num_interarrival=hdr->num_pkts_per_chirp-1;
      lowrate_=hdr->lowrate_;
      spreadfactor_=hdr->spreadfactor_;
      packetSize_ = hdr->packetSize_;

      allocatemem();
      running_=1;
	}
    }


   // If packet is missing then don't run the pathChirp 
   // algo, mark congested flag
  // NOTE: We are ignoring the case when more than num_pkts_per_chirp consecutive packets are lost (will rarely occur)
 	  if ((hdr->pktnum_ - prevpktnum_ >1) || ((prevpktnum_<hdr->num_pkts_per_chirp-1) && (hdr->pktnum_ <= prevpktnum_)))
	  {
	    congestionflag_=1;
	  }

	  // if we are already in the next chirp because the last few packets of the previous one was dropped then write the bandwidth estimate
	  if ((prevpktnum_<hdr->num_pkts_per_chirp-1) && (hdr->pktnum_ < prevpktnum_))
	    {
	       write_instant_bw();
	    }

   //record queuing delay
   qing_delay[hdr->pktnum_]=Scheduler::instance().clock() - hdr->send_time;
   

   if (hdr->pktnum_ == (hdr->num_pkts_per_chirp-1))
     write_instant_bw();

   prevpktnum_=hdr->pktnum_;

    // free memory
    Packet::free(pkt);

}


/* send out a packet with timestamp and appropriate  packet number */
void pathChirpAgentRcv::sendpkt(double inst_bw,int loss)
{

      // Create a new packet
      Packet* pkt = allocpkt();

      // Access the pathChirp header for the new packet:
      hdr_pathChirpRcv* hdr = hdr_pathChirpRcv::access(pkt);

      // Store the current time in the 'send_time' field
      hdr->inst_bw=inst_bw;

      //hdr->size=packetSize_;
      hdr->loss = loss;

      // Send the packet
      send(pkt, 0);

      return;
}
   

/* used by tcl script */
int pathChirpAgentRcv::command(int argc, const char*const* argv)
{
	if (argc == 2) {
	  if (strcmp(argv[1], "start") == 0) {
	    start();
	    return (TCL_OK);
	  }
	}
	return (Agent::command(argc, argv));
}

/* allocating memory */
void pathChirpAgentRcv::allocatemem()
{

  int count;
  

  qing_delay=(double *)calloc((int)(num_interarrival+1),sizeof(double));

  rates=(double *)calloc((int)(num_interarrival),sizeof(double));
  iat=(double *)calloc((int)(num_interarrival),sizeof(double));

  av_bw_per_pkt=(double *)calloc((int)(num_interarrival),sizeof(double));

  /* initialize variables */
  
  rates[0]=lowrate_;


  iat[0]=8*((double) packetSize_)/rates[0];

  /* compute instantaneous rates within chirp */
  for (count=1;count<=num_interarrival-1;count++)
    {

      rates[count]=rates[count-1]*spreadfactor_;
      iat[count]=iat[count-1]/spreadfactor_;

    }

  return;

}


