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

#include "ClibvorbisSoundTranslator.h"

#ifdef HAVE_LIBVORBIS

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include "vorbis/codec.h"
#include "vorbis/vorbisfile.h"


#include <stdexcept>
#include <typeinfo>

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

#if 0
#include <unistd.h> // for unlink

#include "AStatusComm.h"

static int getUserNotesMiscType(int type)
{
	if(type==AF_FILE_AIFFC || type==AF_FILE_AIFF)
		return(AF_MISC_ANNO);
	else if(type==AF_FILE_WAVE)
		return(AF_MISC_ICMT);

	return(0);
}

#endif

ClibvorbisSoundTranslator::ClibvorbisSoundTranslator()
{
}

ClibvorbisSoundTranslator::~ClibvorbisSoundTranslator()
{
}

static string errorMessage;
static void errorFunction(long code,const char *msg)
{
	errorMessage=msg;
}

	// ??? could just return a CSound object an have used the one constructor that takes the meta info
	// ??? but, then how would I be able to have createWorkingPoolFileIfExists
void ClibvorbisSoundTranslator::onLoadSound(const string filename,CSound *sound) const
{
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

#define REALLOC_FILE_SIZE (1024*1024)

		// ??? make sure it's not more than MAX_LENGTH
			// ??? just truncate the length
		sample_pos_t size=REALLOC_FILE_SIZE;  // start with an initial size unless there's a way to get the final length from vorbisfile
		if(size<0)
			throw(runtime_error(string(__func__)+" -- libvorbis reports the data length as "+istring(size)));

		sound->createWorkingPoolFile(filename,sampleRate,channelCount,size);



		// load the user notes
		char **userNotes=ov_comment(&vf,-1)->user_comments;
		string _userNotes;
		while(*userNotes)
		{
			_userNotes+=(*userNotes);
			_userNotes+="\n";
			userNotes++;
		}
		sound->setUserNotes(_userNotes);


		// load the audio data
		for(unsigned t=0;t<channelCount;t++)
			accessers[t]=new CRezPoolAccesser(sound->getAudio(t));


		// ??? if sample_t is not the bit type that ogg is going to return I should write some convertion functions... that are overloaded to go to and from several types to and from sample_t
			// ??? this needs to match the type of sample_t
			// ??? float is supported by ov_read_float
			// 	
		#define bitRate 16 // ??? I guess ogg is always giving us 16bit pcm

		TAutoBuffer<sample_t> buffer((size_t)((bitRate/8)*4096/sizeof(sample_t)));
		sample_pos_t pos=0;

		unsigned long count=CPath(filename).getSize();
		BEGIN_PROGRESS_BAR("Loading Sound",ftell(f),count);

		int eof=0;
		int current_section;
		for(;;)
		{
			const long ret=ov_read(&vf,(char *)((sample_t *)buffer),buffer.getSize()*sizeof(sample_t),ENDIAN,bitRate/8,1,&current_section);
			if(ret==0)
				break;
			else if(ret<0)
			{ // error in the stream... may not be fatal however
			}
			else // if(ret>0)
			{
				const int chunkSize= ret/((bitRate/8)*channelCount);

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

	sound->setIsModified(false);
}

void ClibvorbisSoundTranslator::onSaveSound(const string filename,CSound *sound) const
{
	throw(runtime_error(string(__func__)+" unimplemented"));
#if 0
	int fileType;

	const string extension=istring(CPath(filename).extension()).lower();
	if(extension=="aiff")
		fileType=AF_FILE_AIFF;
	else if(extension=="wav")
		fileType=AF_FILE_WAVE;
	else if(extension=="snd" || extension=="au")
		fileType=AF_FILE_NEXTSND;
	else if(extension=="sf")
		fileType=AF_FILE_BICSF;
	else
		throw(runtime_error(string(__func__)+" -- unhandled extension for filename: "+filename));

	AFfilesetup setup=afNewFileSetup();
	try
	{
		// ??? all the following parameters need to be passed in somehow as the export format
		// 	??? can easily do it with AFrontendHooks
		afInitFileFormat(setup,fileType); 
		afInitByteOrder(setup,AF_DEFAULT_TRACK,AF_BYTEORDER_LITTLEENDIAN); 			// ??? I would actually want to pass how the user wants to export the data...
		afInitChannels(setup,AF_DEFAULT_TRACK,sound->getChannelCount());
		afInitSampleFormat(setup,AF_DEFAULT_TRACK,AF_SAMPFMT_TWOSCOMP,sizeof(int16_t)*8); 	// ??? I would actually want to pass how the user wants to export the data... int16_t matching AF_SAMPFMT_TWOSCOMP
		afInitRate(setup,AF_DEFAULT_TRACK,sound->getSampleRate()); 				// ??? I would actually want to pass how the user wants to export the data... doesn't actual do any conversion right now (perhaps I could patch it for them)
		afInitCompression(setup,AF_DEFAULT_TRACK,AF_COMPRESSION_NONE);
			//afInitInitCompressionParams(setup,AF_DEFAULT_TRACK, ... );
		afInitFrameCount(setup,AF_DEFAULT_TRACK,sound->getLength());				// ??? I suppose I could allow the user to specify something shorter on the dialog

		saveSoundGivenSetup(filename,sound,setup,fileType);

		afFreeFileSetup(setup);
	}
	catch(...)
	{
		afFreeFileSetup(setup);
		throw;
	}

	errorMessage="";
	afSetErrorHandler(errorFunction);

	const unsigned channelCount=sound->getChannelCount();
	const unsigned sampleRate=sound->getSampleRate();
	const sample_pos_t size=sound->getLength();

	// setup for saving the cues (except for positions)
	TAutoBuffer<int> markIDs(sound->getCueCount());
	if(sound->getCueCount()>0)
	{
		if(!afQueryLong(AF_QUERYTYPE_MARK,AF_QUERY_SUPPORTED,fileFormatType,0,0))
			Warning("This format does not support saving cues");
		else
		{
			for(size_t t=0;t<sound->getCueCount();t++)
				markIDs[t]=t;

			afInitMarkIDs(initialSetup,AF_DEFAULT_TRACK,markIDs,(int)sound->getCueCount());
			for(size_t t=0;t<sound->getCueCount();t++)
			{
				// to indicate if the cue is anchored we append to the name:
				// "|+" or "|-" whether it is or isn't
				const string name=sound->getCueName(t)+"|"+(sound->isCueAnchored(t) ? "+" : "-");
				afInitMarkName(initialSetup,AF_DEFAULT_TRACK,markIDs[t],name.c_str());

				// can't save position yet because that function requires a file handle, but
				// we can't move this code after the afOpenFile, or it won't save cues at all
				//afSetMarkPosition(h,AF_DEFAULT_TRACK,markIDs[t],sound->getCueTime(t));
			}
		}
	}



	// prepare to save the user notes
	const string userNotes=sound->getUserNotes();
	int userNotesMiscID=1;
	const int userNotesMiscType=getUserNotesMiscType(fileFormatType);
	if(userNotes.length()>0)
	{
		/* this is not implemented in libvorbis yet
		if(!afQueryLong(AF_QUERYTYPE_MISC,AF_QUERY_SUPPORTED,fileFormatType,0,0))
			Warning("This format does not support saving user notes");
		else
		*/
		{
			afInitMiscIDs(initialSetup,&userNotesMiscID,1);
			afInitMiscType(initialSetup,userNotesMiscID,userNotesMiscType);
			afInitMiscSize(initialSetup,userNotesMiscID,userNotes.length());
		}
	}

	unlink(filename.c_str());
	AFfilehandle h=afOpenFile(filename.c_str(),"w",initialSetup);
	if(h==AF_NULL_FILEHANDLE)
		throw(runtime_error(string(__func__)+" -- error opening '"+filename+"' for writing -- "+errorMessage));

	// ??? this if set may not completly handle all possibilities
	int ret;
	if(typeid(sample_t)==typeid(int16_t))
		ret=afSetVirtualSampleFormat(h,AF_DEFAULT_TRACK,AF_SAMPFMT_TWOSCOMP,sizeof(sample_t)*8);
	else if(typeid(sample_t)==typeid(float))
		ret=afSetVirtualSampleFormat(h,AF_DEFAULT_TRACK,AF_SAMPFMT_FLOAT,sizeof(sample_t)*8);
	else
	{
		afCloseFile(h);
		throw(runtime_error(string(__func__)+" -- unhandled sample_t format"));
	}

	if(ret!=0)
		throw(runtime_error(string(__func__)+" -- error setting virtual format -- "+errorMessage));

	if(afSetVirtualByteOrder(h,AF_DEFAULT_TRACK,AF_BYTEORDER_LITTLEENDIAN))
		throw(runtime_error(string(__func__)+" -- error setting virtual byte order -- "+errorMessage));
	afSetVirtualChannels(h,AF_DEFAULT_TRACK,channelCount);

	CRezPoolAccesser *accessers[MAX_CHANNELS]={0};
	try
	{
		for(unsigned t=0;t<channelCount;t++)
			accessers[t]=new CRezPoolAccesser(sound->getAudio(t));

		// save the audio data
		TAutoBuffer<sample_t> buffer((size_t)(channelCount*4096));
		sample_pos_t pos=0;
		AFframecount count=size/4096+1;
		BEGIN_PROGRESS_BAR("Saving Sound",0,count);
		for(AFframecount t=0;t<count;t++)
		{
			const int chunkSize=  (t==count-1 ) ? size%4096 : 4096;
			if(chunkSize!=0)
			{
				for(unsigned c=0;c<channelCount;c++)
				{
					for(int i=0;i<chunkSize;i++)
						buffer[i*channelCount+c]=(*(accessers[c]))[pos+i];
				}
				if(afWriteFrames(h,AF_DEFAULT_TRACK,(void *)buffer,chunkSize)!=chunkSize)
					throw(runtime_error(string(__func__)+" -- error writing audio data -- "+errorMessage));
				pos+=chunkSize;
			}

			UPDATE_PROGRESS_BAR(t);
		}

		END_PROGRESS_BAR();
		
		// write the cue's positions
		if(afQueryLong(AF_QUERYTYPE_MARK,AF_QUERY_SUPPORTED,fileFormatType,0,0))
		{
			for(size_t t=0;t<sound->getCueCount();t++)
				afSetMarkPosition(h,AF_DEFAULT_TRACK,markIDs[t],sound->getCueTime(t));
		}

		// write the user notes
		if(userNotes.length()>0)
		{
			/* this is not implemented in libvorbis yet
			if(!afQueryLong(AF_QUERYTYPE_MISC,AF_QUERY_SUPPORTED,fileFormatType,0,0))
				Warning("This format does not support saving user notes");
			else
			*/
			{
				afWriteMisc(h,userNotesMiscID,(void *)userNotes.c_str(),userNotes.length());
			}
		}


		afCloseFile(h);

		for(unsigned t=0;t<MAX_CHANNELS;t++)
		{
			delete accessers[t];
			accessers[t]=NULL;
		}
	}
	catch(...)
	{
		// attempt to close the file ??? and free the setup???

		endAllProgressBars();

		for(unsigned t=0;t<MAX_CHANNELS;t++)
			delete accessers[t];

		throw;
	}
#endif
}


bool ClibvorbisSoundTranslator::handlesExtension(const string extension) const
{
	return(extension=="ogg");
}

bool ClibvorbisSoundTranslator::supportsFormat(const string filename) const
{
#if 0
	int fd=open(filename.c_str(),O_RDONLY);
	int _e=errno;
	if(fd==-1)
		throw(runtime_error(string(__func__)+" -- error opening file '"+filename+"' -- "+strerror(_e)));

	int implemented=0;
	int id=afIdentifyNamedFD(fd,filename.c_str(),&implemented);
	close(fd);

	return(implemented);
#endif
	printf("need to implement this if possible\n");
	return(false);
}

const vector<string> ClibvorbisSoundTranslator::getFormatNames() const
{
	vector<string> names;

	names.push_back("OggVorbis");

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
