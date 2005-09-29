/*=======================================================================*/
/** @file   mayasvncmd.cpp



    @author Gregg A. Tavares
*/
/**************************** i n c l u d e s ****************************/

#include <maya/MIOStream.h>
#include <maya/MPxCommand.h>
#include <maya/MArgDatabase.h>
#include <maya/MSyntax.h>
#include <maya/MFileIO.h>
#include <maya/MGlobal.h>
#include <maya/MFnPlugin.h>
#include <maya/MSceneMessage.h>
#include <maya/MStringArray.h>

#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <map>
#include <string>

using std::map;
using std::string;

// used for case insensitive maps
struct ltstr
{
  bool operator()(const string& s1, const string& s2) const
  {
    return stricmp(s1.c_str(), s2.c_str()) < 0;
  }
};

#include "dbgprint.h"

/*************************** c o n s t a n t s ***************************/

#undef SCENEMSGOP
#define SCENEMSGS \
	SCENEMSGOP( 0, kSceneUpdate ,"called after any operation that changes which files are loaded  ")	\
	SCENEMSGOP( 0, kBeforeNew ,"called before a File > New operation  ")	\
	SCENEMSGOP( 0, kAfterNew ,"called after a File > New operation  ")	\
	SCENEMSGOP( 0, kBeforeImport ,"called before a File > Import operation  ")	\
	SCENEMSGOP( 0, kAfterImport ,"called after a File > Import operation  ")	\
	SCENEMSGOP( 0, kBeforeOpen ,"called before a File > Open operation  ")	\
	SCENEMSGOP( 0, kAfterOpen ,"called after a File > Open operation  ")	\
	SCENEMSGOP( 0, kBeforeExport ,"called before a File > Export operation  ")	\
	SCENEMSGOP( 0, kAfterExport ,"called after a File > Export operation  ")	\
	SCENEMSGOP( 0, kBeforeSave ,"called before a File > Save (or SaveAs) operation  ")	\
	SCENEMSGOP( 0, kAfterSave ,"called after a File > Save (or SaveAs) operation  ")	\
	SCENEMSGOP( 0, kBeforeReference ,"called before a File > Reference operation  ")	\
	SCENEMSGOP( 0, kAfterReference ,"called after a File > Reference operation  ")	\
	SCENEMSGOP( 0, kBeforeRemoveReference ,"called before a File > RemoveReference operation  ")	\
	SCENEMSGOP( 0, kAfterRemoveReference ,"called after a File > RemoveReference operation  ")	\
	SCENEMSGOP( 0, kBeforeImportReference ,"called before a File > ImportReference operation  ")	\
	SCENEMSGOP( 0, kAfterImportReference ,"called after a File > ImportReference operation  ")	\
	SCENEMSGOP( 0, kBeforeExportReference ,"called before a File > ExportReference operation  ")	\
	SCENEMSGOP( 0, kAfterExportReference ,"called after a File > ExportReference operation  ")	\
	SCENEMSGOP( 0, kBeforeUnloadReference ,"called before a File > UnloadReference operation  ")	\
	SCENEMSGOP( 0, kAfterUnloadReference ,"called after a File > UnloadReference operation  ")	\
	SCENEMSGOP( 0, kBeforeSoftwareRender ,"called before a Software Render begins  ")	\
	SCENEMSGOP( 0, kAfterSoftwareRender ,"called after a Software Render ends  ")	\
	SCENEMSGOP( 0, kBeforeSoftwareFrameRender ,"called before each frame of a Software Render  ")	\
	SCENEMSGOP( 0, kAfterSoftwareFrameRender ,"called after each frame of a Software Render  ")	\
	SCENEMSGOP( 0, kSoftwareRenderInterrupted ,"called when an interactive render is interrupted by the user  ")	\
	SCENEMSGOP( 0, kMayaInitialized ,"called on interactive or batch startup after initialization  ")	\
	SCENEMSGOP( 0, kMayaExiting ,"called just before Maya exits  ")	\
	SCENEMSGOP( 1, kBeforeNewCheck ,"called prior to File > New operation, allows user to cancel action  ")	\
	SCENEMSGOP( 1, kBeforeOpenCheck ,"called prior to File > Open operation, allows user to cancel action  ")	\
	SCENEMSGOP( 1, kBeforeSaveCheck ,"called prior to File > Save operation, allows user to cancel action ")	\

/******************************* t y p e s *******************************/

typedef void (*MayaCallback)(void* clientData);
typedef void (*MayaCheckCallback)(bool* retCode, void* clientData);

struct MelInfo
{
	MString	_melScript;
	bool	_bDisplayEnabled;
	bool	_bUndoEnabled;

	MelInfo(const MString& melScript, bool bDisplayEnabled, bool bUndoEnabled)
	: _melScript(melScript)
	, _bDisplayEnabled(bDisplayEnabled)
	, _bUndoEnabled(bUndoEnabled)
	{ }

	MelInfo()
	{ }

	MelInfo(const MelInfo& rhs)
		: _melScript(rhs._melScript)
		, _bDisplayEnabled(rhs._bDisplayEnabled)
		, _bUndoEnabled(rhs._bDisplayEnabled)
	{ }

	MelInfo& MelInfo::operator=(const MelInfo& rhs)
	{
		_melScript       = rhs._melScript;
		_bDisplayEnabled = rhs._bDisplayEnabled;
		_bUndoEnabled    = rhs._bDisplayEnabled;

		return *this;
	}
};

typedef map<string, MelInfo, ltstr> MelMap;

class mayaSvn : public MPxCommand
{
	struct MsgInfo
	{
		MSceneMessage::Message	msg;
		bool					bCheck;
		const char*				pLabel;
		const char*				pDesc;
		MCallbackId				callbackId;
		bool					bInstalled; // since we don't know what a valid callbackId is
		MelMap					melScripts;
	};

	static MsgInfo msgInfos[];
public:
					mayaSvn();
	virtual			~mayaSvn();

	MStatus			doIt( const MArgList& args );
	static void*	creator();
	static MSyntax	newSyntax();

	static void		callbackStub(void* clientdata);
	static void		callbackCheckStub(bool* retCode, void* clientdata);

	static void		handleCallback(const MsgInfo& msgInfo);
	static bool		handleCheckCallback(const MsgInfo& msgInfo);

	static const char*	msgDescription(MSceneMessage::Message msg);
	static const char*	msgLabel(MSceneMessage::Message msg);
	static MsgInfo*		findMsgInfo(MSceneMessage::Message msg);
	static MsgInfo*		findMsgInfo(const MString& eventLabel);

	static void			listEvents(MStringArray& events);
	static bool			listScripts(const MString& eventLabel, MStringArray& scripts);
	static void			listAllScripts(MStringArray& scripts);
	static bool			addEventScript(const MString& eventLabel, const MString& scriptName, const MString& melScript, bool bDisplayEnabled, bool bUndoEnabled);
	static bool			delEventScript(const MString& eventLabel, const MString& scriptName);
	static bool			getFilename(const MString& nameType, MString& filename);
	static bool			compareFiles(const MString& file1, const MString& file2);
	static MString		doFileSaveDialog(const MString& title, const MString& filter, const MString& defExt, const MString& filename);

	static MStatus	install();
	static MStatus	remove();
};

/************************** p r o t o t y p e s **************************/


/***************************** g l o b a l s *****************************/

bool g_bDebug;

/****************************** m a c r o s ******************************/

#define NUM_TABLE_ELEMENTS(table)	(sizeof(table)/sizeof((table)[0]))

/**************************** r o u t i n e s ****************************/

mayaSvn::MsgInfo mayaSvn::msgInfos[] =
{
	#undef SCENEMSGOP
	#define SCENEMSGOP(check, msg,desc)	{ MSceneMessage::msg, check, #msg, desc, },
	SCENEMSGS
};

mayaSvn::MsgInfo* mayaSvn::findMsgInfo(MSceneMessage::Message msg)
{
	for (int ii = 0; ii < NUM_TABLE_ELEMENTS(msgInfos); ++ii)
	{
		if (msgInfos[ii].msg == msg)
		{
			return &msgInfos[ii];
		}
	}

	return NULL;
}

mayaSvn::MsgInfo* mayaSvn::findMsgInfo(const MString& eventLabel)
{
	for (int ii = 0; ii < NUM_TABLE_ELEMENTS(msgInfos); ++ii)
	{
		if (!stricmp(msgInfos[ii].pLabel + 1, eventLabel.asChar()))
		{
			return &msgInfos[ii];
		}
	}
	return NULL;
}

const char* mayaSvn::msgDescription(MSceneMessage::Message msg)
{
	MsgInfo* pMsgInfo = findMsgInfo(msg);
	if (pMsgInfo)
	{
		return pMsgInfo->pDesc;
	}
	return "** unknown msg **";
}

const char* mayaSvn::msgLabel(MSceneMessage::Message msg)
{
	MsgInfo* pMsgInfo = findMsgInfo(msg);
	if (pMsgInfo)
	{
		return pMsgInfo->pLabel;
	}
	return "** unknown msg **";
}

void mayaSvn::callbackStub(void* clientdata)
{
	handleCallback(*(MsgInfo*)clientdata);
}

void mayaSvn::callbackCheckStub(bool* retCode, void* clientdata)
{
	*retCode = handleCheckCallback(*(MsgInfo*)clientdata);
}

void mayaSvn::handleCallback(const MsgInfo& mi)
{
	dbgPrintf ("executing scripts for event \"%s\"", mi.pLabel + 1);
	for (MelMap::const_iterator it = mi.melScripts.begin(); it != mi.melScripts.end(); ++it)
	{
		const MelInfo& mel = it->second;
		dbgPrintf ("executing script \"%s\" for event \"%s\"", it->first.c_str(), mi.pLabel + 1);
		dbgPrintf (mel._melScript.asChar());
		MGlobal::executeCommand(mel._melScript, mel._bDisplayEnabled, mel._bUndoEnabled);
	}
	fflush(stdout);
}

bool mayaSvn::handleCheckCallback(const MsgInfo& mi)
{
	dbgPrintf ("executing check scripts for event \"%s\"", mi.pLabel + 1);
	for (MelMap::const_iterator it = mi.melScripts.begin(); it != mi.melScripts.end(); ++it)
	{
		const MelInfo& mel = it->second;
		int result;
		dbgPrintf ("executing script \"%s\" for event \"%s\"", it->first.c_str(), mi.pLabel + 1);
		dbgPrintf (mel._melScript.asChar());
		MGlobal::executeCommand(mel._melScript, result, mel._bDisplayEnabled, mel._bUndoEnabled);
		if (!result)
		{
			fflush(stdout);
			return false;
		}
	}
	fflush(stdout);
	return true;
}

void mayaSvn::listEvents(MStringArray& events)
{
	for (int ii = 0; ii < NUM_TABLE_ELEMENTS(msgInfos); ++ii)
	{
		events.append(msgInfos[ii].pLabel + 1);
	}
}

MString escape (const MString& str)
{
	MString	newstr;
	unsigned len  = str.length();
	const char* lastOk = str.asChar();

	for (const char* s = lastOk; *s; ++s)
	{
		char c = *s;
		if (c == '\\')
		{
			newstr += MString(lastOk, s - lastOk) + "\\\\";
			lastOk  = s + 1;
		}
		else if (c == '=')
		{
			newstr += MString(lastOk, s - lastOk) + "\\\"";
			lastOk  = s + 1;
		}
		else if (c == '\n')
		{
			newstr += MString(lastOk, s - lastOk) + "\\n";
			lastOk  = s + 1;
		}
		else if (c == '\t')
		{
			newstr += MString(lastOk, s - lastOk) + "\\t";
			lastOk  = s + 1;
		}
		else if (c == '\r')
		{
			newstr += MString(lastOk, s - lastOk) + "\\r";
			lastOk  = s + 1;
		}
	}
	return newstr + MString(lastOk, s - lastOk);
}


bool mayaSvn::listScripts(const MString& eventLabel, MStringArray& scripts)
{
	MsgInfo* pInfo = findMsgInfo(eventLabel);
	if (!pInfo)
	{
		errPrintf ("unknown event \"%s\"", eventLabel.asChar());
		return false;
	}

	for (MelMap::const_iterator it = pInfo->melScripts.begin(); it != pInfo->melScripts.end(); ++it)
	{
		const MelInfo& mel = it->second;

		scripts.append(MString("\"") + MString(it->first.c_str()) + "\" \"" + escape(mel._melScript) + "\"\n");
	}

	return true;
}

void mayaSvn::listAllScripts(MStringArray& scripts)
{
	for (int ii = 0; ii < NUM_TABLE_ELEMENTS(msgInfos); ++ii)
	{
		MsgInfo& mi = msgInfos[ii];

		for (MelMap::const_iterator it = mi.melScripts.begin(); it != mi.melScripts.end(); ++it)
		{
			const MelInfo& mel = it->second;

			scripts.append(MString("\"") + (mi.pLabel + 1) + "\" \"" + MString(it->first.c_str()) + "\" \"" + escape(MString(mel._melScript)) + "\"\n");
		}
	}
}

bool mayaSvn::getFilename(const MString& nameType, MString& filename)
{
	MStatus stat;

	#undef NAMEOP
	#define NAMETYPES	\
		NAMEOP(beforeOpenFilename)	\
		NAMEOP(beforeImportFilename)	\
		NAMEOP(beforeSaveFilename)	\
		NAMEOP(beforeExportFilename)	\
		NAMEOP(beforeReferenceFilename)	\
		NAMEOP(getLastTempFile)	\

	#undef NAMEOP
	#define NAMEOP(name)	\
		if (!stricmp(# name, nameType.asChar())) \
		{ \
			filename = MFileIO::name(&stat); \
			return stat; \
		}

	NAMETYPES

	errPrintf ("unknown name type, known types:");
	#undef NAMEOP
	#define NAMEOP(name)	\
		statPrintf (# name);

	NAMETYPES

	return false;
}

bool mayaSvn::addEventScript(const MString& eventLabel, const MString& scriptName, const MString& melScript, bool bDisplayEnabled, bool bUndoEnabled)
{
	MsgInfo* pInfo = findMsgInfo(eventLabel);
	if (!pInfo)
	{
		errPrintf ("unknown event \"%s\"", eventLabel.asChar());
		return false;
	}

	pInfo->melScripts[scriptName.asChar()] = MelInfo(melScript, bDisplayEnabled, bUndoEnabled);
	dbgPrintf ("script \"%s\" added to event \"%s\"", scriptName.asChar(), eventLabel.asChar());
	return true;
}

bool mayaSvn::delEventScript(const MString& eventLabel, const MString& scriptName)
{
	MsgInfo* pInfo = findMsgInfo(eventLabel);
	if (!pInfo)
	{
		errPrintf ("unknown event \"%s\"", eventLabel.asChar());
		return false;
	}

	MelMap::iterator it = pInfo->melScripts.find(scriptName.asChar());
	if (it == pInfo->melScripts.end())
	{
		warnPrintf ("no script \"%s\" attached to event \"%s\"", scriptName.asChar(), eventLabel.asChar());
		return true;
	}

	pInfo->melScripts.erase(it);
	dbgPrintf ("script \"%s\" deleted from event \"%s\"", scriptName.asChar(), eventLabel.asChar());
	return true;
}

bool mayaSvn::compareFiles(const MString& file1, const MString& file2)
{
	static char buffer1[16386];
	static char buffer2[16386];
	bool result = false;
	int fh1 = -1;
	int fh2 = -1;

	fh1 = open(file1.asChar(), O_RDONLY | O_BINARY, S_IREAD);
	if (fh1 < 0) goto cleanup;
	fh2 = open(file2.asChar(), O_RDONLY | O_BINARY, S_IREAD);
	if (fh2 < 0) goto cleanup;

	long size1 = filelength (fh1);
	long size2 = filelength (fh2);

	if (size1 == size2)
	{
		while (size1 > 0)
		{
			long sizeToRead = size1 > sizeof(buffer1) ? sizeof(buffer1) : size1;

			read(fh1, buffer1, sizeToRead);
			read(fh2, buffer2, sizeToRead);

			if (memcmp(buffer1, buffer2, sizeToRead))
			{
				goto cleanup;
			}

			size1 -= sizeToRead;
		}

		result = true;
	}

cleanup:
	if (fh2 >= 0) close(fh2);
	if (fh1 >= 0) close(fh1);

	return result;
}

MString mayaSvn::doFileSaveDialog(const MString& title, const MString& filter, const MString& defExt, const MString& filename)
{
	static OPENFILENAME ofn;
	static char  filenameBuffer[MAX_PATH];
	static char  filterBuffer[1024];

	// setup filter (assume | for separator
	if (filter.length() > 0)
	{
		char* d      = filterBuffer;
		char* e      = filterBuffer + sizeof(filterBuffer) / sizeof(filterBuffer[0]);
		const char *s = filter.asChar();

		while (*s && d < e - 2)
		{
			if (*s == '|')
			{
				*d = '\0';
			}
			else
			{
				*d = *s;
			}
			++d;
			++s;
		}
		*d++ = '\0';
		*d++ = '\0';
	}

	if (filename.length() < MAX_PATH - 1)
	{
		const char* s = filename.asChar();
		char* d = filenameBuffer;

		while (*s)
		{
			if (*s == '/')
			{
				*d++ = '\\';
			}
			else
			{
				*d++ = *s;
			}
			++s;
		}
		*d = '\0';
	}

	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.lpstrFilter = filter.length() > 0 ? filterBuffer : NULL;
	ofn.lpstrFile   = filenameBuffer;
	ofn.nMaxFile    = MAX_PATH;
	ofn.lpstrTitle  = title.asChar();
	ofn.Flags       = OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
	ofn.lpstrDefExt = defExt.length() > 0 ? defExt.asChar() : NULL;

	MString result;

	if (GetSaveFileName(&ofn))
	{
		result = ofn.lpstrFile;
	}

	return result;
}

MStatus mayaSvn::install()
{
	MStatus stat;

	for (int ii = 0; ii < NUM_TABLE_ELEMENTS(msgInfos); ++ii)
	{
		MsgInfo& mi = msgInfos[ii];

		if (!mi.bInstalled)
		{
			if (mi.bCheck)
			{
				mi.callbackId = MSceneMessage::addCallback(mi.msg, callbackCheckStub, &mi, &stat);
			}
			else
			{
				mi.callbackId = MSceneMessage::addCallback(mi.msg, callbackStub, &mi, &stat);
			}
			if (!stat)
			{
				errPrintf ("could not install callback for MSceneMessage::%s\n", mi.pLabel);
				return stat;
			}
			mi.bInstalled = true;

			dbgPrintf ("installed callback for MSceneMessage::%s\n", mi.pLabel);
		}
	}

	statPrintf ("installed mayaSvn\n");

	return stat;
}

MStatus mayaSvn::remove()
{
	MStatus stat;

	statPrintf ("mayaSvn: removing all callbacks\n");

	for (int ii = 0; ii < NUM_TABLE_ELEMENTS(msgInfos); ++ii)
	{
		MsgInfo& mi = msgInfos[ii];

		if (mi.bInstalled)
		{
			MSceneMessage::removeCallback(mi.callbackId);
			mi.bInstalled = false;

			dbgPrintf ("removed callback for MSceneMessage::%s\n", mi.pLabel);
		}
	}

	return stat;
}

#define kDebugFlag				"-d"
#define kDebugFlagLong			"-debug"
#define kListEventsFlag			"-le"
#define kListEventsFlagLong		"-listEvents"
#define kListScriptsFlag		"-ls"
#define kListScriptsFlagLong	"-listScripts"
#define kListAllScriptsFlag		"-las"
#define kListAllScriptsFlagLong	"-listAllScripts"
#define kAddEventFlag			"-ae"
#define kAddEventFlagLong		"-addEvent"
#define kDelEventFlag			"-de"
#define kDelEventFlagLong		"-delEvent"
#define kScriptNameFlag			"-sn"
#define kScriptNameFlagLong		"-scriptName"
#define kMelFlag				"-m"
#define kMelFlagLong			"-mel"
#define kDisplayEnabledFlag		"-ds"
#define kDisplayEnabledFlagLong	"-displayEnabled"
#define kUndoEnabledFlag		"-ue"
#define kUndoEnabledFlagLong	"-undoEnabled"
#define kGetFilenameFlag		"-gf"
#define kGetFilenameFlagLong	"-getFilename"
#define kCompareFilesFlag		"-cf"
#define kCompareFilesFlagLong	"-compareFiles"
#define kFile2Flag				"-f2"
#define kFile2FlagLong			"-file2"
#define kFileSaveDialogFlag		"-fsd"
#define kFileSaveDialogFlagLong	"-fileSaveDialog"
#define kTitleFlag				"-t"
#define kTitleFlagLong			"-title"
#define kFilenameFlag			"-fn"
#define kFilenameFlagLong		"-fileName"
#define kDefExtFlag				"-ext"
#define kDefExtFlagLong			"-extension"
#define kFilterFlag				"-ft"
#define kFilterFlagLong			"-filter"


MStatus mayaSvn::doIt( const MArgList& args )
//
//	Description:
//		implements the MEL mayaSvn command.
//
//	Arguments:
//		args - the argument list that was passes to the command from MEL
//
//	Return Value:
//		MS::kSuccess - command succeeded
//		MS::kFailure - command failed (returning this value will cause the
//                     MEL script that is being run to terminate unless the
//                     error is caught using a "catch" statement.
//
{
	MStatus stat;
	MArgDatabase	argData(syntax(), args);

	dbgReset();

	g_bDebug = argData.isFlagSet(kDebugFlag);
	dbgSetDebug(g_bDebug);

    if (argData.isFlagSet(kListEventsFlag))
	{
		MStringArray eventList;

		listEvents(eventList);
		clearResult();
		setResult(eventList);
	}
	else if (argData.isFlagSet(kListAllScriptsFlag))
	{
		MStringArray scriptList;

		listAllScripts(scriptList);
		clearResult();
		setResult(scriptList);
	}
	else if (argData.isFlagSet(kGetFilenameFlag))
	{
		MString	nameType;
		MString	filename;

		argData.getFlagArgument(kGetFilenameFlag, 0, nameType);
		if (!getFilename(nameType, filename))
		{
			return MStatus::kFailure;
		}
		clearResult();
		setResult(filename);
	}
	else if (argData.isFlagSet(kListScriptsFlag))
	{
		MString	eventLabel;
		MStringArray scriptList;

		argData.getFlagArgument(kListScriptsFlag, 0, eventLabel);

		if (!listScripts(eventLabel,scriptList))
		{
			return MStatus::kFailure;
		}
		clearResult();
		setResult(scriptList);
	}
	else if (argData.isFlagSet(kAddEventFlag))
	{
		MString	eventLabel;
		MString	scriptName;
		MString	melScript;
		bool	bDisplayEnabled;
		bool	bUndoEnabled;

		if (!argData.isFlagSet(kScriptNameFlag))
		{
			errPrintf ("no -scriptName specified");
			return MStatus::kFailure;
		}
		if (!argData.isFlagSet(kMelFlag))
		{
			errPrintf ("no -mel specified");
			return MStatus::kFailure;
		}

		argData.getFlagArgument(kAddEventFlag, 0, eventLabel);
		argData.getFlagArgument(kScriptNameFlag, 0, scriptName);
		argData.getFlagArgument(kMelFlag, 0, melScript);

		bDisplayEnabled = argData.isFlagSet(kDisplayEnabledFlag);
		bUndoEnabled    = argData.isFlagSet(kUndoEnabledFlag);

		if (!addEventScript(eventLabel, scriptName, melScript, bDisplayEnabled, bUndoEnabled))
		{
			return MStatus::kFailure;
		}
	}
	else if (argData.isFlagSet(kDelEventFlag))
	{
		MString	eventLabel;
		MString	scriptName;

		if (!argData.isFlagSet(kScriptNameFlag))
		{
			errPrintf ("no -scriptName specified");
			return MStatus::kFailure;
		}

		argData.getFlagArgument(kAddEventFlag, 0, eventLabel);
		argData.getFlagArgument(kScriptNameFlag, 0, scriptName);

		if (!delEventScript(eventLabel, scriptName))
		{
			return MStatus::kFailure;
		}
	}
	else if (argData.isFlagSet(kCompareFilesFlag))
	{
		MString file1;
		MString file2;

		if (!argData.isFlagSet(kFile2Flag))
		{
			errPrintf ("no -file2 specified");
			return MStatus::kFailure;
		}

		argData.getFlagArgument(kCompareFilesFlag, 0, file1);
		argData.getFlagArgument(kFile2Flag, 0, file2);

		clearResult();
		setResult(compareFiles(file1, file2));
	}
	else if (argData.isFlagSet(kFileSaveDialogFlag))
	{
		MString title;
		MString filename;
		MString defExt;
		MString filter;

		if (argData.isFlagSet(kTitleFlag)) { argData.getFlagArgument(kTitleFlag, 0, title); }
		if (argData.isFlagSet(kFilenameFlag)) { argData.getFlagArgument(kFilenameFlag, 0, filename); }
		if (argData.isFlagSet(kDefExtFlag)) { argData.getFlagArgument(kDefExtFlag, 0, defExt); }
		if (argData.isFlagSet(kFilterFlag)) { argData.getFlagArgument(kFilterFlag, 0, filter); }

		clearResult();
		setResult(doFileSaveDialog(title, filter, defExt, filename));
	}
	else
	{
		stat = MS::kInvalidParameter;
		MGlobal::displayError("no or unknown arguments");
	}

	fflush(stdout);

	return stat;
}

MSyntax mayaSvn::newSyntax()
{
	MSyntax syntax;

	syntax.addFlag(kListScriptsFlag, kListScriptsFlagLong, MSyntax::kString);
	syntax.addFlag(kAddEventFlag, kAddEventFlagLong, MSyntax::kString);
	syntax.addFlag(kDelEventFlag, kDelEventFlagLong, MSyntax::kString);
	syntax.addFlag(kScriptNameFlag, kScriptNameFlagLong, MSyntax::kString);
	syntax.addFlag(kMelFlag, kMelFlagLong, MSyntax::kString);
	syntax.addFlag(kGetFilenameFlag, kGetFilenameFlagLong, MSyntax::kString);

	syntax.addFlag(kCompareFilesFlag, kCompareFilesFlagLong, MSyntax::kString);
	syntax.addFlag(kFile2Flag, kFile2FlagLong, MSyntax::kString);
	syntax.addFlag(kFileSaveDialogFlag, kFileSaveDialogFlagLong);
	syntax.addFlag(kTitleFlag, kTitleFlagLong, MSyntax::kString);
	syntax.addFlag(kFilenameFlag, kFilenameFlagLong, MSyntax::kString);
	syntax.addFlag(kDefExtFlag, kDefExtFlagLong, MSyntax::kString);
	syntax.addFlag(kFilterFlag, kFilterFlagLong, MSyntax::kString);

	syntax.addFlag(kDisplayEnabledFlag, kDisplayEnabledFlagLong);
	syntax.addFlag(kUndoEnabledFlag, kUndoEnabledFlagLong);
	syntax.addFlag(kListAllScriptsFlag, kListAllScriptsFlagLong);
	syntax.addFlag(kDebugFlag, kDebugFlagLong);
	syntax.addFlag(kListEventsFlag, kListEventsFlagLong);

	return syntax;
}

mayaSvn::mayaSvn()
{
}

mayaSvn::~mayaSvn()
{
}

void* mayaSvn::creator()
{
	return new mayaSvn();
}

/***********************************  ************************************/
/***********************************  ************************************/
/***********************************  ************************************/

//////////////////////////////////////////////////////////////////////////
//
// Plugin registration
//
//////////////////////////////////////////////////////////////////////////

MStatus initializePlugin( MObject obj )
{
	MFnPlugin plugin( obj, "Greggman.com", "0.01");

	mayaSvn::install();
	return plugin.registerCommand( "mayaSvn", mayaSvn::creator, mayaSvn::newSyntax);
}

MStatus uninitializePlugin( MObject obj)
{
	// remove all the callbacks
	mayaSvn::remove();

	MFnPlugin plugin( obj );
	return plugin.deregisterCommand( "mayaSvn" );
}
