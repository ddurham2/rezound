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

/* Later I should support loading and saving the user notes and cues, etc.. but I'm not even
 * done defining my own, and the libFLAC++ documentation is a little hard to follow about metadata
 * so I'll ommit this for now 
 */

#include "CFLACSoundTranslator.h"

#if defined(HAVE_LIBFLACPP)

#include <unistd.h>

#include <string>
#include <stdexcept>
#include <utility>

#include <FLAC++/all.h>

#include <istring>
#include <CPath.h>
#include <TAutoBuffer.h>

#include "CSound.h"
#include "AStatusComm.h"

CFLACSoundTranslator::CFLACSoundTranslator()
{
}

CFLACSoundTranslator::~CFLACSoundTranslator()
{
}




class MyFLACDecoderFile : public FLAC::Decoder::File
{
public:
	#define REALLOC_FILE_SIZE (1024*1024/4)

	MyFLACDecoderFile(const string _filename,CSound *_sound) :
		File(),

		filename(_filename),
		sound(_sound),

		statusBar(_("Loading Sound"),0,CPath(filename).getSize(false),true),

		sampleRate(0),
		channelCount(0),
		bitRate(0),

		pos(0)
	{
		for(unsigned t=0;t<MAX_CHANNELS;t++)
			accessers[t]=NULL;

		set_filename(filename.c_str());

		set_metadata_ignore_all();
		//set_metadata_respond(FLAC__METADATA_TYPE_VORBIS_COMMENT);
		//set_metadata_respond(FLAC__METADATA_TYPE_CUESHEET);

		State s=init();
		if(s!=FLAC__FILE_DECODER_OK)
			throw runtime_error(string(__func__)+" -- "+s.as_cstring());
	}

	virtual ~MyFLACDecoderFile()
	{
		finish();

		// remove space we didn't need to add (because we're adding in chunks)
		sound->removeSpace(pos,sound->getLength()-pos);

		for(unsigned t=0;t<channelCount;t++)
			delete accessers[t];
	}

protected:

	::FLAC__StreamDecoderWriteStatus write_callback(const ::FLAC__Frame *frame, const FLAC__int32 *const buffer[])
	{
		if(sound->isEmpty())
		{
#warning an exception thrown from here causes an abort because it is running in a thread??? why? I dunno, might want to talk to the FLAC guys
			sampleRate=get_sample_rate();
			channelCount=get_channels();
			bitRate=get_bits_per_sample();

			#warning I need to translate from different bit rates that might come back from get_bits_per_sample ... I am not sure what they would be
			//printf("br %d\n",bitRate);
			//printf("cc %d\n",channelCount);
			//printf("sr %d\n",sampleRate);
			if(bitRate!=16 && bitRate!=24 && bitRate!=32)
			{
				Error("unhandled bitrate! (data will be incorrect): "+istring(bitRate));
			}

			if(channelCount<=0 || channelCount>MAX_CHANNELS) // ??? could just ignore the extra channels
				throw runtime_error(string(__func__)+" -- invalid number of channels in audio file: "+istring(channelCount)+" -- you could simply increase MAX_CHANNELS in CSound.h");

			if(sampleRate<100 || sampleRate>197000)
				throw runtime_error(string(__func__)+" -- an unlikely sample rate of "+istring(sampleRate));

			sound->createWorkingPoolFile(filename,sampleRate,channelCount,REALLOC_FILE_SIZE);

			for(unsigned t=0;t<channelCount;t++)
				accessers[t]=new CRezPoolAccesser(sound->getAudio(t));
		}


		const sample_pos_t sampleframes_read=frame->header.blocksize;

		if((pos+sampleframes_read)>sound->getLength())
			sound->addSpace(sound->getLength(),REALLOC_FILE_SIZE);

		for(unsigned i=0;i<channelCount;i++)
		{
			const FLAC__int32 *src=buffer[i];
			CRezPoolAccesser &dest=*(accessers[i]);

			if(bitRate==16)
			{
				for(unsigned t=0;t<sampleframes_read;t++)
					// the FLAC__int32 src seems to actually be 16bit (maybe this changes depending on the file?)
					dest[pos+t]=convert_sample<int16_t,sample_t>(src[t]);
			}
			else if(bitRate==24)
			{
				for(unsigned t=0;t<sampleframes_read;t++)
				{
					int24_t sd;
					sd.set(src[t]);
					dest[pos+t]=convert_sample<int24_t,sample_t>(sd);
				}
			}
			else if(bitRate==32)
			{
				for(unsigned t=0;t<sampleframes_read;t++)
					dest[pos+t]=convert_sample<int32_t,sample_t>(src[t]);
			}
			else
			{ // warned user already
			}
		}

		pos+=sampleframes_read;


		// update status bar and detect user cancel
		FLAC__uint64 filePosition;
		FLAC__file_decoder_get_decode_position(decoder_, &filePosition);
		return statusBar.update(filePosition) ? FLAC__STREAM_DECODER_WRITE_STATUS_ABORT : FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
	}

	void metadata_callback(const ::FLAC__StreamMetadata *metadata)
	{
		/*
		if(sound->isEmpty())
		{
			printf("ignoring metadata (until we know the parameters)\n");
			return;
		}

		printf("accepting metadata\n");
		*/
	}

	void error_callback(::FLAC__StreamDecoderErrorStatus status)
	{
		/*
		printf("error\n");
		*/
	}


private:

	const string filename;
	CSound *sound;

	CStatusBar statusBar;

	unsigned sampleRate;
	unsigned channelCount;
	unsigned bitRate;


	sample_pos_t pos;

	CRezPoolAccesser *accessers[MAX_CHANNELS];
};

bool CFLACSoundTranslator::onLoadSound(const string filename,CSound *sound) const
{
	MyFLACDecoderFile f(filename,sound);
	return f.process_until_end_of_file();
}





class MyFLACEncoderFile : public FLAC::Encoder::File
{
public:
	CStatusBar statusBar;
	bool cancelled;

	MyFLACEncoderFile(sample_pos_t saveLength) :
		File(),
		statusBar(_("Saving Sound"),0,saveLength,true),
		cancelled(false)
	{
	}

	virtual ~MyFLACEncoderFile()
	{
	}

protected:
	void progress_callback(FLAC__uint64 bytes_written, FLAC__uint64 samples_written, unsigned frames_written, unsigned total_frames_estimate)
	{
		cancelled|=statusBar.update(samples_written);
	}
};

bool CFLACSoundTranslator::onSaveSound(const string filename,const CSound *sound,const sample_pos_t saveStart,const sample_pos_t saveLength,bool useLastUserPrefs) const
{
	int bitRate=0;

	if(sound->getCueCount()>0 || sound->getUserNotes()!="")
	{
		if(Question(_("Saving user notes or cues is unimplemented for FLAC\nDo you wish to continue?"),yesnoQues)!=yesAns)
			return false;
	}

	MyFLACEncoderFile f(saveLength);

	f.set_filename(filename.c_str());

	f.set_channels(sound->getChannelCount());

	/* ??? needs to be a user choice */
	f.set_bits_per_sample(16);
	bitRate=16;

	f.set_sample_rate(sound->getSampleRate());

	// calling set_do_mid_side_stereo set_loose_mid_side_stereo   requires the sound to have only 2 channels
	// maybe call set_loose_mid_side_stereo if it makes for better compression ???
	
	//f.set_metadata(...) // ??? to do to set cues and user notes, etc


	MyFLACEncoderFile::State s=f.init();
	if(s==FLAC__STREAM_ENCODER_OK)
	{
		#define BUFFER_SIZE 65536
		TAutoBuffer<FLAC__int32> buffers[MAX_CHANNELS];
		FLAC__int32 *_buffers[MAX_CHANNELS];

		for(unsigned t=0;t<sound->getChannelCount();t++)
		{
			buffers[t].setSize(BUFFER_SIZE);
			_buffers[t]=buffers[t]; // get point of buffer[t] to be able to pass to f.process()
		}


		sample_pos_t pos=0;
		while(pos<saveLength)
		{
			const sample_pos_t len=min((sample_pos_t)BUFFER_SIZE,saveLength-pos);
			for(unsigned i=0;i<sound->getChannelCount();i++)
			{
				const CRezPoolAccesser src=sound->getAudio(i);
				FLAC__int32 *dest=buffers[i];

				if(bitRate==16)
				{
					for(sample_pos_t t=0;t<len;t++)
						dest[t]=convert_sample<sample_t,int16_t>(src[t+pos+saveStart]);
				}
				else
					throw runtime_error(string(__func__)+" -- internal error -- unhandled bitRate: "+istring(bitRate));
			}

			if(!f.process(_buffers,len))
			{
				const int errNO=errno;
				f.finish();
				throw runtime_error(string(__func__)+" -- error writing FLAC file -- "+strerror(errNO));
			}

			if(f.cancelled)
			{
				f.finish();
				unlink(filename.c_str());
				return false;
			}

			pos+=len;
		}

		f.finish();

		return true;
	}
	else
		throw runtime_error(string(__func__)+" -- error creating FLAC encoder -- "+s.as_cstring());

}


bool CFLACSoundTranslator::handlesExtension(const string extension,const string filename) const
{
	return extension=="flac" || extension=="fla";
}

bool CFLACSoundTranslator::supportsFormat(const string filename) const
{
	// implementing this would not be convenient right now, I wish there were a simple function for it in the API
	return false;
}

const vector<string> CFLACSoundTranslator::getFormatNames() const
{
	vector<string> names;

	names.push_back("FLAC");

	return names;
}

const vector<vector<string> > CFLACSoundTranslator::getFormatFileMasks() const
{
	vector<vector<string> > list;
	vector<string> fileMasks;

	fileMasks.clear();
	fileMasks.push_back("*.flac");
	fileMasks.push_back("*.fla");
	list.push_back(fileMasks);

	return list;
}

#endif // HAVE_LIBFLACPP
