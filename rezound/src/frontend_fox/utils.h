#ifndef __utils_h__
#define __utils_h__

static void setFontOfAllChildren(FXComposite *w,FXFont *f)
{
	if(w)
	{
		for(FXint t=0;t<w->numChildren();t++)
		{
			FXObject *c=w->childAtIndex(t);
			if(dynamic_cast<FXComposite *>(c))
				setFontOfAllChildren(static_cast<FXComposite *>(c),f);
			else if(dynamic_cast<FXComboBox *>(c))
				static_cast<FXComboBox *>(c)->setFont(f);
			else if(dynamic_cast<FXGroupBox *>(c))
				static_cast<FXGroupBox *>(c)->setFont(f);
			else if(dynamic_cast<FXHeader *>(c))
				static_cast<FXHeader *>(c)->setFont(f);
			else if(dynamic_cast<FXIconList *>(c))
				static_cast<FXIconList *>(c)->setFont(f);
			else if(dynamic_cast<FXLabel *>(c))
				static_cast<FXLabel *>(c)->setFont(f);
			else if(dynamic_cast<FXListBox *>(c))
				static_cast<FXListBox *>(c)->setFont(f);
			else if(dynamic_cast<FXList *>(c))
				static_cast<FXList *>(c)->setFont(f);
			else if(dynamic_cast<FXMDIChild *>(c))
				static_cast<FXMDIChild *>(c)->setFont(f);
			else if(dynamic_cast<FXMenuCaption *>(c))
				static_cast<FXMenuCaption *>(c)->setFont(f);
			else if(dynamic_cast<FXProgressBar *>(c))
				static_cast<FXProgressBar *>(c)->setFont(f);
			else if(dynamic_cast<FXRuler *>(c))
				static_cast<FXRuler *>(c)->setFont(f);
			else if(dynamic_cast<FXSpinner *>(c))
				static_cast<FXSpinner *>(c)->setFont(f);
			else if(dynamic_cast<FXStatusLine *>(c))
				static_cast<FXStatusLine *>(c)->setFont(f);
			else if(dynamic_cast<FXTable *>(c))
				static_cast<FXTable *>(c)->setFont(f);
			else if(dynamic_cast<FXTextField *>(c))
				static_cast<FXTextField *>(c)->setFont(f);
			else if(dynamic_cast<FXText *>(c))
				static_cast<FXText *>(c)->setFont(f);
			else if(dynamic_cast<FXToolTip *>(c))
				static_cast<FXToolTip *>(c)->setFont(f);
			else if(dynamic_cast<FXTreeListBox *>(c))
				static_cast<FXTreeListBox *>(c)->setFont(f);
			else if(dynamic_cast<FXTreeList *>(c))
				static_cast<FXTreeList *>(c)->setFont(f);

		}
	}
}

#endif
