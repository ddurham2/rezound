#ifndef __CMacroRecorder_H__
#define __CMacroRecorder_H__

#include <string>
using namespace std;

class CNestedDataFile;
class CActionParameters;
class ASoundFileManager;
class CLoadedSound;

class CMacroRecorder
{
public:
	CMacroRecorder();
	virtual ~CMacroRecorder();

	bool isRecording() const;

	void startRecording(CNestedDataFile *file,const string macroName);
	void stopRecording();

	static void removeMacro(CNestedDataFile *file,const string macroName);

private:
	bool recording;
	CNestedDataFile *file;
	string macroName;
	string key;
	unsigned actionCount;

	friend class AActionFactory;
		// this is called by AActionFactory::performAction() before the action is actually performed .. a hook point for all actions performed
		// loadedSound is optional and is used when the selection positions are needed for calculating how to set the selection positions at playback
	void pushAction(const string actionName,const CActionParameters *actionParameters,CLoadedSound *loadedSound);

	friend void undo(ASoundFileManager *);
		// this is called when an action is undone but we recorded it in the macro
	void popAction(const string actionName);

};

#endif
