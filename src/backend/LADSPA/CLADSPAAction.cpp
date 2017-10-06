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

#include "CLADSPAAction.h"

#include "../CActionSound.h"
#include "../AActionDialog.h"
#include "../ASoundFileManager.h"
#include "../CSoundPlayerChannel.h"
#include "../CLoadedSound.h"

#include <TAutoBuffer.h>

// ??? technically need to lock the size of any sounds that I'm going to access in doActionSizeSafe, except currently there's no way for other sounds to be altered while this is running so I will delay implementing that
/*
 * Some LADSPA actions use fftw.  If ladspa actions are loaded that were compiled with fftw 
 * built using floats/doubles as the native type, then ReZound needs to be built with fftw 
 * configured the same way.  Otherwise the fftw-using LADSPA plugins will produce garbage.
 */

/*
#include <map>
static std::map<string,int> badPlugins;
static int foo() {
	badPlugins["Canyon Delay"];
	return 0;
}
static int foofoo=foo();
*/

CLADSPAAction::CLADSPAAction(const LADSPA_Descriptor *_desc,const AActionFactory *factory,const CActionSound *actionSound,const CActionParameters &_actionParameters) :
	AAction(factory,actionSound),
	desc(_desc),
	actionParameters(_actionParameters),
	channelMapping(actionParameters.getValue<CPluginMapping>("Channel Mapping")),
	soundFileManager(actionParameters.getSoundFileManager()),

	restoreChannelsTempAudioPoolKey(-1),
	origSound(NULL)
{
	//channelMapping.print();
	for(unsigned t=0;t<desc->PortCount;t++)
	{
		const LADSPA_PortDescriptor pd=desc->PortDescriptors[t];
		if(LADSPA_IS_PORT_INPUT(pd) && LADSPA_IS_PORT_AUDIO(pd))
			inputAudioPorts.push_back(t);
		else if(LADSPA_IS_PORT_OUTPUT(pd) && LADSPA_IS_PORT_AUDIO(pd))
			outputAudioPorts.push_back(t);
		else if(LADSPA_IS_PORT_INPUT(pd) && LADSPA_IS_PORT_CONTROL(pd))
			inputControlPorts.push_back(t);
		else if(LADSPA_IS_PORT_OUTPUT(pd) && LADSPA_IS_PORT_CONTROL(pd))
			outputControlPorts.push_back(t);
	}
}

CLADSPAAction::~CLADSPAAction()
{
	if(restoreChannelsTempAudioPoolKey!=-1)
		origSound->removeTempAudioPools(restoreChannelsTempAudioPoolKey);
}

#define BUFFER_SIZE 16384

bool CLADSPAAction::doActionSizeSafe(CActionSound *actionSound,bool prepareForUndo)
{
	// issue this warning to the user
	if(AActionFactory::macroRecorder.isRecording() && soundFileManager->getOpenedCount()>1)
		Warning(_("You are recording a macro.\n\nDue to the way in which LADSPA is implemented, when the macro is played back for things to repeat as expected, the files refered to by the routing parameters (including the file being altered) must be loaded in the same positions as they currently appear in the loaded sound list window.\n\nThis is due to the fact that the saved LADSPA parameters in the macro refer to the index of the loaded file rather than the name of the file.  This could be improved later."));
	// ??? ^^ in order to improve this, I should iterate over the routing parameters and re-write the indexes of loaded sounds before doing everything below.. then at this point I could deal with loading other files as neceesary and comparison of filenames, etc

/* ... THIS IS A COMPLEX AND BLOATED METHOD ... */

	/*
	if(badPlugins.find(desc->Name)!=badPlugins.end())
	{
		if(Question("Plugin: "+string(desc->Name)+"\n\n"+_("Message from the ReZound author:\nI have seen this plugin cause the ReZound process to segfault.  As far as I can tell this is not my fault, but a problem within the plugin.  Please do not report this as a bug\nDo you want to continue?"),yesnoQues)!=yesAns)
			throw EUserMessage("");
	}
	*/

	const sample_pos_t start=actionSound->start;
	const sample_pos_t stop=actionSound->stop;
	const sample_pos_t selectionLength=actionSound->selectionLength();
	const unsigned origActionSoundChannelCount=actionSound->sound->getChannelCount();
	origSound=actionSound->sound; // used only in case the destructor has to free some temp pools

	// determine the index of the actionSound
	size_t actionSoundIndex=0;
	for(size_t t=0;t<soundFileManager->getOpenedCount();t++)
	{
		if(soundFileManager->getSound(t)->sound==actionSound->sound)
		{
			actionSoundIndex=t;
			break;
		}
	}


	// could do: if nothing in the output maps to a channel or in the passThrus dont include it in the moveSelectionToTempPools.. tried it real quick but wasn't trivial
	moveSelectionToTempPools(actionSound,mmSelection,actionSound->selectionLength());

	// create an instance of the plugin 
	LADSPA_Handle instance=desc->instantiate(desc,actionSound->sound->getSampleRate());



	// set up the memory pointers for the plugin's ports
	TAutoBuffer<LADSPA_Data> inputBuffers[MAX_CHANNELS];
	for(size_t t=0;t<inputAudioPorts.size();t++)
	{
		inputBuffers[t].setSize(BUFFER_SIZE);
		desc->connect_port(instance,inputAudioPorts[t],inputBuffers[t]);
	}

	TAutoBuffer<LADSPA_Data> outputBuffers[MAX_CHANNELS];
	for(size_t t=0;t<outputAudioPorts.size();t++)
	{
		outputBuffers[t].setSize(BUFFER_SIZE);
		desc->connect_port(instance,outputAudioPorts[t],outputBuffers[t]);
	}

	TAutoBuffer<LADSPA_Data> controlValues(inputControlPorts.size());
	for(size_t t=0;t<inputControlPorts.size();t++)
	{
		controlValues[t]=actionParameters.getValue<double>(desc->PortNames[inputControlPorts[t]]);
		//printf("parameter:\t%s\t%f\n",desc->PortNames[inputControlPorts[t]],controlValues[t]);
		desc->connect_port(instance,inputControlPorts[t],((LADSPA_Data *)controlValues)+t);
	}

	// I bind to these because some plugins don't check that they haven't been bound to
	TAutoBuffer<LADSPA_Data> unusedOutputValues(outputControlPorts.size());
	for(size_t t=0;t<outputControlPorts.size();t++)
		desc->connect_port(instance,outputControlPorts[t],((LADSPA_Data *)unusedOutputValues)+t);


	// append new channels if requested
	actionSound->sound->addChannels(actionSound->sound->getChannelCount(),channelMapping.outputAppendCount,true);


	// possibly call set_run_adding_gain (if run_adding is not NULL)

		// I suppose if the output audio port count isn't the same as the number of channels then I could try to figure out (rule-based) how the channels should be mapped... 
		// 	if there are n ins and n outs then I should just do round robin on the channels of audio mapping
		// 	if there is 1 in and n (n>1) outs either I should insert n-1 new channels or ask the user
		// 		cause if there's 1 in and 2 outs and the audio already has two channels then I would be turning it into 4
		// 		perhaps if that were the senario then I should run the plugin twice but mix the second run's output onto the first run's output
		// 	if there are n (n>1) inputs and m outputs where m<n then I suppose I should remove channels or just tell the user that if the number of channels in the audio doesn't match the number of inputs then they need to change it to match
		
	
	// this keeps track if the dest channels need to be mixed onto or copied onto
	vector<sample_pos_t> destChannelsWrittenTo;
	for(unsigned t=0;t<actionSound->sound->getChannelCount();t++)
		destChannelsWrittenTo.push_back(0);

	const sample_pos_t chunkCount=(sample_pos_t)sample_fpos_ceil((sample_fpos_t)selectionLength/BUFFER_SIZE);

	for(unsigned a=0;a<channelMapping.inputMappings.size();a++)
	{
		// call activate if it's not NULL
		if(desc->activate)
			desc->activate(instance);

		const vector<vector<CPluginMapping::RInputDesc> > &inputMapping=channelMapping.inputMappings[a];
		const vector<vector<CPluginMapping::ROutputDesc> > &outputMapping=channelMapping.outputMappings[a];

		vector<vector<sample_pos_t> > srcPositions;
		vector<vector<sample_pos_t> > srcStartPositions;
		vector<vector<sample_pos_t> > srcStopPositions;
		vector<vector<bool> > srcProducingSilence;
		for(unsigned t=0;t<inputMapping.size();t++)
		{
			srcStartPositions.push_back(vector<sample_pos_t>());
			srcStopPositions.push_back(vector<sample_pos_t>());
			srcPositions.push_back(vector<sample_pos_t>());
			srcProducingSilence.push_back(vector<bool>());
			
			for(unsigned i=0;i<inputMapping[t].size();i++)
			{
				if(inputMapping[t][i].soundFileManagerIndex==actionSoundIndex) // if it's the action sound, then that's also the destination
				{
					srcStartPositions[t].push_back(0);
					srcStopPositions[t].push_back(selectionLength-1);
					srcPositions[t].push_back(0);
				}
				else
				{
					// use some or all of the sound
					if(inputMapping[t][i].howMuch==CPluginMapping::RInputDesc::hmSelection)
					{
						CSoundPlayerChannel *ch=soundFileManager->getSound(inputMapping[t][i].soundFileManagerIndex)->channel;
						srcStartPositions[t].push_back(ch->getStartPosition());
						srcStopPositions[t].push_back(ch->getStopPosition());
						srcPositions[t].push_back(ch->getStartPosition());
					}
					else // hmAll
					{
						srcStartPositions[t].push_back(0);
						srcStopPositions[t].push_back(soundFileManager->getSound(inputMapping[t][i].soundFileManagerIndex)->sound->getLength()-1);
						srcPositions[t].push_back(0);
					}
				}
				srcProducingSilence[t].push_back(false);
			}
		}


		CStatusBar statusBar(string(desc->Name)+" "+istring(a+1)+"/"+istring(channelMapping.inputMappings.size()),start,stop,true); 

		sample_pos_t destPos=start;
		while(destPos<=stop)
		{
				/* as long as BUFFER_SIZE is < 2bil, this cast to int is fine */
			const int amount=min((stop-destPos)+1,(sample_pos_t)BUFFER_SIZE);

			// build an input chunk
			for(unsigned t=0;t<inputMapping.size();t++)
			{
				// set to silence if nothing will be written to it
				if(inputMapping[t].size()<=0)
					memset(inputBuffers[t],0,BUFFER_SIZE*sizeof(inputBuffers[0][0]));
				
				for(unsigned i=0;i<inputMapping[t].size();i++)
				{
					LADSPA_Data *inputBuffer=inputBuffers[t];
					sample_pos_t srcPos=srcPositions[t][i];
					const float gain=inputMapping[t][i].gain;

					const CRezPoolAccesser src=
						(inputMapping[t][i].soundFileManagerIndex==actionSoundIndex && inputMapping[t][i].channel<origActionSoundChannelCount)
							?
							actionSound->sound->getTempAudio(tempAudioPoolKey,inputMapping[t][i].channel)
							:
							soundFileManager->getSound(inputMapping[t][i].soundFileManagerIndex)->sound->getAudio(inputMapping[t][i].channel)
						;
					
					int k=0;
					int limitedAmount=0;
					if((srcPos+amount)>=srcStopPositions[t][i])
					{ // maybe produce some silence or loop
						if(inputMapping[t][i].wdro==CPluginMapping::RInputDesc::wdroSilence)
						{ // produce silence
								// setting limitedAmount so it will copy below
							limitedAmount=(srcStopPositions[t][i]-srcPos)+1;
							srcProducingSilence[t][i]=true;
						}
						else if(inputMapping[t][i].wdro==CPluginMapping::RInputDesc::wdroLoop)
						{ // looping
							const sample_pos_t start=srcStartPositions[t][i];
							const sample_pos_t stop=srcStopPositions[t][i];
							const sample_pos_t len=(stop-start)+1;
							while(k<amount)
							{
								int l=min((sample_pos_t)amount-(sample_pos_t)k,(stop-srcPos)+1);

								if(i==0)
								{
									for(int j=0;j<l;j++)
										inputBuffer[k++]=convert_sample<sample_t,LADSPA_Data>(src[srcPos++])*gain;
								}
								else
								{
									for(int j=0;j<l;j++)
										inputBuffer[k++]+=convert_sample<sample_t,LADSPA_Data>(src[srcPos++])*gain;
								}


								if(srcPos>stop)
									srcPos=start;
							}
						}
						else
							throw runtime_error(string(__func__)+" -- unhandled wdro type: "+istring(inputMapping[t][i].wdro));
					}
					else
					{
							// setting limitedAmount so it will copy below
						limitedAmount=amount;
					}


					if(i==0)
					{ // first time copy
						for(;k<limitedAmount;k++)
							inputBuffer[k]=convert_sample<sample_t,LADSPA_Data>(src[srcPos++])*gain;
					}
					else
					{ // next times mix
						for(;k<limitedAmount;k++)
							inputBuffer[k]+=convert_sample<sample_t,LADSPA_Data>(src[srcPos++])*gain;
					}

					if(srcProducingSilence[t][i])
					{
						/* or use memset? */
						for(;k<amount;k++)
							inputBuffer[k]=0;
					}

					srcPositions[t][i]=srcPos;
				}
			}


			// process the input chunk
			desc->run(instance,amount);


			// write the output chunk to the destination
			sample_pos_t origDestPos=destPos;
			for(unsigned t=0;t<outputMapping.size();t++)
			{
				CRezPoolAccesser dest=actionSound->sound->getAudio(t);

				for(unsigned i=0;i<outputMapping[t].size();i++)
				{
					LADSPA_Data *outputBuffer=outputBuffers[outputMapping[t][i].channel];
					const float gain=outputMapping[t][i].gain;

					if(destChannelsWrittenTo[t]<chunkCount)
					{ // first time copy
						for(int k=0;k<amount;k++)
							dest[destPos++]=convert_sample<LADSPA_Data,sample_t>(outputBuffer[k]*gain);
						destChannelsWrittenTo[t]++;
					}
					else
					{ // next times mix
						for(int k=0;k<amount;k++)
						{
							dest[destPos]=ClipSample(
								(mix_sample_t)(dest[destPos])
								+
								(convert_sample<LADSPA_Data,sample_t>(outputBuffer[k]*gain))
							);
							destPos++;
						}
					}

					destPos=origDestPos;
				}
			}
			destPos+=amount;

			if(statusBar.update(destPos))
			{
				undoActionSizeSafe(actionSound);
				if(desc->deactivate)
					desc->deactivate(instance);
				desc->cleanup(instance);
				return false;
			}
		}

		// call deactivate if it's not NULL
		if(desc->deactivate)
			desc->deactivate(instance);
	}

	// call cleanup
	desc->cleanup(instance);

	// process passThrus
	for(unsigned destChannel=0;destChannel<channelMapping.passThrus.size();destChannel++)
	{
		const vector<CPluginMapping::RInputDesc> &passThru=channelMapping.passThrus[destChannel];

		for(unsigned t=0;t<passThru.size();t++)
		{
			const CRezPoolAccesser src=
				(passThru[t].soundFileManagerIndex==actionSoundIndex && passThru[t].channel<origActionSoundChannelCount)
					?
					actionSound->sound->getTempAudio(tempAudioPoolKey,passThru[t].channel)
					:
					soundFileManager->getSound(passThru[t].soundFileManagerIndex)->sound->getAudio(passThru[t].channel)
				;

			const float gain=passThru[t].gain;
			
			sample_pos_t srcPosition;
			sample_pos_t srcStartPosition;
			sample_pos_t srcStopPosition;
			if(passThru[t].soundFileManagerIndex==actionSoundIndex) // if it's the action sound, then that's also the destination
			{
				srcStartPosition=0;
				srcStopPosition=selectionLength-1;
				srcPosition=srcStartPosition;
			}
			else
			{
				// use some or all of the sound
				if(passThru[t].howMuch==CPluginMapping::RInputDesc::hmSelection)
				{
					CSoundPlayerChannel *ch=soundFileManager->getSound(passThru[t].soundFileManagerIndex)->channel;
					srcStartPosition=ch->getStartPosition();
					srcStopPosition=ch->getStopPosition();
					srcPosition=srcStartPosition;
				}
				else // hmAll
				{
					srcStartPosition=0;
					srcStopPosition=soundFileManager->getSound(passThru[t].soundFileManagerIndex)->sound->getLength()-1;
					srcPosition=srcStartPosition;
				}
			}

			CRezPoolAccesser dest=actionSound->sound->getAudio(destChannel);

			CStatusBar statusBar(_("Pass Through ")+istring(destChannel+1)+"/"+istring(channelMapping.passThrus.size()),start,stop,true); 

			if(!destChannelsWrittenTo[destChannel])
			{ /*??? this could be an optimization where I check this flag all down below, but instead I've taken the easy way of just ALWAYS mixing below (which will mix onto silence the first time) */
				actionSound->sound->silenceSound(destChannel,start,selectionLength,true,true);
				destChannelsWrittenTo[destChannel]++;
			}

			// ---------------------------------------------------

			sample_pos_t destPos=start;
			if(passThru[t].wdro==CPluginMapping::RInputDesc::wdroSilence)
			{
				// this is the amount to write total
				const sample_pos_t limitedAmount=min(stop-start+1,srcStopPosition-srcStartPosition+1);

				// now do it in 100 steps
				for(sample_pos_t k=0;k<limitedAmount;k++,destPos++,srcPosition++)
				{
					dest[destPos]=ClipSample((mix_sample_t)dest[destPos]+src[srcPosition]*gain);
					if(statusBar.update(destPos))
					{
						undoActionSizeSafe(actionSound);
						return false;
					}
				}
			}
			else if(passThru[t].wdro==CPluginMapping::RInputDesc::wdroLoop)
			{
				const sample_pos_t loopCount=(stop-start+1)/(srcStopPosition-srcStartPosition+1);
				for(sample_pos_t l=0;l<loopCount;l++)
				{
					for(srcPosition=srcStartPosition;srcPosition<=srcStopPosition;srcPosition++,destPos++)
					{
						dest[destPos]=ClipSample((mix_sample_t)dest[destPos]+src[srcPosition]*gain);
						if(statusBar.update(destPos))
						{
							undoActionSizeSafe(actionSound);
							return false;
						}
					}
				}
				const sample_pos_t remainder=(stop-start+1)%(srcStopPosition-srcStartPosition+1);

				srcPosition=srcStartPosition;
				for(sample_pos_t r=0;r<remainder;r++,destPos++,srcPosition++)
				{
					dest[destPos]=ClipSample((mix_sample_t)dest[destPos]+src[srcPosition]*gain);
					if(statusBar.update(destPos))
					{
						undoActionSizeSafe(actionSound);
						return false;
					}
				}
			}
			else
				throw runtime_error(string(__func__)+" -- unhandled wdro type: "+istring(passThru[t].wdro));
		}
	}

	for(unsigned t=0;t<destChannelsWrittenTo.size();t++)
	{
		if(destChannelsWrittenTo[t])
			actionSound->sound->invalidatePeakData(t,actionSound->start,actionSound->stop);
		else
			actionSound->sound->silenceSound(t,start,selectionLength,true,true);
	}

	// use remove channels on end if requested
	if(channelMapping.outputRemoveCount>0 && channelMapping.outputRemoveCount<actionSound->sound->getChannelCount())
	{
		bool doChannels[MAX_CHANNELS]={0};
		for(unsigned t=actionSound->sound->getChannelCount()-channelMapping.outputRemoveCount;t<actionSound->sound->getChannelCount();t++)
			doChannels[t]=true;
		restoreChannelsTempAudioPoolKey=actionSound->sound->moveChannelsToTemp(doChannels);
	}


	// set the new selection points (only necessary if the length of the sound has changed)
	//actionSound->stop=actionSound->start+selectionLength-1;
	
	if(!prepareForUndo)
		freeAllTempPools();

	return true;
}

AAction::CanUndoResults CLADSPAAction::canUndo(const CActionSound *actionSound) const
{
	return curYes;
}

void CLADSPAAction::undoActionSizeSafe(const CActionSound *actionSound)
{
	// remove append channels if we did append some
	if(preactionChannelCount<actionSound->sound->getChannelCount())
		actionSound->sound->removeChannels(preactionChannelCount,actionSound->sound->getChannelCount()-preactionChannelCount);

	// restore removed channels if we did remove some
	if(restoreChannelsTempAudioPoolKey!=-1)
	{
		// create doChannels as it appeared when we called moveChannelsToTemp()
		bool doChannels[MAX_CHANNELS]={0};
		for(unsigned t=preactionChannelCount-channelMapping.outputRemoveCount;t<preactionChannelCount;t++)
			doChannels[t]=true;

		actionSound->sound->moveChannelsFromTemp(restoreChannelsTempAudioPoolKey,doChannels);
		restoreChannelsTempAudioPoolKey=-1;
	}

	restoreSelectionFromTempPools(actionSound,actionSound->start,actionSound->selectionLength());
}


// --------------------------------------------------

CLADSPAActionFactory::CLADSPAActionFactory(const LADSPA_Descriptor *_desc,AActionDialog *channelSelectDialog,AActionDialog *dialog) :
	AActionFactory(_desc->Name,string(_desc->Maker)+"\n"+string(_desc->Copyright),channelSelectDialog,dialog),
	desc(_desc)
{
}

CLADSPAActionFactory::~CLADSPAActionFactory()
{
	delete dialog;
}

CLADSPAAction *CLADSPAActionFactory::manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const
{
	return new CLADSPAAction(
		desc,
		this,
		actionSound,
		*actionParameters
	);
}


