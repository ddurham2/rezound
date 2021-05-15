#include "ModalDialog.h"

#include "settings.h"
#include "StatusComm.h"

#include <CNestedDataFile/CNestedDataFile.h>

#include "RezAction.h"

#include <QKeyEvent>
#include <QKeySequence>
#include <QMessageBox>
#include <QMenu>

Q_DECLARE_METATYPE(RezAction*)

#include "ui_KeyBindingsDialogContent.h"
class KeyBindingsDialog : public ModalDialog, private Ui::KeyBindingsDialogContent
{
	Q_OBJECT
public:
    KeyBindingsDialog(QWidget *parent) :
        ModalDialog(parent,_("Hot Keys"))
	{
		Ui::KeyBindingsDialogContent::setupUi(getFrame());
		QMetaObject::connectSlotsByName(this);
	}

    virtual ~KeyBindingsDialog()
	{
	}

    int exec(const map<const string,RezAction *> &actionRegistry)
	{
        for(map<const string,RezAction *>::const_iterator i=actionRegistry.begin(); i!=actionRegistry.end(); i++)
		{
			// prefix the action name with where it falls in the menus
			QString prefix;
			QList<QWidget *> ws=i->second->associatedWidgets();
			bool foundMenu=false;
			// for each widget associated with the menu
			for(int t=0;t<ws.size() && !foundMenu;t++)
			{
				// walk up the parent chain finding QMenus
				QWidget *w=ws[t];
				while(w)
				{
					if(dynamic_cast<QMenu *>(w))
					{
						prefix=((QMenu*)w)->title().replace("&","")+" -> "+prefix;
						foundMenu=true;
					}
					w=w->parentWidget();
				}
			}

			QStringList v;
			v.push_back(prefix+i->second->getStrippedText().c_str());
			v.push_back(i->second->shortcut().toString());
			QTreeWidgetItem *item=new QTreeWidgetItem(v);
			item->setFlags(item->flags() | Qt::ItemIsEditable);
			item->setData(0,Qt::UserRole,QVariant::fromValue(i->second));
			list->addTopLevelItem(item);
		}
		if(list->topLevelItemCount()>0)
		{
			list->resizeColumnToContents(0);
			list->setCurrentItem(list->topLevelItem(0));
		}

		list->sortItems(0,Qt::AscendingOrder);

		int ret=QDialog::exec();

		for(int t=list->topLevelItemCount()-1; t>=0; t--)
		{
			QTreeWidgetItem *i=list->takeTopLevelItem(t);
			if(ret)
			{
                RezAction *a=i->data(0,Qt::UserRole).value<RezAction *>();
				if(i->text(1)=="")
					a->setShortcut(QKeySequence());
				else
					a->setShortcut(QKeySequence::fromString(i->text(1)));

				gKeyBindingsStore->setValue<string>(a->getStrippedUntranslatedText(),a->shortcut().toString().toStdString());
			}
			delete i;
		}
		gKeyBindingsStore->save();
		list->clear();

		return ret;
	}

protected:
	bool eventFilter(QObject *obj,QEvent *ev)
	{
		if(ev->type()==QEvent::KeyPress)
		{
			// NOTE: the TAB key doesn't get captured, there's probably a flag for 
			// that, but I don't think anyone might be hooking up tab as a hotkey 
			// since that would message up focus control.


			QKeyEvent *ke=(QKeyEvent *)ev;

			if(ke->key()<0x1000000) /* not just a modifier key by itself */
			{
				pressed=QKeySequence(ke->key() | ke->modifiers());
				//printf("key pressed: '%s'\n",pressed.toString().toStdString().c_str());

				// close the popup
				mb->accept();
				return true;
			}

			return false;
		}
		return QObject::eventFilter(obj,ev);
	}

private Q_SLOTS:

	void on_list_itemDoubleClicked(QTreeWidgetItem *item, int column)
	{
		if(column!=1)
			return;

		int editingIndex=list->indexOfTopLevelItem(item);

		//popup dialog to get keypress
		mb=new QMessageBox(
			QMessageBox::Question,
			_("Assign Hotkey"),
			_("Press a Key Combination"),
			QMessageBox::NoButton,
			this,
            Qt::Popup);
		QAbstractButton *noneButton=mb->addButton(_("Assign No Key"),QMessageBox::NoRole);
		QAbstractButton *cancelButton=mb->addButton(_("Cancel"),QMessageBox::RejectRole);
		mb->setDefaultButton(NULL);
		mb->installEventFilter(this);
		pressed=QKeySequence();
		mb->exec();
		QAbstractButton *pushed=mb->clickedButton();
		if(pressed.isEmpty())
		{
			if(pushed==noneButton)
			{
				// clear key
				item->setText(1,"");
			}
		}
		else
		{
			const QString key=pressed.toString();

			// clear anything with dup value
			for(int t=0;t<list->topLevelItemCount();t++)
			{
				if(t==editingIndex)
					continue;

				if(list->topLevelItem(t)->text(1)==key)
				{
					const string name=list->topLevelItem(t)->text(0).toStdString();
					if(Question("'"+key.toStdString()+"'"+_(" is already assigned to: ")+name+"\n\n"+_("Do you want to override the existing key assignment?"),yesnoQues)!=yesAns)
						return;

					list->topLevelItem(t)->setText(1,"");
					//break   keep going just in case there are more
				}
			}

			// replace key
			item->setText(1,key);
		}
	}

	void on_assignButton_clicked()
	{
		on_list_itemDoubleClicked(list->currentItem(),1);
	}

private:
	QMessageBox *mb;
	QKeySequence pressed;
};

extern KeyBindingsDialog *gKeyBindingsDialog;
