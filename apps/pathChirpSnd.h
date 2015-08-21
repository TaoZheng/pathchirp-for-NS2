/* pathchirpSnd.h
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

#ifndef ns_pathChirp_h
#define ns_pathChirp_h

#include "agent.h"
#include "tclcl.h"
#include "packet.h"
#include "address.h"
#include "ip.h"

class pathChirpAgentSnd;

struct hdr_pathChirpSnd {
	int pktnum_;
	int packetSize_;
	double send_time;
	double lowrate_;
	double spreadfactor_;
	int num_pkts_per_chirp;

	// Header access methods
	static int offset_; // required by PacketHeaderManager
	inline static int& offset() { return offset_; }
	inline static hdr_pathChirpSnd* access(const Packet* p) {
		return (hdr_pathChirpSnd*) p->access(offset_);
	}
};

struct hdr_pathChirpRcv {
	int loss;
	double inst_bw;

	// Header access methods
	static int offset_; // required by PacketHeaderManager
	inline static int& offset() { return offset_; }
	inline static hdr_pathChirpRcv* access(const Packet* p) {
		return (hdr_pathChirpRcv*) p->access(offset_);
	}
};


class pathChirpAgentSndTimer : public TimerHandler {
public:
	pathChirpAgentSndTimer(pathChirpAgentSnd *t) : TimerHandler() { t_ = t; }
protected:
	virtual void expire(Event *e);
	pathChirpAgentSnd *t_;
};



class pathChirpAgentSnd : public Agent {
 public:
	pathChirpAgentSnd();
	virtual void timeout();
 protected:
	virtual void start();
	virtual void stop();
	virtual void sendpkt();
	virtual void resetting();
	virtual int command(int argc, const char*const* argv);
	virtual void recv(Packet*, Handler*);

	inline double next(double);
	inline double computeinterchirpgap();

	double lowrate_;
	double avgrate_;
	double highrate_;
	double newlowrate_;
	double newavgrate_;
	double newhighrate_;
	double spreadfactor_;
	double interchirpgap_;
	int running_;
	int pktnum_;
	int packetSize_;
	double curriat_;
	int num_pkts_per_chirp;
	int lowcount;
	int losscount;
	int highcount;
	int resetflag;

	pathChirpAgentSndTimer timer_;
};

#endif


