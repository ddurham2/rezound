/* 
 * Copyright (C) 2002 - David W. Durham
 * 
 * This file is part of ReZound, an audio editing application.
 * 
 * ReZound is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 * 
 * ReZound is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 */

/*
 * Information about MIDI's Sample Dump Specification obtained from:
 *	http://www.borg.com/~jglatt/tech/sds.htm
 */

#include "CMIDISDSSoundTranslator.h"

#include <stdexcept>
#include <algorithm>

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

//#include <sys/poll.h>

#include <istring>
#include <CPath.h>

#include "AStatusComm.h"
#include "AFrontendHooks.h"
#include "CSound.h"

//#define DEBUG_PRINT


CMIDISDSSoundTranslator::CMIDISDSSoundTranslator()
{
}

CMIDISDSSoundTranslator::~CMIDISDSSoundTranslator()
{
}

// for MIDI bytes (3 7bit bytes to native int)
static int bytesToInt(unsigned char bh,unsigned char bm,unsigned char bl)
{
	return (((int)(bh&0x7f))<<7<<7) + (((int)(bm&0x7f))<<7) + ((int)(bl&0x7f));
}

static int full_read(int fd,unsigned char *buffer,int len)
{
	int l=0;
	while(l<len)
	{
#ifdef DEBUG_PRINT
		printf("%s: reading... %d\n",__func__,len-l);fflush(stdout);
#endif
		int t=read(fd,buffer+l,len-l);
		if(t==-1)
			throw runtime_error(string(__func__)+" -- error reading from midi device -- "+strerror(errno));
		if(t==0)
			break;
#ifdef DEBUG_PRINT
		for(int k=0;k<t;k++)
			printf("%s: 0x%x (%d/%d)\n",__func__,*(buffer+l+k),l+k+1,len),fflush(stdout);
#endif
	
		l+=t;
	}
	return l;
}

static void print_hex(unsigned char *buffer,int len)
{
	printf("------------\n");fflush(stdout);
	for(int t=0;t<len;t++)
		printf("%d: 0x%x, ",t,buffer[t]),fflush(stdout);
	printf("\n");fflush(stdout);
}

// wait for a pattern on the input (-1 is wild-card in pattern)
// -1 is returned if cancelled
static int wait_for_pattern(const int fd,unsigned char *buffer,const int pattern[],const int patternLen,int msTimeout,const char *waitMessage=NULL)
{
	const int origFlags=fcntl(fd,F_GETFL);

	// turn off blocking
	fcntl(fd,F_SETFL,origFlags|O_NONBLOCK);

		// ??? some kind of wait-cancel prompt might be better for this task
	CStatusBar *statusBar=waitMessage ? new CStatusBar(waitMessage,0,1,true) : NULL;
	try
	{
		int tmp=0;

		int patternPos=0; // this is the position we're trying to match

		int maxTimeout=0;

		// use select to do a timed-out wait for something available on the opened file descriptor since this might be a physical MIDI device
		while(patternPos<patternLen)
		{
			int r;

			fd_set rfds;
			struct timeval tv;

			FD_ZERO(&rfds);
			FD_SET(fd,&rfds);
			tv.tv_sec=0;
			tv.tv_usec=250 *1000; // 1/4 seconds
			r=select(fd+1,&rfds,NULL,NULL,&tv);

			/* could use poll, but it's not necessarily on every system
			pollfd foo;
			foo.fd=fd;
			foo.events=POLLIN;
			foo.revents=0;
			r=poll(&foo,1,240);
			*/

			if(r!=0)
			{ // not timed out
				unsigned char readBuffer[1024];
				const int l=read(fd,readBuffer,min(1024,patternLen-patternPos));
				if(l>0)
				{
					// check all bytes read if they match the pattern; as soon as one byte doesn't match, start looking for the beginning again
					for(int t=0;t<l;t++)
					{
#ifdef DEBUG_PRINT
						printf("%s: 0x%x\n",__func__,readBuffer[t]);fflush(stdout);
#endif
						if(pattern[patternPos]==-1 || pattern[patternPos]==readBuffer[t])
							buffer[patternPos++]=readBuffer[t];
						else
							patternPos=0; // start over
					}
				}
				else if(l<0)
					throw runtime_error(string(__func__)+" -- error reading from file -- "+strerror(errno));
				//maxTimeout=0;  I could reset the timeout here, but I think I want to time out based on not having received the expected pattern
			}
			else if(r<0)
			{ // error
				int err=errno;
				throw runtime_error(string(__func__)+" -- "+strerror(err));
			}
			else
			{ // timed out
				maxTimeout+=250;

				if(maxTimeout>msTimeout)
				{ // timed-out for good
					fcntl(fd,F_SETFL,origFlags);
					delete statusBar;
					return patternPos;
				}

				if(statusBar && statusBar->update(tmp++%2))
				{ // cancelled
					fcntl(fd,F_SETFL,origFlags);
					delete statusBar;
					return -1;
				}
			}
		}

		// restore original flags
		fcntl(fd,F_SETFL,origFlags);

		delete statusBar;

		return patternPos;
	}
	catch(...)
	{
		fcntl(fd,F_SETFL,origFlags);
		delete statusBar;
		throw;
	}
}

	// ??? could just return a CSound object and have used the one constructor that takes the meta info
	// ??? but, then how would I be able to have createWorkingPoolFileIfExists
bool CMIDISDSSoundTranslator::onLoadSound(const string filename,CSound *sound) const
{
	int sysExChannel=-1;
	
	const bool isDevice=CPath(filename).isDevice();

	bool ret=true;

#ifdef DEBUG_PRINT
	printf("opening: %s\n",filename.c_str());fflush(stdout);
#endif
	int fd=open(filename.c_str(),isDevice ? O_RDWR : O_RDONLY);
	if(fd==-1)
		throw runtime_error(string(__func__)+" -- error opening file: "+filename+" -- "+strerror(errno));
	try
	{
		unsigned char buffer[256];
		int l;

		// if we're dealing with a device, then prompt the user for which sample Id to request, otherwise just wait for one
		bool sampleDumpRequestSent=false; // use this flag to know if we should timeout after not getting anything
		if(isDevice)
		{
			int waveformId=-1;
			if(!gFrontendHooks->promptForOpenMIDISampleDump(sysExChannel,waveformId))
			{
				close(fd);
				return false;
			}

			if(waveformId>16384)
				throw runtime_error(string(__func__)+" -- waveform IDs must be 0 to 16384");

			if(waveformId>=0)
			{ // send DUMP REQUEST
				unsigned char buffer[]={0xf0,0x7e,0,0x3,0,0,0xf7};
				buffer[2]=sysExChannel;

				buffer[4]=waveformId&0x7f; 		// bits 0 to 6
				buffer[5]=(waveformId>>7)&0x7f; 	// bits 7 to 13

				if(write(fd,buffer,7)!=7)
				{
					const int err=errno;
					throw runtime_error(string(__func__)+" -- error writing SAMPLE DUMP request to device -- perhaps the disk is full -- "+strerror(err));
				}

				sampleDumpRequestSent=true;
			}
		}

		/*
		{
			unsigned char buffer[256];
			int l;
			l=full_read(fd,buffer,21);
			printf("l: %d\n",l);fflush(stdout);
			print_hex(buffer,21);
			return false;
		}
		*/

		// wait for "Dump Header" pattern: F0 7E cc 01 sl sh ee pl pm ph gl gm gh hl hm hh il im ih jj F7
		static const int dumpHeaderPattern[]={0xf0,0x7e,-1,0x01,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,0xf7};
		static const int dumpHeaderPatternLen=sizeof(dumpHeaderPattern)/sizeof(*dumpHeaderPattern);

		// I would do the line below, but for some dumb reason if I instantiate a CStatusBar and (select or poll)/read on fd, then it drops data reading.  I even used ltrace -S to see if I could see FOX inadvertantly reading from fd.  I did find that if I instantated the CStatusBar BEFORE I opened fd then it worked.  Anyway, for now I'm just passing NULL for the message so that a statusBar won't be created when a DUMP REQUEST was sent
		//l=wait_for_pattern(fd,buffer,dumpHeaderPattern,dumpHeaderPatternLen,sampleDumpRequestSent ? 2*1000 : 3600*1000,"Waiting For Dump");
		if(sampleDumpRequestSent)
			l=wait_for_pattern(fd,buffer,dumpHeaderPattern,dumpHeaderPatternLen,2*1000,NULL);
		else
			l=wait_for_pattern(fd,buffer,dumpHeaderPattern,dumpHeaderPatternLen,3600*1000,"Waiting For Dump");

		if(l!=dumpHeaderPatternLen)
		{
			if(l==-1)
			{ // cancelled
				close(fd);
				return false;
			}

			if(sampleDumpRequestSent)
			{
				// ??? a CANCEL msg might come if the sample doesn't exist, so check for that too if possible?
				Error("Sample Dump Request Failed -- that waveform ID may not exist");
				close(fd);
				return false;
			}
			else
				throw runtime_error(string(__func__)+" -- error while waiting for Dump Header -- only read "+istring(l)+" bytes");
		}

		// expecting: F0 7E cc 01 sl sh ee pl pm ph gl gm gh hl hm hh il im ih jj F7 (just a double-check)
		if(!(buffer[0]==0xf0 && buffer[1]==0x7e && buffer[3]==0x01 && buffer[20]==0xf7))
		{
#ifdef DEBUG_PRINT
			print_hex(buffer,21);
#endif
			throw runtime_error(string(__func__)+" -- invalid data in expected Dump Header");
		}


		sysExChannel=buffer[2];

		const int waveformId=bytesToInt(0,buffer[5],buffer[4]);

		const int bitRate=buffer[6];

		const unsigned sampleRate=  1000000000/bytesToInt(buffer[9],buffer[8],buffer[7]);

		// may need to take into account the bitRate when it's not 16 ???
		const sample_pos_t length=bytesToInt(buffer[12],buffer[11],buffer[10]);

				// offset in words
			// may need to take into account the bitRate when it's not 16 ???
		const sample_pos_t loopStart=bytesToInt(buffer[15],buffer[14],buffer[13]);
		const sample_pos_t loopStop=bytesToInt(buffer[18],buffer[17],buffer[16]);
		const int loopType=buffer[19];

//#ifdef DEBUG_PRINT
		printf("---------\n");
		printf("sysExChannel: %d\n",sysExChannel);
		printf("waveformId: %d\n",waveformId);
		printf("bitRate: %d\n",bitRate);
		printf("sampleRate: %d\n",sampleRate);
		printf("length: %d\n",length);
		printf("loopStart: %d\n",loopStart);
		printf("loopStop: %d\n",loopStop);
		printf("loopType: %d\n",loopType);
		printf("---------\n");
//#endif


		if(sampleRate<4000 || sampleRate>96000)
			throw runtime_error(string(__func__)+" -- an unlikely sample rate of "+istring(sampleRate));
		if(bitRate!=16)
			throw runtime_error(string(__func__)+" -- only 16bit data supported, not "+istring(bitRate)+"bit -- I have no way of testing non-16bit data, so contact me if you have a device that can test it");

		if(isDevice)
		{ // send WAIT message
#ifdef DEBUG_PRINT
			printf("header received... sending WAIT\n");
#endif
			unsigned char buffer[]={0xf0,0x7e,0,0x7c,0,0xf7};
			buffer[2]=sysExChannel;
			buffer[4]=0;
			write(fd,buffer,6);
		}

#ifdef DEBUG_PRINT
		printf("creating working file\n");
#endif
		sound->createWorkingPoolFile(filename,sampleRate,1,length);

		// add some labelled cues about the loop points
		sound->addCue("Loop Start",loopStart,false); 
		sound->addCue("Loop Stop",loopStop,false); 

		// set some misc data for later saving
		{
			TPoolAccesser<int16_t,CSound::PoolFile_t> a=sound->getGeneralDataAccesser<int16_t>("SDS Data");
			a.clear();
			a.append(3);
			a[0]=sysExChannel;
			a[1]=waveformId;
			a[2]=loopType;
		}

		if(isDevice)
		{ // send ACK message to continue after the WAIT
#ifdef DEBUG_PRINT
			printf("sending ACK\n");
#endif
			unsigned char buffer[]={0xf0,0x7e,0,0x7f,0,0xf7};
			buffer[2]=sysExChannel;
			buffer[4]=0;
			write(fd,buffer,6);
		}

#ifdef DEBUG_PRINT
		printf("reading data...\n");
#endif

		CStatusBar statusBar(_("Downloading Sample"),0,length,true);

		sample_pos_t lengthRead=0;
		int expectedPacketNumber=0;
		while(lengthRead<length)
		{
			// read 127 byte "Data Packet": F0 7E cc 02 kk [120 bytes here] ll F7
			static const int dataPacketPattern[]={0xf0,0x7e,-1,0x02,-1};
			static const int dataPacketPatternLen=sizeof(dataPacketPattern)/sizeof(*dataPacketPattern);
			l=wait_for_pattern(fd,buffer,dataPacketPattern,dataPacketPatternLen,2 *1000);
			if(l==dataPacketPatternLen)
				l+=full_read(fd,buffer+l,127-l);
		
#ifdef DEBUG_PRINT
			printf("packet read: %d\n",l);
			print_hex(buffer,127);
#endif
			if(l!=127)
			{
				Warning(string(__func__)+" -- Data Dump was cut short -- keeping what was received");
				break;
			}
			
			// expecting: F0 7E cc 02 kk [120 bytes here] ll F7 (just a double-check)
			if(!(buffer[0]==0xf0 && buffer[1]==0x7e && buffer[3]==0x02 && buffer[126]==0xf7))
			{
#ifdef DEBUG_PRINT
				print_hex(buffer,127);
#endif
				Warning(string(__func__)+" -- invalid Data Packet received");
				break;
			}

			if(buffer[4]!=expectedPacketNumber)
			{
#ifdef DEBUG_PRINT
				print_hex(buffer,127);
#endif
				Warning(string(__func__)+" -- invalid packet number in Data Packet received");
				break;
			}

			/* ??? the ACK doesn't seem to work with my K2500, maybe the write isn't finishing 
			if(isDevice)
			{ // send WAIT message
#ifdef DEBUG_PRINT
				printf("sending WAIT\n");
#endif
				unsigned char buffer[]={0xf0,0x7e,0,0x7c,0,0xf7};
				buffer[2]=sysExChannel;
				buffer[4]=expectedPacketNumber;
				write(fd,buffer,6);
			}
			*/

#ifdef DEBUG_PRINT
			printf("processing data\n");
#endif
			// read the 120 bytes worth of sample data
				// assuming 16bit unsigned data from MIDI device
			{
				CRezPoolAccesser dest=sound->getAudio(0);
				sample_pos_t k=0;
				unsigned char *ptr=buffer+5;
				for(int t=0;t<120 && lengthRead<length;t+=3)
				{
					// bits are left-justified in 3 byte sections except that bit 7 is unused in each byte
					int16_t s=( (((int16_t)ptr[t+0]))<<9 ) + ( (((int16_t)ptr[t+1]))<<2 ) + (((int16_t)ptr[t+2])>>5);
					dest[lengthRead++]=convert_sample<int16_t,sample_t>(s-0x8000);
				}

				// could perform checksum verification
			}

			if(isDevice)
			{ // send ACK message
#ifdef DEBUG_PRINT
				printf("sending ACK\n");
#endif
				unsigned char buffer[]={0xf0,0x7e,0,0x7f,0,0xf7};
				buffer[2]=sysExChannel;
				buffer[4]=expectedPacketNumber;
				write(fd,buffer,6);
			}

			if(statusBar.update(lengthRead))
			{ // cancelled
				if(lengthRead<=0)
					ret=false;
				else
					Message(_("Keeping what was received"));

				if(isDevice)
				{ // send CANCEL message
					unsigned char buffer[6]={0xf0,0x7e,0,0x7d,0,0xf7};
					buffer[2]=sysExChannel;
					buffer[4]=expectedPacketNumber;
					write(fd,buffer,6);
				}
				break;
			}

			// next packet
			expectedPacketNumber= (expectedPacketNumber+1)%128;
		}

		close(fd);
		return ret;
	}
	catch(...)
	{
		if(isDevice && sysExChannel>=0)
		{ // send CANCEL message
			unsigned char buffer[6]={0xf0,0x7e,0,0x7d,0,0xf7};
			buffer[2]=sysExChannel;
			buffer[4]=0;
			write(fd,buffer,6);
		}
		close(fd);
		throw;
	}

}

bool CMIDISDSSoundTranslator::onSaveSound(const string filename,const CSound *sound,const sample_pos_t saveStart,const sample_pos_t saveLength,bool useLastUserPrefs) const
{
	int sysExChannel=-1;
	int packetSeq=0;
	
	const bool isDevice=CPath(filename).isDevice();

	bool ret=true;

#ifdef DEBUG_PRINT
	printf("opening: %s\n",filename.c_str());
#endif
	int fd=open(filename.c_str(),!isDevice ? O_RDWR|O_CREAT|O_TRUNC : O_WRONLY, 0777);
	if(fd==-1)
		throw runtime_error(string(__func__)+" -- error opening file: "+filename+" -- "+strerror(errno));
	try
	{
		unsigned char buffer[256];
		int l;

		// prompt for some necessary things
		sysExChannel=	sound->containsGeneralDataPool("SDS Data") ? sound->getGeneralDataAccesser<int16_t>("SDS Data")[0] : -1;
		int waveformId=	sound->containsGeneralDataPool("SDS Data") ? sound->getGeneralDataAccesser<int16_t>("SDS Data")[1] : -1;
		int loopType=	sound->containsGeneralDataPool("SDS Data") ? sound->getGeneralDataAccesser<int16_t>("SDS Data")[2] : -1;
		int dumpChannel=0;	// ??? which channel in the audio file to dump to the receiver

		if(!gFrontendHooks->promptForSaveMIDISampleDump(sysExChannel,waveformId,loopType))
		{
			close(fd);
			return false;
		}

		size_t cueIndex;

		resendHeader:

		// build dump header information
		buffer[0]=0xf0;
		buffer[1]=0x7e;
		buffer[2]=sysExChannel;			// cc
		buffer[3]=0x01;
		buffer[4]=waveformId&0x7f;		// sl
		buffer[5]=(waveformId>>7)&0x7f;		// sh
		buffer[6]=16;				// ee
		const int samplePeriod=1000000000/sound->getSampleRate();
		buffer[7]=samplePeriod&0x7f;		// pl
		buffer[8]=(samplePeriod>>7)&0x7f;	// pm
		buffer[9]=(samplePeriod>>14)&0x7f;	// ph
		const sample_pos_t length=saveLength; // should check size limit ???
		buffer[10]=length&0x7f;			// gl
		buffer[11]=(length>>7)&0x7f;		// gm
		buffer[12]=(length>>14)&0x7f;		// gh
		const sample_pos_t loopStart=sound->containsCue("Loop Start",cueIndex) ? sound->getCueTime(cueIndex)-saveStart : 0;
		buffer[13]=loopStart&0x7f;		// hl
		buffer[14]=(loopStart>>7)&0x7f;		// hm
		buffer[15]=(loopStart>>14)&0x7f;	// hh
		const sample_pos_t loopStop=sound->containsCue("Loop Stop",cueIndex) ? sound->getCueTime(cueIndex)-saveStart : saveLength-1;
		buffer[16]=loopStop&0x7f;		// il
		buffer[17]=(loopStop>>7)&0x7f;		// im
		buffer[18]=(loopStop>>14)&0x7f;		// ih
		buffer[19]=loopType;			// jj
		buffer[20]=0xf7;


		// write dump header
		l=write(fd,buffer,21);
		if(l!=21)
		{
			int err=errno;
			throw runtime_error(string(__func__)+" -- error writing DUMP HEADER -- "+strerror(err));
		}

		if(isDevice)
		{
			// wait for up to 2 seconds for a handshaking response
			int waitTime=2 *1000;

			waitForResponse:

			// expect either an ACK, NAK, WAIT or CANCEL
			static int dataPacketPattern[]={0xf0,0x7e,0,-1,-1,0xf7};
			dataPacketPattern[2]=sysExChannel;
			static const int dataPacketPatternLen=sizeof(dataPacketPattern)/sizeof(*dataPacketPattern);
			l=wait_for_pattern(fd,buffer,dataPacketPattern,dataPacketPatternLen,waitTime);
			if(l!=dataPacketPatternLen)
			{
				if(l==0 && waitTime==(2 *1000)) // 2 seconds are up, so start a non-handshaking dump
					goto startDumping;
				throw runtime_error(string(__func__)+" -- invalid or no response after sending DUMP HEADER");
			}
			/* not that important I guess
			if(buffer[4]!=0)
				throw runtime_error(string(__func__)+" -- invalid packet sequence in response from DUMP HEADER");
			*/

			if(buffer[3]==0x7f)
			{ // ACK
#ifdef DEBUG_PRINT
				printf("DUMP HEADER -- ACK\n");
#endif
				goto startDumping;
			}
			else if(buffer[3]==0x7e)
			{ // NAK
#ifdef DEBUG_PRINT
				printf("DUMP HEADER -- NAK\n");
#endif
				goto resendHeader;
			}
			else if(buffer[3]==0x7d)
			{ // CANCEL
#ifdef DEBUG_PRINT
				printf("DUMP HEADER -- CANCEL\n");
#endif
				close(fd);
				return false;
			}
			else if(buffer[3]==0x7c)
			{ // WAIT
#ifdef DEBUG_PRINT
				printf("DUMP HEADER -- WAIT\n");
#endif
				waitTime=3600 *1000; // wait longer now for the same things
				goto waitForResponse;
			}
			else
				throw runtime_error(string(__func__)+" -- invalid response from DUMP HEADER -- "+istring((int)buffer[3]));
		}

		startDumping:

		const CRezPoolAccesser src=sound->getAudio(dumpChannel);

		#define SAMPLES_PER_PACK 40 // only 40 16bit samples can fit in the packed 120 7bit bytes
		const int lastPacketSeq=saveLength/SAMPLES_PER_PACK;

		CStatusBar statusBar(isDevice ? _("Uploading Sample") : _("Dumping Sample"),0,lastPacketSeq,true);

		while(packetSeq<=lastPacketSeq)
		{
			buffer[0]=0xf0;
			buffer[1]=0x7e;
			buffer[2]=sysExChannel;
			buffer[3]=0x02;
			buffer[4]=packetSeq%128;

			const int len=packetSeq<lastPacketSeq ? SAMPLES_PER_PACK : saveLength%SAMPLES_PER_PACK;
			unsigned char *b=buffer+5;
			for(int t=0;t<len;t++)
			{
				int s=(int)convert_sample<sample_t,int16_t>(src[saveStart+(packetSeq*SAMPLES_PER_PACK)+t])+0x8000;
				(*(b++))=(s>>9)&0x7f;
				(*(b++))=(s>>2)&0x7f;
				(*(b++))=s&0x3;
			}
			// pad with zeros if necessary
			for(int t=len;t<SAMPLES_PER_PACK;t++)
			{
				(*(b++))=0;
				(*(b++))=0;
				(*(b++))=0;
			}

			// calc checksum
			unsigned char ll=0;
			for(int t=0;t<125;t++)
				ll^=buffer[t];
			ll&=0x7f;

			buffer[125]=ll;
			buffer[126]=0xf7;

			l=write(fd,buffer,127);
			if(l!=127)
			{
				int err=errno;
				throw runtime_error(string(__func__)+" -- error writing DUMP PACKET -- "+strerror(err));
			}

			if(isDevice)
			{
				// wait 20ms for an ACK, NAK or CANCEL
				static int dataPacketPattern[]={0xf0,0x7e,0,-1,-1,0xf7};
				dataPacketPattern[2]=sysExChannel;
				static const int dataPacketPatternLen=sizeof(dataPacketPattern)/sizeof(*dataPacketPattern);
				l=wait_for_pattern(fd,buffer,dataPacketPattern,dataPacketPatternLen,20);
				if(l!=dataPacketPatternLen)
				{
					if(l==0) // 20ms are up, so continue a non-handshaking dump
						goto keepGoing;
					throw runtime_error(string(__func__)+" -- invalid or no response after sending DUMP HEADER");
				}
				if(buffer[3]==0x7f)
				{ // ACK
#ifdef DEBUG_PRINT
				printf("DUMP PACKET -- ACK\n");
#endif
					// keep going, don't really care about what packet was ACKed
				}
				else if(buffer[3]==0x7e)
				{ // NAK
#ifdef DEBUG_PRINT
				printf("DUMP PACKET -- NAK\n");
#endif
					//set packetSeq back to start resending from the NAKed packet
					//however this isn't the most efficient way.. I suppose I should
					//have a 128 element array of buffers to be able to resend old 
					//packets at ease without having to resend unnessarily possibly
					//ACKed packets
					int kk=buffer[4]; // 0 to 127 of the NAKed packet
					// map kk to the most probable absolute packet number
					kk+=(packetSeq/128)*128; // ??? I don't know if this is really correct, but I'm not too worried about it right now
					packetSeq=kk;
				}
				else if(buffer[3]==0x7d)
				{ // CANCEL
#ifdef DEBUG_PRINT
				printf("DUMP PACKET -- CANCEL\n");
#endif
					ret=false;
					break;
				}
				else
					throw runtime_error(string(__func__)+" -- invalid response from DUMP PACKET -- "+istring((int)buffer[3]));
			}

			keepGoing:

			if(statusBar.update(packetSeq))
			{ // cancelled
				if(isDevice && sysExChannel>=0)
				{ // send CANCEL message
					unsigned char buffer[6]={0xf0,0x7e,0,0x7d,0,0xf7};
					buffer[2]=sysExChannel;
					buffer[4]=packetSeq;
					write(fd,buffer,6);
				}
				ret=false;
				break;
			}

			packetSeq++;
		}

		close(fd);
	}
	catch(...)
	{
		if(isDevice && sysExChannel>=0)
		{ // send CANCEL message
			unsigned char buffer[6]={0xf0,0x7e,0,0x7d,0,0xf7};
			buffer[2]=sysExChannel;
			buffer[4]=packetSeq;
			write(fd,buffer,6);
		}
		close(fd);
		throw;
	}
	
	if(!ret && !isDevice)
		unlink(filename.c_str()); // remove the cancelled file

	return ret;
}


bool CMIDISDSSoundTranslator::handlesExtension(const string extension,const string filename) const
{
	CPath path(filename);
	//         *.sds        or       path is a device and the filename part starts with "midi"
	return extension=="sds" || (path.exists() && path.isDevice() && path.baseName().substr(0,4)=="midi");
}

bool CMIDISDSSoundTranslator::supportsFormat(const string filename) const
{
	printf("file: %s\n",filename.c_str());
	bool ret=false;

	// this won't get called from ASoundTranslator::findTranslator if filename is a device

	//                      0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20  
	// should start with  "F0 7E xx 01 xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx F7"
	FILE *f=fopen(filename.c_str(),"r");
	if(f!=NULL)
	{
		uint8_t buffer[21]={0};
		fread(buffer,21,1,f);
		if(buffer[0]==0xf0 && buffer[1]==0x7e && buffer[3]==0x01 && buffer[20]==0xf7)
			ret=true;
		fclose(f);
	}

	// perhaps if isDevice, then send a NAK and it will resend the data, if not a device, then we can seek
	// we'd have to know the waveformId and request it or wait for something.. but if it wasn't a MIDI 
	// then attempting to read a header would mess things up unless we could rewind some
	
	return ret;
}

const vector<string> CMIDISDSSoundTranslator::getFormatNames() const
{
	vector<string> names;

	names.push_back("MIDI Sample Dump Standard");

	return names;
}

const vector<vector<string> > CMIDISDSSoundTranslator::getFormatFileMasks() const
{
	vector<vector<string> > list;
	vector<string> fileMasks;

	fileMasks.clear();
	fileMasks.push_back("*.sds");
	fileMasks.push_back("midi*");
	list.push_back(fileMasks);

	return list;
}

