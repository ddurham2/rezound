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
 * Much of this source code is adapted from the ogg/vorbis example source code files
 *
 * Something I found to be a bad thing is that if you load and save the same ogg file
 * over and over with a bit of a low bit rate compression, then the artifacts of the
 * compression noise become quite louder and louder.
 *
 */

#include "ClibvorbisSoundTranslator.h"

#ifdef HAVE_LIBVORBIS

#include <stdio.h>	// for fopen/fclose
#include <errno.h>
#include <string.h>	// for strerror()
#include <ctype.h>

#include <time.h>	// for time()
#include <stdlib.h>	// for rand() and atoll

#include "vorbis/codec.h"

#ifdef HAVE_LIBVORBISFILE
#include "vorbis/vorbisfile.h"
#endif

#ifdef HAVE_LIBVORBISENC
#include "vorbis/vorbisenc.h"
#endif


#include <stdexcept>
#include <typeinfo>
#include <vector>

#include <istring>
#include <TAutoBuffer.h>

#include <CPath.h>

#include "CSound.h"
#include "AStatusComm.h"


#ifdef WORDS_BIGENDIAN
	#define ENDIAN 1
#else
	#define ENDIAN 0
#endif

ClibvorbisSoundTranslator::ClibvorbisSoundTranslator()
{
}

ClibvorbisSoundTranslator::~ClibvorbisSoundTranslator()
{
}

	// ??? could just return a CSound object an have used the one constructor that takes the meta info
	// ??? but, then how would I be able to have createWorkingPoolFileIfExists
void ClibvorbisSoundTranslator::onLoadSound(const string filename,CSound *sound) const
{
#ifdef HAVE_LIBVORBISFILE
	FILE *f=fopen(filename.c_str(),"rb");
	int err=errno;
	if(f==NULL)
		throw(runtime_error(string(__func__)+" -- error opening '"+filename+"' -- "+strerror(err)));
	
	OggVorbis_File vf;
	if(ov_open(f, &vf, NULL, 0) < 0)
	{
		fclose(f);
		throw(runtime_error(string(__func__)+" -- input does not appear to be an Ogg bitstream"));
	}

	CRezPoolAccesser *accessers[MAX_CHANNELS]={0};
	try
	{

		vorbis_info *vi=ov_info(&vf,-1);

		unsigned channelCount=vi->channels;
		if(channelCount<=0 || channelCount>MAX_CHANNELS) // ??? could just ignore the extra channels
			throw(runtime_error(string(__func__)+" -- invalid number of channels in audio file: "+istring(channelCount)+" -- you could simply increase MAX_CHANNELS in CSound.h"));

		unsigned sampleRate=vi->rate;
		if(sampleRate<4000 || sampleRate>96000)
			throw(runtime_error(string(__func__)+" -- an unlikely sample rate of "+istring(sampleRate)));

		#define REALLOC_FILE_SIZE (1024*1024/4)

		// ??? make sure it's not more than MAX_LENGTH
			// ??? just truncate the length
		sample_pos_t size=REALLOC_FILE_SIZE;  // start with an initial size unless there's a way to get the final length from vorbisfile
		if(size<0)
			throw(runtime_error(string(__func__)+" -- libvorbis reports the data length as "+istring(size)));

		sound->createWorkingPoolFile(filename,sampleRate,channelCount,size);

		/*
		 * As best as I can tell, the ogg stream format for user comments
		 * is simply an array of strings.  Now the API presets an interface
		 * for supporting 'tags' but it looked to me like all this is is a
		 * tag name follow by an '=' constitutes a tag a value (which 
		 * follows the '=').  So I will use these 'tags' to loading and 
		 * saving of cues, and the usernotes will be made up of any tags in
		 * the user comments that aren't my cue tags.
		 */

		vorbis_comment *oc=ov_comment(&vf,-1);
		char **commentsArray;
		vector<int> commentsThatWereCues;

		// load the cues
		commentsArray=oc->user_comments;
		sound->clearCues();
		for(int t=0;t<oc->comments;t++)
		{
			if(strncmp(*commentsArray,"CUE=",4)==0)
			{
				string cuePos;
				string cueName;
				bool isAnchored=false;

				// parse "[0-9]+[+-][A-Z]*\0" which should follow the "CUE="
				// which is the sample position, +(is anchored) -(is not), cue name
				char *cueDesc=*commentsArray+4;
				int state=0;
				for(;;)
				{
					const char c=*cueDesc;
					cueDesc++;
					if(c==0)
						break;

					switch(state)
					{
					case 0:
						if(isdigit(c))
						{
							cuePos.append(&c,1);
							state=1;
						}
						else
							goto notACue;
						break;

					case 1:
						if(isdigit(c))
							cuePos.append(&c,1);
						else if(c=='-')
						{
							isAnchored=false;
							state=2;
						}
						else if(c=='+')
						{
							isAnchored=true;
							state=2;
						}
						else
							goto notACue;
						break;

					case 2:
						cueName.append(&c,1);
						break;

					default:
						goto notACue;
					}
				}
				
				if(cueName=="")
					cueName=sound->getUnusedCueName("cue"); // give it a unique name

				try
				{
					sound->addCue(cueName,(sample_pos_t)atoll(cuePos.c_str()),isAnchored);
				}
				catch(exception &e)
				{ // don't make an error adding a cue cause the whole file not to load
					Warning("file: '"+filename+"' -- "+e.what());
				}

				commentsThatWereCues.push_back(t);
			}

			notACue:

			commentsArray++;
		}

		// load the user notes
		commentsArray=oc->user_comments;
		string userNotes;
		size_t ctwcPos=0;
		for(int t=0;t<oc->comments;t++)
		{
			if(commentsThatWereCues.empty() || commentsThatWereCues[ctwcPos]!=t)
			{ // comment was not previously found to be a cue definition
				userNotes+=(*commentsArray);
				userNotes+="\n";
			}
			else // ignore comment if it was a cue definition
				ctwcPos++;

			commentsArray++;
		}
		sound->setUserNotes(userNotes);


		// load the audio data
		for(unsigned t=0;t<channelCount;t++)
			accessers[t]=new CRezPoolAccesser(sound->getAudio(t));


		// ??? if sample_t is not the bit type that ogg is going to return I should write some convertion functions... that are overloaded to go to and from several types to and from sample_t
			// ??? this needs to match the type of sample_t
			// ??? float is supported by ov_read_float
		#define BIT_RATE 16

		TAutoBuffer<sample_t> buffer((size_t)((BIT_RATE/8)*4096/sizeof(sample_t)));
		sample_pos_t pos=0;

		unsigned long count=CPath(filename).getSize();
		BEGIN_PROGRESS_BAR("Loading Sound",ftell(f),count);

		int eof=0;
		int current_section;
		for(;;)
		{
			const long ret=ov_read(&vf,(char *)((sample_t *)buffer),buffer.getSize()*sizeof(sample_t),ENDIAN,BIT_RATE/8,1,&current_section);
			if(ret==0)
				break;
			else if(ret<0)
			{ // error in the stream... may not be fatal however
			}
			else // if(ret>0)
			{
				const int chunkSize= ret/((BIT_RATE/8)*channelCount);

				if((pos+REALLOC_FILE_SIZE)>sound->getLength())
					sound->addSpace(sound->getLength(),REALLOC_FILE_SIZE);

				for(unsigned c=0;c<channelCount;c++)
				{
					for(int i=0;i<chunkSize;i++)
						(*(accessers[c]))[pos+i]=buffer[i*channelCount+c];
				}
				pos+=chunkSize;
			}

			UPDATE_PROGRESS_BAR(ftell(f));
		}

		// remove any extra allocated space
		if(sound->getLength()>pos)
			sound->removeSpace(pos,sound->getLength()-pos);

		END_PROGRESS_BAR();

		for(unsigned t=0;t<MAX_CHANNELS;t++)
		{
			delete accessers[t];
			accessers[t]=NULL;
		}
	}
	catch(...)
	{
		endAllProgressBars();

		ov_clear(&vf); // closes file too

		for(unsigned t=0;t<MAX_CHANNELS;t++)
			delete accessers[t];
		throw;
	}

	ov_clear(&vf); // closes file too
#else
	throw(runtime_error(string(__func__)+" -- loading Ogg Vorbis is not enabled -- missing libvorbisfile"));
#endif
}

void ClibvorbisSoundTranslator::onSaveSound(const string filename,CSound *sound) const
{
#ifdef HAVE_LIBVORBISENC

	vorbis_info vi;


	const unsigned channelCount=sound->getChannelCount();
	const unsigned sampleRate=sound->getSampleRate();
	const sample_pos_t size=sound->getLength();

	vorbis_info_init(&vi);

	/*  constant bit rate 
	int maxEncBitRate,normEncBitRate,minEncBitRate;
	maxEncBitRate=normEncBitRate=minEncBitRate=128000; // ??? need to prompt the user for these settings
	if(vorbis_encode_init(&vi,channelCount,sampleRate,maxEncBitRate,normEncBitRate,minEncBitRate)<0) // constant bitrate
	*/
	/* or variable bitrate (with .7 quality) ??? need to prompt the user for the quality setting */
	if(vorbis_encode_init_vbr(&vi,channelCount,sampleRate,0.7)<0)
		throw(runtime_error(string(__func__)+" -- error initializing the Ogg Vorbis encoder engine"));

	vorbis_comment vc;
	vorbis_comment_init(&vc);


	// save the cues
	for(size_t t=0;t<sound->getCueCount();t++)
	{
		const string cueDesc="CUE="+istring(sound->getCueTime(t))+(sound->isCueAnchored(t) ? "+" : "-")+sound->getCueName(t);
		vorbis_comment_add(&vc,(char *)cueDesc.c_str());
	}


	// save user notes
	// since the user notes may contain "tagname=value" then I break each line in the user notes string into a separate comment
	const string userNotes=sound->getUserNotes();
	string comment;
	for(size_t t=0;t<userNotes.length();t++)
	{
		const char c=userNotes[t];
		if(c=='\n')
		{
			vorbis_comment_add(&vc,(char *)comment.c_str());
			comment="";
		}
		else
			comment.append(&c,1);
	}
	if(comment!="")
		vorbis_comment_add(&vc,(char *)comment.c_str());
	

	/* set up the analysis state and auxiliary encoding storage */
	vorbis_dsp_state vd;
	vorbis_block vb;
	vorbis_analysis_init(&vd,&vi);
	vorbis_block_init(&vd,&vb);

	ogg_page og; // one raw packet of data for decode

	/* set up our packet->stream encoder */
	/* pick a random serial number; that way we can more likely build chained streams just by concatenation */
	ogg_stream_state os; // take physical pages, weld into a logical stream of packets
	srand(time(NULL));
	ogg_stream_init(&os,rand());

	FILE *f=fopen(filename.c_str(),"wb");
	int err=errno;
	if(f==NULL)
		throw(runtime_error(string(__func__)+" -- error opening '"+filename+"' -- "+strerror(err)));

	/* 
	 * Vorbis streams begin with three headers; the initial header 
	 * (with most of the codec setup parameters) which is mandated 
	 * by the Ogg bitstream spec.  The second header holds any 
	 * comment fields.  The third header holds the bitstream 
	 * codebook.  We merely need to make the headers, then pass 
	 * them to libvorbis one at a time; libvorbis handles the 
	 * additional Ogg bitstream constraints 
	 */
	{
		ogg_packet header;
		ogg_packet header_comm;
		ogg_packet header_code;

		vorbis_analysis_headerout(&vd,&vc,&header,&header_comm,&header_code);
		ogg_stream_packetin(&os,&header); /* automatically placed in its own page */
		ogg_stream_packetin(&os,&header_comm);
		ogg_stream_packetin(&os,&header_code);

		/* 
		 * We don't have to write out here, but doing so makes streaming 
		 * much easier, so we do, flushing ALL pages. This ensures the actual
		 * audio data will start on a new page
		 */
		for(;;)
		{
			if(ogg_stream_flush(&os,&og)==0)
				break;
			fwrite(og.header,1,og.header_len,f);
			fwrite(og.body,1,og.body_len,f);
		}
	}


	CRezPoolAccesser *accessers[MAX_CHANNELS]={0};
	for(unsigned t=0;t<channelCount;t++)
		accessers[t]=new CRezPoolAccesser(sound->getAudio(t));

	try
	{
		#define CHUNK_SIZE 4096

		ogg_packet op; // one Ogg bitstream page.  Vorbis packets are inside

		sample_pos_t pos=0;

		const sample_pos_t chunkCount= (size/CHUNK_SIZE) + ((size%CHUNK_SIZE)!=0 ? 1 : 0);

		BEGIN_PROGRESS_BAR("Saving Sound",0,chunkCount);

		for(sample_pos_t t=0;t<=chunkCount;t++)
		{
			if(t==chunkCount)
			{ // after last chunk
				/* 
				 * Tell the library we're at end of stream so that it can handle 
				 * the last frame and mark end of stream in the output properly 
				 */
				vorbis_analysis_wrote(&vd,0);
			}
			else
			{ // give some data to the encoder

				// get the buffer to submit data
				float **buffer=vorbis_analysis_buffer(&vd,CHUNK_SIZE);

				// copy data into buffer
				const unsigned chunkSize= (t==chunkCount-1) ? (size%CHUNK_SIZE) : CHUNK_SIZE;
				for(unsigned c=0;c<channelCount;c++)
				for(unsigned i=0;i<chunkSize;i++)
					buffer[c][i]=(float)((*(accessers[c]))[pos+i])/(float)MAX_SAMPLE;
				pos+=chunkSize;

				// tell the library how much we actually submitted
				vorbis_analysis_wrote(&vd,chunkSize);
			}

			/* 
			 * vorbis does some data preanalysis, then divvies up blocks for 
			 * more involved (potentially parallel) processing.  Get a single 
			 * block for encoding now 
			 */
			while(vorbis_analysis_blockout(&vd,&vb)==1)
			{
				// analysis, assume we want to use bitrate management
				vorbis_analysis(&vb,NULL);
				vorbis_bitrate_addblock(&vb);

				while(vorbis_bitrate_flushpacket(&vd,&op))
				{
					// weld the packet into the bitstream
					ogg_stream_packetin(&os,&op);

					// write out pages (if any)
					for(;;)
					{
						if(ogg_stream_pageout(&os,&og)==0)
							break;
						fwrite(og.header,1,og.header_len,f);
						fwrite(og.body,1,og.body_len,f);

						if(ogg_page_eos(&og))
							break;
					}
				}
			}

			UPDATE_PROGRESS_BAR(t);
		}

		END_PROGRESS_BAR();

		for(unsigned t=0;t<MAX_CHANNELS;t++)
		{
			delete accessers[t];
			accessers[t]=NULL;
		}

		/* clean up.  vorbis_info_clear() must be called last */
		ogg_stream_clear(&os);
		vorbis_block_clear(&vb);
		vorbis_dsp_clear(&vd);
		vorbis_comment_clear(&vc);
		vorbis_info_clear(&vi);

		fclose(f);
	}
	catch(...)
	{
		ogg_stream_clear(&os);
		vorbis_block_clear(&vb);
		vorbis_dsp_clear(&vd);
		vorbis_comment_clear(&vc);
		vorbis_info_clear(&vi);

		fclose(f);

		endAllProgressBars();

		for(unsigned t=0;t<MAX_CHANNELS;t++)
			delete accessers[t];

		throw;
	}

#else
	throw(runtime_error(string(__func__)+" -- saving Ogg Vorbis is not enabled -- missing libvorbisenc"));
#endif
}


bool ClibvorbisSoundTranslator::handlesExtension(const string extension) const
{
	return(extension=="ogg");
}

bool ClibvorbisSoundTranslator::supportsFormat(const string filename) const
{
#ifdef HAVE_LIBVORBISFILE
	FILE *f=fopen(filename.c_str(),"rb");
	if(f==NULL)
		return(false);
	
	OggVorbis_File vf;
	if(ov_open(f, &vf, NULL, 0) < 0)
	{
		fclose(f);
		return(false);
	}

	ov_clear(&vf);
	return(true);
#else
	return(false);
#endif
}

const vector<string> ClibvorbisSoundTranslator::getFormatNames() const
{
	vector<string> names;

	names.push_back("Ogg Vorbis");

	return(names);
}

const vector<vector<string> > ClibvorbisSoundTranslator::getFormatExtensions() const
{
	vector<vector<string> > list;
	vector<string> extensions;

	extensions.clear();
	extensions.push_back("ogg");
	list.push_back(extensions);

	return(list);
}

#endif // HAVE_LIBVORBIS
