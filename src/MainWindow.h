// MainWindow.h
// Generated by Interface Elements (Window v2.1) on Jun 27 1998
// This is a user written class and will not be overwritten.

#ifndef _MAINWINDOW_H_
#define _MAINWINDOW_H_

#include "MainWindowDefs.h"
#include "TeamItem.h"
#include "TeamListView.h"

#include "Hashtable.h"
#include "PriorityMenu.h"

#include <ToolBar.h>

bool
postlistproc(BRow *, void *);

static const BString ProjectWebsite =
	"https://github.com/HaikuArchives/Slayer/blob/master/DOCS.txt";

class MainWindow : public BWindow {
  public:
	// true if window minimized
	bool minimized;

	TeamListView *teamView;
	float barPos;

	int32 team_amount;
	Hashtable *team_items_list;
	BList RemoveList;

	// which iteration.. used in updating teams
	int32 iteration;

	// used to calculate CPU usage by thread / team
	bigtime_t total_CPU_diff;

	MainWindow(void);
	~MainWindow(void);

	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage *message);
	virtual void Quit();
	virtual void Minimize(bool minimize);
	virtual void MenusBeginning();

	void UpdateTeams();
	void RemoveProcessItems(BList *);
	void SaveStatus();
	void DoKill();
	void DoPriority(int32 priority);
	void DoPriority();
	void DoSuspend();
	void DoResume();
	void SetButtonState();

	void SetRefreshRate(int32);

	BBitmap *ResourceVectorToBitmap(const char *resName, float iconSize = 24.0);

	PriorityMenu *priorityMenu;
	BToolBar *fToolBar;
	BMessageRunner *fRefreshRunner;
	int32 fRefreshRate;
};

#endif
