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

#ifndef __AAction_H__
#define __AAction_H__

#include "../../config/common.h"

class AActionFactory;
class AAction;



#include <string>
#include <vector>

#include "CActionSound.h"
#include "AStatusComm.h"

class CLoadedSound;
class CSoundPlayerChannel;
class AActionDialog;

class CActionParameters;


#include "CSound.h" // really only necesary because of CSound::RCue

typedef TPoolAccesser<sample_t,CSound::PoolFile_t > CRezClipboardPoolAccesser;



/*
 * This class is used to manufacture one of the actual derivations of AAction
 * It takes care of knowing when to show dialogs for the and managing the undo
 * stack
 */
class AActionFactory
{
public:
	virtual ~AActionFactory() { }

	// Invoked on this factory action object, sets in motion what needs to be done to perform the associated action.
	// The invoker of the action should provide the CLoadedSound object and a CActionParameters object to use to put
	//    parameters to the action into.  Perhaps, if there are no dialogs to show, the ivoker of the action would have
	//    supplied all the necessary parameters to the action in the CActionParameters object passed.  And sometimes it
	//    is necessary to supply some of the parameters manually and some by a dialog  (The AActionDialog::show() method 
	//    should provide the parameters when obtained from a dialog).
	// The derived object should have supplied this base object with the dialogs to potentially show based on the arguments:
	// - if showChannelSelect is true, then the dialog will be shown that allows the user to choose which channels the action to
	// 	(Note: additionally it could allow the user to check, 'all data' or 'selected data' to apply the action to but would
	// 	  need to modifie the given action sound's start and stop positions)
	// - if advancedMode is true, then the alternate advancedDialog will be shown to the user.  But if the hasAdvancedMode()
	//      returns false, then no action will be taken
	//
	// performAction returns true if the action was performed or false if they cancelled any of the dialog windows
	//
	bool performAction(CLoadedSound *loadedSound,CActionParameters *actionParameters,bool showChannelSelectDialog,bool advancedMode);

	const string &getName() const;
	const string &getDescription() const;

	/*
	 * The showChannelSelectDialog parameter:
	 *	   The frontend implementation can create a channel select dialog
	 *	   that allows the user to choose which channels from the given sound
	 *	   to apply the action to.  It should also allow the user to choose to
	 *	   apply the action to only the selected data or all the data; it should
	 *	   actionSound->selectAll() if the users chose to apply to all the data,
	 *	   otherwise, actionSound already is selecting the original selection 
	 *	   region.
	 *
	 *	   The derived class's show method should modify show's actionSound 
	 *	   parameter's doChannel values
	 */



protected:

	// - hasAdvancedMode can be passed as true if something is to be done when performAction's advancedMode parameter is true
	// - Otherwise, it will say "this action has no advanced mode" so the user doesn't think something different than normal is
	//   happening
	// - lockForResize can be passed as false to avoid locking the CSound object for resize if the action doesn't need that lock to, but a lockSize will be obtained anyway
	AActionFactory(const string actionName,const string actionDescription,const bool hasAdvancedMode,AActionDialog *channelSelectDialog,AActionDialog *normalDialog,AActionDialog *advancedDialog,bool willResize=true,bool crossfadeEdgesIsApplicable=true);


	// this method can be overridden to do and setup before any dialog is shown for doing the action
	virtual bool doPreActionSetup() { return(true); }

	/*
	 * - The derived class is to implement this method to 'new' the derived AAction class and construct it
	 *   with the given parameters.  The frontend dialog and backend code will just have to agree on how
	 *   to use this void * to pass data... I sugguest a struct which is nested within the AAction derivation
	 *   which the dialog will declare and object of
	 * - The advancedMode parameter just indicates whether the advancedMode parameter was true to the performAction() method
	 */
			// ??? shouldn't I pass whether advancedMode should be used?... because we can have advancedMode without an advancedDialog... if so, how would i know when to use it
	virtual AAction *manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const=0;

protected:
	const string actionName;
	const string actionDescription;

	AActionDialog * const channelSelectDialog;
	AActionDialog * const normalDialog;
	AActionDialog * const advancedDialog;

private: 
	const bool hasAdvancedMode;
	const bool willResize;
	const bool crossfadeEdgesIsApplicable;

};


/*
 * Since the frontend may try to redraw while we are wanting to show a modal dialog 
 * within doActionSizeSafe, the frontend may be locked from doing so since AAction
 * gets s resizeLock on the sound... So inorder to show a message (and bail out), 
 * doActionSizeSafe should just throw one of these.
 */
#include <stdexcept>
class EUserMessage : public runtime_error { public: EUserMessage(const char *msg) : runtime_error(msg) { } };

/*
    This class is a base class for all effect and edit actions.  doActionSizeSafe
    and undoActionSizeSafe should be overridden and defined.  canUndo should return
    curYes or curNo as to whether the action can be undone so we can warn the user and 
    know not to put this action on the stack for later undo-ing.

    Most everything is private because AActionFactory actually controls the AAction 
    derived object.
*/
class AAction
{
public:

	virtual ~AAction();

	// - undoes the action (if canUndo()==curYes && prepareForUndo was true)
	// - if channel is passed, restores the selection positions from before the action executed if a channel was given to doAction
	// - note, willResize includes not only changing the length of the data, but also moving data into undo pools and such
	// 	// ??? when/if this is private don't have default parameters
	void undoAction(CSoundPlayerChannel *channel=NULL,bool willResize=true);


	// this CPoolFile object needs to be instantiated at startup and destroyed at
	// shutdown and should be created as a file in the user-specified temp directory
	// it is used for storing copied/cut data
	static CSound::PoolFile_t *clipboardPoolFile;
	static void clearClipboard();
	static unsigned getClipboardChannelCount(); // ??? may supply a clipboard ID
	static sample_pos_t getClipboardLength(); // ??? may supply a clipboard ID

protected:

	AAction(const CActionSound &actionSound);

	enum CanUndoResults
	{
		curYes,
		curNo,
		curNA 	// cases like 'copy' where undo is an action, but it's not to go on the undo stack
	};

	// ??? perhaps if this particular action isn't going to change the size, then we don't need a changeSize lock, but one that ensures that the size won't change
	// so, still call it 'SizeSafe', but in the constructor, indicate which lock will be needed for this action
	// still some actions may not need any lock.. so create an enum

	// These are called "SizeSafe" because when these are called, there will 
	// be a lock in place that makes it okay to change the size of the audio
	//   The actionSound is a parameter rather than the implementation using a protected
	//   data-member because sometimes I want to invoke the method with different value
	//
	// doActionSizeSafe should return true if the action was done or false if not
	virtual bool doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)=0;
	virtual void undoActionSizeSafe(const CActionSound &actionSound)=0;
	virtual CanUndoResults canUndo(const CActionSound &actionSound) const=0;

	// this method can be overloaded to return false, if the action does not warrent saving the file (i.e. the user will not be prompted)
	virtual bool doesWarrantSaving() const;


	// ??? perhaps I could make a more intelligent system that saves and restores structure as well as data
	// but take for example the CChangeRate action... we could surely backup the selection, but when we
	// restore, how would restoreUndoSelection know where to remove or add space in order to make it the original
	// again.... perhaps we could compare the start and stop positions of the actionSound parameter from before 
	// doActionSizeSafe and after undoActionSizeSafe... but all actions may not work this way... 
	// when many more action are already implemented, perhaps then I can know a way to generalize the undo system
	// into something that simplifies the process of creating actions' undo ability

	// These methods can be used by most actions to make a copy of and restore the data they are going to modify
	// However, this only backs-up and restores according to the positions indicated by the actionSound object and
	// the backupMode parameter.
	// Also, when restoring, the any space to the channels that needs to be added or removed should be done by the
	// action's implementation; these methods only restore the data, not the structure
	//
	// The restoreUndoSelection does infact call sound->invalidatePeakData for the data it restored
	enum MoveModes
	{
		mmInvalid,
		mmSelection,
		mmAllButSelection,
		mmAll
	};
	void moveSelectionToTempPools(const CActionSound &actionSound,const MoveModes moveMode,sample_pos_t replaceLength=0,sample_pos_t fudgeFactor=0);
	void restoreSelectionFromTempPools(const CActionSound &actionSound,sample_pos_t removeWhere=0,sample_pos_t removeLength=0);
	// frees what moveSelectionToTempPool created
	void freeAllTempPools();

	int tempAudioPoolKey;
	int tempAudioPoolKey2; // used incase MoveMethod was mmAllButSelection

private:
	friend class AActionFactory;

	// - does the action to the sound specified by the action sound this was constructed with
	// - channel can be passed to restore the selection positions if the sound is undone
	// - if prepareForUndo is false, then the derivation shouldn't make provisions to be able to undo the action
	// - sets the selection of the channel to the selection of the resulting actionSound when done
	// - note, willResize includes not only changing the length of the data, but also moving data into undo pools and such
	bool doAction(CSoundPlayerChannel *channel,bool prepareForUndo,bool willResize,bool crossfadeEdgesIsApplicable);

	CanUndoResults canUndo() const;


	void setSelection(sample_pos_t start,sample_pos_t stop,CSoundPlayerChannel *channel);


	CActionSound actionSound;		// the CActionSound this action was constructed with
	bool done;				// true if this action has already been done

	// members used to keep track of undo backup information
	sample_pos_t oldSelectStart,oldSelectStop;	// the selection positions when doAction was called to restore if the action is undone
	/*
	TDimableList<int,string> undoPoolNames; // a list of arbitrary names of pools in the undo CPoolFile unique across all actions, but the int key is unique to this action
	void removeUndoPools();			// removes all undo pools in the global undo CPoolFile that this action created
	*/

	bool origIsModified;
	
	// members used by moveSelectionToTempPool and restoreSelectionFromTempPool
	MoveModes restoreMoveMode;		// the method used to backup for undoing the action
	sample_pos_t restoreWhere,restoreLength;
	sample_pos_t restoreLength2;
	sample_pos_t restoreTotalLength;
	//CActionSound restoreActionSound;	// the CActionSound passed to backupUndoSelection
	vector<CSound::RCue> restoreCues;

	
	// members used for crossfading and uncrossfading the edges after an action
	void crossfadeEdges(CActionSound &actionSound);
	void uncrossfadeEdges();
	bool didCrossfadeEdges;
	int tempCrossfadePoolKeyStart;
	int tempCrossfadePoolKeyStop;
	sample_pos_t crossfadeStart;
	sample_pos_t crossfadeStartLength;
	sample_pos_t crossfadeStop;
	sample_pos_t crossfadeStopLength;
	
	

	/*
	// used to get unique pool names for the global undo CPoolFile across all actions
	static const string getUniquePoolName();
	static int poolNameCounter;
	*/

};

#endif
