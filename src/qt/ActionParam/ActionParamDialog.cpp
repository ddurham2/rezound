/* 
 * Copyright (C) 2007 - David W. Durham
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
#include <stdio.h>
#include "ActionParamDialog.h"

#include <QMainWindow>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QInputDialog>

#include <stdexcept>

#include <istring>

#include <CNestedDataFile/CNestedDataFile.h>

#include "../StatusComm.h"
#include "../settings.h"
#include "../../misc/urlencode.h"

#include "../../backend/CActionParameters.h"
#include "../../backend/CActionSound.h"
#include "../../backend/AAction.h" // for EUserMessage

// ----------------------------------------

// ??? TODO Well, I got it to not need the width, it's determined by the 
// widgets' needed widths.. I don't quite know tho, what the height won't 
// work the same what.. I'll work on figuring that out and then both 
// parameters should be unnecessary

ActionParamDialog::ActionParamDialog(QWidget *mainWindow,bool _showPresetPanel,const string _presetPrefix,ModalDialog::ShowTypes showType) :
	ModalDialog(mainWindow,"",/*0,0,*/ModalDialog::ftVertical,showType),
	showPresetPanel(_showPresetPanel),

	explanationButtonCreated(false),
	
	presetPrefix(_presetPrefix=="" ? "" : _presetPrefix DOT ""),

	firstShowing(true)
{
	apdc.setupUi(getFrame());
	apdc.controlsFrame->setLayout(new QHBoxLayout);
	apdc.controlsFrame->layout()->setMargin(0);
	apdc.controlsFrame->layout()->setSpacing(0);

	connect(apdc.nativePresetList,SIGNAL(itemDoubleClicked(QListWidgetItem *)),this,SLOT(onPresetUseButton()));
	connect(apdc.userPresetList,SIGNAL(itemDoubleClicked(QListWidgetItem *)),this,SLOT(onPresetUseButton()));
	connect(apdc.useNativePresetButton,SIGNAL(clicked()),this,SLOT(onPresetUseButton()));
	connect(apdc.useUserPresetButton,SIGNAL(clicked()),this,SLOT(onPresetUseButton()));
	connect(apdc.saveUserPresetButton,SIGNAL(clicked()),this,SLOT(onPresetSaveButton()));
	connect(apdc.removeUserPresetButton,SIGNAL(clicked()),this,SLOT(onPresetRemoveButton()));

	// do initial sizing of splitter 
	QList<int> sizes;
	sizes.push_back(getFrame()->height()-apdc.presetsFrame->minimumHeight()-apdc.splitter->handleWidth());
	sizes.push_back(apdc.splitter->handleWidth());
	sizes.push_back(apdc.presetsFrame->minimumHeight());
	apdc.splitter->setSizes(sizes);

#if 0
	disableFrameDecor();

	setHeight(getHeight()+100); // since we're adding this presets section, make the dialog taller
#endif

	// make sure the dialog has at least a minimum height and width
	//ASSURE_HEIGHT(this,10);
	//ASSURE_WIDTH(this,200);
}

ActionParamDialog::~ActionParamDialog()
{
}

void ActionParamDialog::onExplainButton()
{
	Message(getExplanation());
}

QWidget *ActionParamDialog::newPanel(bool horz,QWidget *parent,bool createMargin,bool createFrame)
{
	if(!parent)
	{
		/*
		if(apdc.controlsFrame->children().size()>0)
			throw runtime_error(string(__func__)+" -- this method has already been called with a NULL parameter");
		*/
		parent=apdc.controlsFrame;
	}

	QFrame *f=new QFrame;

	if(createFrame)
		f->setFrameShape(QFrame::StyledPanel);
	else
		f->setFrameShape(QFrame::NoFrame);

	if(horz)
		f->setLayout(new QHBoxLayout);
	else
		f->setLayout(new QVBoxLayout);


	if(createMargin)
		f->layout()->setMargin(6);
	else
		f->layout()->setMargin(0);
	f->layout()->setSpacing(4);

	parent->layout()->addWidget(f);

	return f;
}

QWidget *ActionParamDialog::newHorzPanel(QWidget *parent,bool createMargin,bool createFrame)
{
	return newPanel(true,parent,createMargin,createFrame);
}

QWidget *ActionParamDialog::newVertPanel(QWidget *parent,bool createMargin,bool createFrame)
{
	return newPanel(false,parent,createMargin,createFrame);
}

ConstantParamValue *ActionParamDialog::addSlider(QWidget *parent,const string name,const string units,AActionParamMapper *valueMapper,f_at_x optRetValueConv,bool showInverseButton)
{
	if(!parent)
		throw runtime_error(string(__func__)+" -- parent was passed NULL -- use CActionParameValue::newHorzPanel() or newVertPanel() to obtain a parent parameter to pass");
	ConstantParamValue *slider=new ConstantParamValue(valueMapper,showInverseButton,name.c_str());
	parent->layout()->addWidget(slider);
	slider->setUnits(units.c_str());
	slider->setValue(valueMapper->getDefaultValue());
	parameters.push_back(make_pair(ptConstant,slider));
	retValueConvs.push_back(optRetValueConv);

	return slider;
}

ConstantParamValue *ActionParamDialog::getSliderParam(const string name)
{
	const unsigned index=findParamByName(name);
	if(parameters[index].first==ptConstant)
		return (ConstantParamValue *)parameters[index].second;
	throw runtime_error(string(__func__)+" -- widget with name, "+name+", is not a slider");
}

TextParamValue *ActionParamDialog::addNumericTextEntry(QWidget *parent,const string name,const string units,const double initialValue,const double minValue,const double maxValue,const string unitsTipText)
{
	if(!parent)
		throw runtime_error(string(__func__)+" -- parent was passed NULL -- use CActionParameValue::newHorzPanel() or newVertPanel() to obtain a parent parameter to pass");
	TextParamValue *textEntry=new TextParamValue(name.c_str(),initialValue,minValue,maxValue);
	parent->layout()->addWidget(textEntry);
	textEntry->setUnits(units.c_str(),unitsTipText.c_str());
	parameters.push_back(make_pair(ptNumericText,textEntry));
	retValueConvs.push_back(NULL);

	return textEntry;
}

TextParamValue *ActionParamDialog::addStringTextEntry(QWidget *parent,const string name,const string initialValue,const string tipText)
{
	if(!parent)
		throw runtime_error(string(__func__)+" -- parent was passed NULL -- use CActionParameValue::newHorzPanel() or newVertPanel() to obtain a parent parameter to pass");
	TextParamValue *textEntry=new TextParamValue(name.c_str(),initialValue);
	parent->layout()->addWidget(textEntry);
	textEntry->setTipText(tipText.c_str());
	parameters.push_back(make_pair(ptStringText,textEntry));
	retValueConvs.push_back(NULL);

	return textEntry;
}

TextParamValue *ActionParamDialog::getTextParam(const string name)
{
	for(size_t t=0;t<parameters.size();t++)
	{
		if((parameters[t].first==ptStringText || parameters[t].first==ptNumericText) && ((TextParamValue *)parameters[t].second)->getName()==name)
			return (TextParamValue *)parameters[t].second;
	}
	throw runtime_error(string(__func__)+" -- no text param found named "+name);
}

DiskEntityParamValue *ActionParamDialog::addDiskEntityEntry(QWidget *parent,const string name,const string initialEntityName,DiskEntityParamValue::DiskEntityTypes entityType,const string tipText)
{
	if(!parent)
		throw runtime_error(string(__func__)+" -- parent was passed NULL -- use CActionParameValue::newHorzPanel() or newVertPanel() to obtain a parent parameter to pass");
	DiskEntityParamValue *diskEntityEntry=new DiskEntityParamValue(name.c_str(),initialEntityName,entityType);
	parent->layout()->addWidget(diskEntityEntry);
	diskEntityEntry->setTipText(tipText.c_str());
	parameters.push_back(make_pair(ptDiskEntity,diskEntityEntry));
	retValueConvs.push_back(NULL);

	return diskEntityEntry;
}

DiskEntityParamValue *ActionParamDialog::getDiskEntityParam(const string name)
{
	const unsigned index=findParamByName(name);
	if(parameters[index].first==ptDiskEntity)
		return (DiskEntityParamValue *)parameters[index].second;
	throw runtime_error(string(__func__)+" -- widget with name, "+name+", is not a disk entity");
}

ComboTextParamValue *ActionParamDialog::addComboTextEntry(QWidget *parent,const string name,const vector<string> &items,ComboParamValueTypes type,const string tipText,bool isEditable)
{
	if(!parent)
		throw runtime_error(string(__func__)+" -- parent was passed NULL -- use CActionParameValue::newHorzPanel() or newVertPanel() to obtain a parent parameter to pass");
	ComboTextParamValue *comboTextEntry=new ComboTextParamValue(name.c_str(),items,isEditable);
	parent->layout()->addWidget(comboTextEntry);
	if(type==cpvtAsString)
		comboTextEntry->asString=true;
	else
		comboTextEntry->asString=false;
	comboTextEntry->setTipText(tipText.c_str());
	parameters.push_back(make_pair(ptComboText,comboTextEntry));
	retValueConvs.push_back(NULL);

	return comboTextEntry;
}

ComboTextParamValue *ActionParamDialog::getComboText(const string name)
{
	const unsigned index=findParamByName(name);
	if(parameters[index].first==ptComboText);
		return (ComboTextParamValue *)parameters[index].second;
	throw runtime_error(string(__func__)+" -- widget with name, "+name+", is not a combobox");
}


CheckBoxParamValue *ActionParamDialog::addCheckBoxEntry(QWidget *parent,const string name,const bool checked,const string tipText)
{
	if(!parent)
		throw runtime_error(string(__func__)+" -- parent was passed NULL -- use CActionParameValue::newHorzPanel() or newVertPanel() to obtain a parent parameter to pass");
	CheckBoxParamValue *checkBoxEntry=new CheckBoxParamValue(name.c_str(),checked);
	parent->layout()->addWidget(checkBoxEntry);
	checkBoxEntry->setTipText(tipText.c_str());
	parameters.push_back(make_pair(ptCheckBox,checkBoxEntry));
	retValueConvs.push_back(NULL);

	return checkBoxEntry;
}

CheckBoxParamValue *ActionParamDialog::getCheckBoxParam(const string name)
{
	const unsigned index=findParamByName(name);
	if(parameters[index].first==ptCheckBox)
		return (CheckBoxParamValue *)parameters[index].second;
	throw runtime_error(string(__func__)+" -- widget with name, "+name+", is not a checkbox");
}

GraphParamValue *ActionParamDialog::addGraph(QWidget *parent,const string name,const string horzAxisLabel,const string horzUnits,AActionParamMapper *horzValueMapper,const string vertAxisLabel,const string vertUnits,AActionParamMapper *vertValueMapper,f_at_x optRetValueConv)
{
	if(!parent)
		throw runtime_error(string(__func__)+" -- parent was passed NULL -- use CActionParameValue::newHorzPanel() or newVertPanel() to obtain a parent parameter to pass");
	GraphParamValue *graph=new GraphParamValue(name.c_str());
	parent->layout()->addWidget(graph);
	graph->setHorzParameters(horzAxisLabel,horzUnits,horzValueMapper);
	graph->setVertParameters(vertAxisLabel,vertUnits,vertValueMapper);
	parameters.push_back(make_pair(ptGraph,graph));
	retValueConvs.push_back(optRetValueConv);

	return graph;
}

GraphParamValue *ActionParamDialog::addGraphWithWaveform(QWidget *parent,const string name,const string vertAxisLabel,const string vertUnits,AActionParamMapper *vertValueMapper,f_at_x optRetValueConv)
{
	if(!parent)
		throw runtime_error(string(__func__)+" -- parent was passed NULL -- use CActionParameValue::newHorzPanel() or newVertPanel() to obtain a parent parameter to pass");
	GraphParamValue *graph=new GraphParamValue(name.c_str());
	parent->layout()->addWidget(graph);
	graph->setVertParameters(vertAxisLabel,vertUnits,vertValueMapper);
	parameters.push_back(make_pair(ptGraphWithWaveform,graph));
	retValueConvs.push_back(optRetValueConv);

	return graph;
}

GraphParamValue *ActionParamDialog::getGraphParam(const string name)
{
	for(size_t t=0;t<parameters.size();t++)
	{
		if((parameters[t].first==ptGraph || parameters[t].first==ptGraphWithWaveform) && ((GraphParamValue *)parameters[t].second)->getName()==name)
			return (GraphParamValue *)parameters[t].second;
	}
	throw runtime_error(string(__func__)+" -- no graph param found named "+name);
}

LFOParamValue *ActionParamDialog::addLFO(QWidget *parent,const string name,const string ampUnits,const string ampTitle,const double maxAmp,const string freqUnits,const double maxFreq,const bool hideBipolarLFOs)
{
	if(!parent)
		throw runtime_error(string(__func__)+" -- parent was passed NULL -- use CActionParameValue::newHorzPanel() or newVertPanel() to obtain a parent parameter to pass");
    LFOParamValue *LFOEntry=new LFOParamValue(name.c_str(),ampUnits,ampTitle,maxAmp,freqUnits,maxFreq,hideBipolarLFOs);
	//LFOEntry->setTipText(tipText.c_str());
	parent->layout()->addWidget(LFOEntry);
	parameters.push_back(make_pair(ptLFO,LFOEntry));
	retValueConvs.push_back(NULL);

	return LFOEntry;
}

LFOParamValue *ActionParamDialog::getLFOParam(const string name)
{
	const unsigned index=findParamByName(name);
	if(parameters[index].first==ptLFO);
        return (LFOParamValue *)parameters[index].second;
	throw runtime_error(string(__func__)+" -- widget with name, "+name+", is not an LFO");
}

PluginRoutingParamValue *ActionParamDialog::addPluginRoutingParam(QWidget *parent,const string name,const LADSPA_Descriptor *desc)
{
	if(!parent)
		throw runtime_error(string(__func__)+" -- parent was passed NULL -- use CActionParameValue::newHorzPanel() or newVertPanel() to obtain a parent parameter to pass");
    PluginRoutingParamValue *pluginRoutingEntry=new PluginRoutingParamValue(name.c_str(),desc);
	parent->layout()->addWidget(pluginRoutingEntry);
	parameters.push_back(make_pair(ptPluginRouting,pluginRoutingEntry));
	retValueConvs.push_back(NULL);

	return pluginRoutingEntry;
}

PluginRoutingParamValue *ActionParamDialog::getPluginRoutingParam(const string name)
{
	const unsigned index=findParamByName(name);
	if(parameters[index].first==ptPluginRouting);
        return (PluginRoutingParamValue *)parameters[index].second;
	throw runtime_error(string(__func__)+" -- widget with name, "+name+", is not a Plugin Routing");
}

#if 0
void ActionParamDialog::setMargin(int margin)
{
	leftMargin->setWidth(margin);
	rightMargin->setWidth(margin);
}
#endif

void ActionParamDialog::setValue(size_t index,const double value)
{
	switch(parameters[index].first)
	{
	case ptConstant:
		((ConstantParamValue *)parameters[index].second)->setValue(value);
		break;

	case ptNumericText:
		((TextParamValue *)parameters[index].second)->setValue(value);
		break;

	case ptStringText:
		((TextParamValue *)parameters[index].second)->setText(istring(value));
		break;

	case ptComboText:
		((ComboTextParamValue *)parameters[index].second)->setIntegerValue((int)value);
		break;

	case ptCheckBox:
		((CheckBoxParamValue *)parameters[index].second)->setValue((bool)value);
		break;

	case ptGraph:
	case ptGraphWithWaveform:
		/*
		((GraphParamValue *)parameters[index].second)->setValue(value);
		break;
		*/

	case ptLFO:
		/*
		((GraphParamValue *)parameters[index].second)->setValue(value);
		break;
		*/

	case ptPluginRouting:
		/*
		((GraphParamValue *)parameters[index].second)->setValue(value);
		 break;
		 */

	default:
		throw runtime_error(string(__func__)+" -- unhandled or unimplemented parameter type: "+istring(parameters[index].first));
	}
}

void ActionParamDialog::setTipText(const string name,const string tipText)
{
	const unsigned index=findParamByName(name);
	
	switch(parameters[index].first)
	{
	case ptConstant:
		((ConstantParamValue *)parameters[index].second)->setTipText(tipText.c_str());
		break;

	case ptNumericText:
	case ptStringText:
		((TextParamValue *)parameters[index].second)->setTipText(tipText.c_str());
		break;

	case ptDiskEntity:
		((DiskEntityParamValue *)parameters[index].second)->setTipText(tipText.c_str());
		break;

	case ptComboText:
		((ComboTextParamValue *)parameters[index].second)->setTipText(tipText.c_str());
		break;

	case ptCheckBox:
		((CheckBoxParamValue *)parameters[index].second)->setTipText(tipText.c_str());
		break;

	case ptGraph:
	case ptGraphWithWaveform:
/*
		((GraphParamValue *)parameters[index].second)->setTipText(tipText.c_str());
		break;
*/

	case ptLFO:
/*
		((LFOParamValue *)parameters[index].second)->setTipText(tipText.c_str());
		break;
*/

	case ptPluginRouting:
/*
		((PluginRoutingParamValue *)parameters[index].second)->setTipText(tipText.c_str());
		break;
*/

	default:
		throw runtime_error(string(__func__)+" -- unhandled or unimplemented parameter type: "+istring(parameters[index].first));
	}
}

void ActionParamDialog::showControl(const string name,bool show)
{
	const unsigned index=findParamByName(name);
	
	switch(parameters[index].first)
	{
	case ptConstant:
		((ConstantParamValue *)parameters[index].second)->setVisible(show);
		break;

	case ptNumericText:
	case ptStringText:
		((TextParamValue *)parameters[index].second)->setVisible(show);
		break;

	case ptDiskEntity:
		((DiskEntityParamValue *)parameters[index].second)->setVisible(show);
		break;

	case ptComboText:
		((ComboTextParamValue *)parameters[index].second)->setVisible(show);
		break;

	case ptCheckBox:
		((CheckBoxParamValue *)parameters[index].second)->setVisible(show);
		break;

	case ptGraph:
	case ptGraphWithWaveform:
		((GraphParamValue *)parameters[index].second)->setVisible(show);
		break;

	case ptLFO:
        ((LFOParamValue *)parameters[index].second)->setVisible(show);
		break;

	case ptPluginRouting:
        ((PluginRoutingParamValue *)parameters[index].second)->setVisible(show);
		break;

	default:
		throw runtime_error(string(__func__)+" -- unhandled or unimplemented parameter type: "+istring(parameters[index].first));
	}
	
	// tell it to recalc if the number of shown or hidden children changes
//	parameters[index].second->parentWidget()->recalc();
}

bool ActionParamDialog::show(CActionSound *actionSound,CActionParameters *actionParameters)
{
	// couldn't do this from the ctor
	if(firstShowing && !getExplanation().empty())
	{
		QPushButton *eb=new QPushButton;
		eb->setText(_("Explain"));
		//eb->setIcon(FOXIcons->explain);

		((QHBoxLayout *)(getButtonFrame()->layout()))->insertWidget(0,eb);
		connect(eb,SIGNAL(clicked()),this,SLOT(onExplainButton()));
	}

	this->actionSound=actionSound;

	bool retval=false;

	if(getOrigTitle()=="")
		throw runtime_error(string(__func__)+" -- title was never set");

	buildPresetLists();

	// restore the splitter's position
	{
		const string d=urldecode(gSettingsRegistry->getValue<string>("Qt4" DOT "SplitterPositions" DOT presetPrefix+getOrigTitle().toStdString()));
		if(!d.empty())
			apdc.splitter->restoreState(QByteArray(d.data(),d.size()));
	}


	// initialize all the graphs to this sound
	for(size_t t=0;t<parameters.size();t++)
	{
		if(parameters[t].first==ptGraphWithWaveform)
		{
			((GraphParamValue *)parameters[t].second)->setSound(actionSound->sound,actionSound->start,actionSound->stop);
			if(firstShowing)
				((GraphParamValue *)parameters[t].second)->clearNodes();
		}
		else if(parameters[t].first==ptPluginRouting)
            ((PluginRoutingParamValue *)parameters[t].second)->setSound(actionSound->sound);
	}

	firstShowing=false;

reshow:

	int r=exec();
	printf("dialog exec returned r: %d\n",r);
	if(r)
	{
		vector<string> addedParameters;
		try
		{
			for(unsigned t=0;t<parameters.size();t++)
			{
				switch(parameters[t].first)
				{
				case ptConstant:
					{
						ConstantParamValue *slider=(ConstantParamValue *)parameters[t].second;
						double ret=slider->getValue();

						if(retValueConvs[t]!=NULL)
							ret=retValueConvs[t](ret);

						actionParameters->setValue<double>(slider->getName(),ret);
						addedParameters.push_back(slider->getName());
					}
					break;

				case ptNumericText:
					{
						TextParamValue *textEntry=(TextParamValue *)parameters[t].second;
						double ret=textEntry->getValue();

						if(retValueConvs[t]!=NULL)
							ret=retValueConvs[t](ret);

						actionParameters->setValue<double>(textEntry->getName(),ret);	
						addedParameters.push_back(textEntry->getName());
					}
					break;

				case ptStringText:
					{
						TextParamValue *textEntry=(TextParamValue *)parameters[t].second;
						const string ret=textEntry->getText();
						actionParameters->setValue<string>(textEntry->getName(),ret);	
						addedParameters.push_back(textEntry->getName());
					}
					break;

				case ptDiskEntity:
					{
						DiskEntityParamValue *diskEntityEntry=(DiskEntityParamValue *)parameters[t].second;
						const string ret=diskEntityEntry->getEntityName();

						actionParameters->setValue<string>(diskEntityEntry->getName(),ret);	

						if(diskEntityEntry->getEntityType()==DiskEntityParamValue::detAudioFilename)
							actionParameters->setValue<bool>(diskEntityEntry->getName()+" OpenAsRaw",diskEntityEntry->getOpenAsRaw());	
						addedParameters.push_back(diskEntityEntry->getName());
					}
					break;

				case ptComboText:
					{
						ComboTextParamValue *comboTextEntry=(ComboTextParamValue *)parameters[t].second;
						if(comboTextEntry->asString)
						{ // return the text of the item selected
							actionParameters->setValue<string>(comboTextEntry->getName(),comboTextEntry->getStringValue());
						}
						else
						{ // return values as integer of the index that was selected
							int ret=comboTextEntry->getIntegerValue();
							actionParameters->setValue<unsigned>(comboTextEntry->getName(),(unsigned)ret);	
						}
						addedParameters.push_back(comboTextEntry->getName());
					}
					break;

				case ptCheckBox:
					{
						CheckBoxParamValue *checkBoxEntry=(CheckBoxParamValue *)parameters[t].second;
						bool ret=checkBoxEntry->getValue();

						actionParameters->setValue<bool>(checkBoxEntry->getName(),ret);	
						addedParameters.push_back(checkBoxEntry->getName());
					}
					break;

				case ptGraph:
				case ptGraphWithWaveform:
					{
						GraphParamValue *graph=(GraphParamValue *)parameters[t].second;
						CGraphParamValueNodeList nodes=graph->getNodes();

						if(retValueConvs[t]!=NULL)
						{
							for(size_t i=0;i<nodes.size();i++)
								nodes[i].y=retValueConvs[t](nodes[i].y);
						}

						actionParameters->setValue<CGraphParamValueNodeList>(graph->getName(),nodes);
						addedParameters.push_back(graph->getName());
					}
					break;

				case ptLFO:
					{
                        LFOParamValue *LFOEntry=(LFOParamValue *)parameters[t].second;
						actionParameters->setValue<CLFODescription>(LFOEntry->getName(),LFOEntry->getValue());
						addedParameters.push_back(LFOEntry->getName());
					}
					break;

				case ptPluginRouting:
					{
                        PluginRoutingParamValue *pluginRoutingEntry=(PluginRoutingParamValue *)parameters[t].second;
						actionParameters->setValue<CPluginMapping>(pluginRoutingEntry->getName(),pluginRoutingEntry->getValue());
						addedParameters.push_back(pluginRoutingEntry->getName());
					}
					break;

				default:
					throw runtime_error(string(__func__)+" -- unhandled parameter type: "+istring(parameters[t].first));
				}
			}
			retval=true;
		}
		catch(EUserMessage &e)
		{
			if(e.what()[0])
			{
				Message(e.what());
				for(size_t t=0;t<addedParameters.size();t++)
					actionParameters->removeKey(addedParameters[t]);
				goto reshow;
			}
			else
				retval=false;;
		}
	}

	// save the splitter's position
	if(showPresetPanel)
	{
		const QByteArray d=apdc.splitter->saveState();
		gSettingsRegistry->setValue<string>("Qt4" DOT "SplitterPositions" DOT presetPrefix+getOrigTitle().toStdString(),urlencode(string(d.constData(),d.size())));
	}

	//hide(); // hide now and ... 
	return retval;
}

void ActionParamDialog::hide()
{
	QDialog::done(0);
}

void ActionParamDialog::onPresetUseButton()
{
	CNestedDataFile *presetsFile;
	QListWidget *listBox;
	if(sender()==apdc.useNativePresetButton || sender()==apdc.nativePresetList)
	{
		presetsFile=gSysPresetsFile;
		listBox=apdc.nativePresetList;
	}
	else //if(sender()==apdc.useUserPresetButton || sender()==apdc.userPresetList)
	{
		presetsFile=gUserPresetsFile;
		listBox=apdc.userPresetList;
	}

	try
	{
		if(!listBox->currentItem())
		{
			gStatusComm->beep();
			return;
		}
		
		const string name=listBox->currentItem()->text().toStdString().substr(4);
		const string title=presetPrefix+getOrigTitle().toStdString() DOT name;

		for(unsigned t=0;t<parameters.size();t++)
		{
			switch(parameters[t].first)
			{
			case ptConstant:
				((ConstantParamValue *)parameters[t].second)->readFromFile(title,presetsFile);
				break;

			case ptNumericText:
			case ptStringText:
				((TextParamValue *)parameters[t].second)->readFromFile(title,presetsFile);
				break;

			case ptDiskEntity:
				((DiskEntityParamValue *)parameters[t].second)->readFromFile(title,presetsFile);
				break;

			case ptComboText:
				((ComboTextParamValue *)parameters[t].second)->readFromFile(title,presetsFile);
				break;

			case ptCheckBox:
				((CheckBoxParamValue *)parameters[t].second)->readFromFile(title,presetsFile);
				break;

			case ptGraph:
			case ptGraphWithWaveform:
				((GraphParamValue *)parameters[t].second)->readFromFile(title,presetsFile);
				break;

			case ptLFO:
                ((LFOParamValue *)parameters[t].second)->readFromFile(title,presetsFile);
				break;

			case ptPluginRouting:
                ((PluginRoutingParamValue *)parameters[t].second)->readFromFile(title,presetsFile);
				break;

			default:
				throw runtime_error(string(__func__)+" -- unhandled parameter type: "+istring(parameters[t].first));
			}
		}
	}
	catch(exception &e)
	{
		Error(e.what());
	}
}

void ActionParamDialog::onPresetSaveButton()
{
	istring name=apdc.userPresetList->currentItem() ? (apdc.userPresetList->currentItem()->text().toStdString().substr(4)) : "";

	askAgain:
	bool ok=false;
	name=QInputDialog::getText(this,_("Preset Name"),_("Preset Name"),QLineEdit::Normal,name.c_str(),&ok).toStdString();
	if(ok)
	{
		if(name.trim()=="")
		{
			Error(_("Invalid Preset Name"));
			goto askAgain;
		}

		// make sure it doesn't contain DOT
		if(name.find(CNestedDataFile::delim)!=string::npos)
		{
			Error(_("Preset Name cannot contain")+string(" '")+string(CNestedDataFile::delim)+"'");
			goto askAgain;
		}

		try
		{
			CNestedDataFile *presetsFile=gUserPresetsFile;


			const string title=presetPrefix+getOrigTitle().toStdString() DOT name;

			bool alreadyExists=false;
			if(presetsFile->keyExists(title))
			{
				alreadyExists=true;
				if(Question(_("Overwrite Existing Preset")+string(" '")+name+"'",yesnoQues)!=yesAns)
					return;
			}

			for(unsigned t=0;t<parameters.size();t++)
			{
				switch(parameters[t].first)
				{
				case ptConstant:
					((ConstantParamValue *)parameters[t].second)->writeToFile(title,presetsFile);
					break;

				case ptNumericText:
				case ptStringText:
					((TextParamValue *)parameters[t].second)->writeToFile(title,presetsFile);
					break;

				case ptDiskEntity:
					((DiskEntityParamValue *)parameters[t].second)->writeToFile(title,presetsFile);
					break;

				case ptComboText:
					((ComboTextParamValue *)parameters[t].second)->writeToFile(title,presetsFile);
					break;

				case ptCheckBox:
					((CheckBoxParamValue *)parameters[t].second)->writeToFile(title,presetsFile);
					break;

				case ptGraph:
				case ptGraphWithWaveform:
					((GraphParamValue *)parameters[t].second)->writeToFile(title,presetsFile);
					break;

				case ptLFO:
                    ((LFOParamValue *)parameters[t].second)->writeToFile(title,presetsFile);
					break;

				case ptPluginRouting:
                    ((PluginRoutingParamValue *)parameters[t].second)->writeToFile(title,presetsFile);
					break;

				default:
					throw runtime_error(string(__func__)+" -- unhandled parameter type: "+istring(parameters[t].first));
				}
			}

			if(!alreadyExists)
			{
				const string key=presetPrefix+getOrigTitle().toStdString() DOT "names";
				vector<string> names=presetsFile->getValue<vector<string> >(key);
				names.push_back(name);
				presetsFile->setValue<vector<string> >(key,names);
				buildPresetList(presetsFile,apdc.userPresetList);
			}
				
			presetsFile->save();
		}
		catch(exception &e)
		{
			Error(e.what());
		}
	}
}

void ActionParamDialog::onPresetRemoveButton()
{
	if(apdc.userPresetList->currentItem())
	{
		string name=apdc.userPresetList->currentItem()->text().toStdString().substr(4);
		if(Question(_("Remove Preset")+string(" '")+name+"'",yesnoQues)==yesAns)
		{
			CNestedDataFile *presetsFile=gUserPresetsFile;

			const string key=presetPrefix+getOrigTitle().toStdString() DOT name;

			// remove preset's values
			presetsFile->removeKey(key);

			// remove preset's name from the names array
			vector<string> names=presetsFile->getValue<vector<string> >(presetPrefix+getOrigTitle().toStdString() DOT "names");
			names.erase(names.begin()+apdc.userPresetList->currentRow());
			presetsFile->setValue<vector<string> >(presetPrefix+getOrigTitle().toStdString() DOT "names",names);

			buildPresetList(presetsFile,apdc.userPresetList);
			presetsFile->save();
		}
	}
	else
		gStatusComm->beep();
}

void ActionParamDialog::buildPresetLists()
{
	if(showPresetPanel)
	{
#if 0
		bool firstTime=presetsFrame==NULL;

		int currentNativePresetItem=nativePresetList ? nativePresetList->getCurrentItem() : 0;
		int currentUserPresetItem=userPresetList ? userPresetList->getCurrentItem() : 0;

		// delete previous stuff
		delete presetsFrame;
			nativePresetList=NULL;
			userPresetList=NULL;

		// (re)create GUI widgets
		presetsFrame=new FXHorizontalFrame(splitter,FRAME_RAISED|FRAME_THICK | LAYOUT_FILL_X, 0,0,0,0, 2,2,2,2);
		try
		{
			if(gSysPresetsFile->getValue<vector<string> >(presetPrefix+getOrigTitle().toStdString() DOT "names").size()>0)
			{
				// native preset stuff
				FXPacker *listFrame=new FXPacker(presetsFrame,FRAME_SUNKEN|FRAME_THICK|LAYOUT_FIX_WIDTH | LAYOUT_FILL_Y, 0,0,210,0, 0,0,0,0, 0,0); // had to do this because FXList won't take that frame style
					nativePresetList=new FXList(listFrame,this,ID_NATIVE_PRESET_LIST,LIST_BROWSESELECT | LAYOUT_FILL_X|LAYOUT_FILL_Y);
					nativePresetList->setNumVisible(4);
				new FXButton(presetsFrame,_("&Use\tOr Double-Click an Item in the List"),NULL,this,ID_NATIVE_PRESET_BUTTON);
			}
		}
		catch(exception &e)
		{
			nativePresetList=NULL;
			Error(e.what());
		}

		// user preset stuff
		FXPacker *listFrame=new FXPacker(presetsFrame,FRAME_SUNKEN|FRAME_THICK|LAYOUT_FIX_WIDTH | LAYOUT_FILL_Y, 0,0,210,0, 0,0,0,0, 0,0); // had to do this because FXList won't take that frame style
			userPresetList=new FXList(listFrame,this,ID_USER_PRESET_LIST,LIST_BROWSESELECT | LAYOUT_FILL_X|LAYOUT_FILL_Y);
			userPresetList->setNumVisible(4);
		FXPacker *buttonGroup=new FXVerticalFrame(presetsFrame);
			new FXButton(buttonGroup,_("&Use\tOr Double-Click an Item in the List"),NULL,this,ID_USER_PRESET_USE_BUTTON,BUTTON_NORMAL|LAYOUT_FILL_X);
			new FXButton(buttonGroup,_("&Save"),NULL,this,ID_USER_PRESET_SAVE_BUTTON,BUTTON_NORMAL|LAYOUT_FILL_X);
			new FXButton(buttonGroup,_("&Remove"),NULL,this,ID_USER_PRESET_REMOVE_BUTTON,BUTTON_NORMAL|LAYOUT_FILL_X);

#endif



		// load presets from file

		if(gSysPresetsFile->keyExists(presetPrefix+getOrigTitle().toStdString() DOT "names"))
		{
			apdc.nativePresetList->show();
			try
			{
				buildPresetList(gSysPresetsFile,apdc.nativePresetList);
				/*
				if(currentNativePresetItem<nativePresetList->count())
					nativePresetList->setCurrentItem(currentNativePresetItem);
				*/
			}
			catch(exception &e)
			{
				Error(e.what());
				apdc.nativePresetList->clear();
			}
		}
		else
			apdc.nativePresetList->hide();
	
		try
		{
			buildPresetList(gUserPresetsFile,apdc.userPresetList);
			/*
			if(currentUserPresetItem<userPresetList->count())
				userPresetList->setCurrentItem(currentUserPresetItem);
			*/
		}
		catch(exception &e)
		{
			Error(e.what());
			apdc.userPresetList->clear();
		}

		/*
		// have to do this after the first showing, otherwise the presets won't show up until you resize the dialog
		if(!firstTime)
			presetsFrame->create();
		*/
	}
	else
		apdc.presetsFrame->hide();
}

void ActionParamDialog::buildPresetList(CNestedDataFile *f,QListWidget *list)
{
	const vector<string> names=f->getValue<vector<string> >(presetPrefix+getOrigTitle().toStdString() DOT "names");
	list->clear();
	for(size_t t=0;t<names.size();t++)
		list->addItem((istring(t,3,true)+" "+names[t]).c_str());
}


unsigned ActionParamDialog::findParamByName(const string name) const
{
	for(unsigned t=0;t<parameters.size();t++)
	{
		switch(parameters[t].first)
		{
		case ptConstant:
			if(((ConstantParamValue *)parameters[t].second)->getName()==name)
				return t;
			break;

		case ptNumericText:
		case ptStringText:
			if(((TextParamValue *)parameters[t].second)->getName()==name)
				return t;
			break;

		case ptDiskEntity:
			if(((DiskEntityParamValue *)parameters[t].second)->getName()==name)
				return t;
			break;

		case ptComboText:
			if(((ComboTextParamValue *)parameters[t].second)->getName()==name)
				return t;
			break;

		case ptCheckBox:
			if(((CheckBoxParamValue *)parameters[t].second)->getName()==name)
				return t;
			break;

		case ptGraph:
		case ptGraphWithWaveform:
			if(((GraphParamValue *)parameters[t].second)->getName()==name)
				return t;
			break;

		case ptLFO:
            if(((LFOParamValue *)parameters[t].second)->getName()==name)
				return t;
			break;

		case ptPluginRouting:
            if(((PluginRoutingParamValue *)parameters[t].second)->getName()==name)
				return t;
			break;

		default:
			throw runtime_error(string(__func__)+" -- unhandled parameter type: "+istring(parameters[t].first));
		}
	}

	throw runtime_error(string(__func__)+" -- no param found named "+name);
}
