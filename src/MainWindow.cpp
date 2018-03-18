// MainWindow.cpp
// Generated by Interface Elements (Window v2.1) on Jun 27 1998
// This is a user written class and will not be overwritten.

#include "MainWindow.h"
#include "ThreadItem.h"
#include "TeamListView.h"
#include "SettingsWindow.h"
#include "SlayerApp.h"

#include <stdio.h>
#include <stdlib.h>

#include <AboutWindow.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <ColumnTypes.h>
#include <IconUtils.h>
#include <InterfaceKit.h>
#include <LayoutBuilder.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MainWindow"

extern const char *slayer_signature;

MainWindow::MainWindow(void)
	: BWindow(BRect(200,200,800,750), B_TRANSLATE_SYSTEM_NAME("Slayer"), B_TITLED_WINDOW, 0)
{
	slayer->mainWindow = this;
	refreshThread = NULL;
	if (Lock()) {
		teamView = new TeamListView("MainTeamList");
		// Menü
		BMenuBar* menuBar = new BMenuBar("MenuBar");
		BMenu* menu;
		menu = new BMenu(B_TRANSLATE_SYSTEM_NAME("Slayer"));
		menuBar->AddItem(menu);
		menu->AddItem(new BMenuItem(B_TRANSLATE("About Slayer..."), new BMessage(B_ABOUT_REQUESTED)));
		menu->AddSeparatorItem();
		menu->AddItem(new BMenuItem(B_TRANSLATE("Settings..."), new BMessage(IE_MAINWINDOW_MAINMENU_WINDOWS_SETTINGS)));
		menu->AddSeparatorItem();
		menu->AddItem(new BMenuItem(B_TRANSLATE("Quit"), new BMessage(B_QUIT_REQUESTED), 'Q'));

		menu = new BMenu(B_TRANSLATE("Action"));
		menu->AddItem(new BMenuItem(B_TRANSLATE("Kill"), new BMessage(IE_MAINWINDOW_MAINKILL), 'K'));
		menu->AddItem(new BMenuItem(B_TRANSLATE("Suspend"), new BMessage(IE_MAINWINDOW_MAINSUSPEND), 'S'));
		menu->AddItem(new BMenuItem(B_TRANSLATE("Resume"), new BMessage(IE_MAINWINDOW_MAINRESUME), 'R'));
		menu->AddSeparatorItem();
		priorityMenu = new PriorityMenu(teamView);
		menu->AddItem(priorityMenu);
		priorityMenu->BuildMenu();

		menuBar->AddItem(menu);

		fToolBar = new BToolBar(B_HORIZONTAL);

		BGroupLayout *topBox = BLayoutBuilder::Group<>(this,B_VERTICAL, 0)
			.Add(menuBar)
			.Add(fToolBar)
			.AddGroup(B_VERTICAL)
			.SetInsets(B_USE_WINDOW_INSETS, 0, B_USE_WINDOW_INSETS, B_USE_WINDOW_INSETS)
			.Add(teamView);

		teamView->LoadState(&(slayer->options.columnsState));

		team_items_list = 0;
		team_amount = 0;
		iteration = 0;

		refreshThread = new RefreshThread();
		UpdateTeams();

		if (slayer->options.wind_rect.IsValid()) {
			MoveTo(slayer->options.wind_rect.left,
			       slayer->options.wind_rect.top);
			ResizeTo(slayer->options.wind_rect.Width(),
			         slayer->options.wind_rect.Height());
		}
		minimized = false;
		if (slayer->options.workspace_activation == Options::all_workspaces)
			SetWorkspaces(B_ALL_WORKSPACES);
		else if (slayer->options.workspace_activation == Options::saved_workspace)
		    SetWorkspaces(0x1UL << (slayer->options.workspaces -1 ));

		if (slayer->options.wind_minimized)
			Minimize(true);

		// Quitting has to be disabled if docked
		if (slayer->docked) {
			BMenu *menu = (BMenu *)FindView("MainMenu");
			BMenuItem *item = menu->FindItem(IE_MAINWINDOW_MAINMENU_FILE_QUIT);
			item->SetEnabled(false);
		}

		fToolBar->AddAction(new BMessage(IE_MAINWINDOW_MAINKILL),this,
			ResourceVectorToBitmap("KILL"),B_TRANSLATE("Kill"),"",false);
		fToolBar->AddAction(new BMessage(IE_MAINWINDOW_MAINSUSPEND),this,
			ResourceVectorToBitmap("SUSPEND"),B_TRANSLATE("Suspend"),"",false);
		fToolBar->AddAction(new BMessage(IE_MAINWINDOW_MAINRESUME),this,
			ResourceVectorToBitmap("RESUME"),B_TRANSLATE("Resume"),"",false);
		fToolBar->AddAction(new BMessage(IE_MAINWINDOW_MAINUPDATE),this,
			ResourceVectorToBitmap("FORCED_RELOAD"),B_TRANSLATE("Forced reload"),"",false);
		fToolBar->GetLayout()->AddItem(BSpaceLayoutItem::CreateGlue());
		if (teamView != NULL)
			teamView->MakeFocus();

		SetButtonState();

		refreshThread->Go();
		Unlock();
	}
	Show();
}


void
MainWindow::MenusBeginning()
{
	BRow* sel = teamView->CurrentSelection();
	bool is_sel = (sel != NULL);
	BMenu *menu = (BMenu *)FindView("MenuBar");
	BMenuItem *item = menu->FindItem(IE_MAINWINDOW_MAINKILL);
	if (item) item->SetEnabled(is_sel);
	item = menu->FindItem(IE_MAINWINDOW_MAINSUSPEND);
	if (item) item->SetEnabled(is_sel);
	item = menu->FindItem(IE_MAINWINDOW_MAINRESUME);
	if (item) item->SetEnabled(is_sel);
	priorityMenu->SetEnabled(is_sel);
	priorityMenu->Update();
}

MainWindow::~MainWindow(void)
{
	slayer->mainWindow = NULL;
	refreshThread->Kill();
	delete refreshThread;
	if (!slayer->docked)
		be_app->PostMessage(B_QUIT_REQUESTED);
}

void MainWindow::AttachedToWindow()
{
}

// Handling of user interface and other events
void MainWindow::MessageReceived(BMessage *message)
{
	switch(message->what){
		case SELECTION_CHANGED:
			BRow *row;
			row = teamView->RowAt(message->FindInt32("index"));

			if(row != NULL && message->FindInt32("buttons") == B_SECONDARY_MOUSE_BUTTON){
				BPoint point;
				uint32 state;
				teamView->GetMouse(&point,&state);

				BPoint p2 = teamView->ConvertToScreen(point);
				p2.x -= 5.0;
				p2.y -= 5.0;
				//if(fItemMenu->FindMarked())
				//	fItemMenu->FindMarked()->SetMarked(false);

				teamView->SelectionMessage()->ReplaceInt32("buttons",0);
				teamView->ActionMenu()->Go(p2, true, true, true);
				//fItemMenu->Go(p2, true, true, true);
			}
			SetButtonState();
			break;
		case IE_MAINWINDOW_MAINMENU_ACTION_KILL:
		case IE_MAINWINDOW_MAINKILL:
			DoKill();
		 	UpdateTeams();
		 	SetButtonState();
			break;
		case IE_MAINWINDOW_MAINMENU_ACTION_SUSPEND:
		case IE_MAINWINDOW_MAINSUSPEND:
			DoSuspend();
			UpdateTeams();
			SetButtonState();
			break;
		case IE_MAINWINDOW_MAINMENU_ACTION_RESUME:
		case IE_MAINWINDOW_MAINRESUME:
			DoResume();
			UpdateTeams();
			SetButtonState();
			break;
		case SET_PRIORITY: {
			int32 priority = message->FindInt32("priority");
			DoPriority(priority);
			UpdateTeams();
			SetButtonState();
			break;
		}
		case IE_MAINWINDOW_MAINPRIORITYVALUE:
			// takes priority from text field
			DoPriority();
			UpdateTeams();
			SetButtonState();
			break;
		case IE_MAINWINDOW_MAINUPDATE:
			UpdateTeams();
			SetButtonState();
			break;
		case B_ABOUT_REQUESTED:    // "About…" is selected from menu…
		{
			BAboutWindow* fAboutWin = new BAboutWindow(B_TRANSLATE_SYSTEM_NAME("Slayer"), slayer_signature);
			fAboutWin->AddDescription(B_TRANSLATE("A thread manager for Haiku"));
			fAboutWin->SetVersion(SLAYER_VERSION);
			fAboutWin->AddCopyright(1999, "Arto Jalkanen");
			const char* authors[] = {
				"Arto Jalkanen (ajalkane@cc.hut.fi)",
				NULL
			};
			fAboutWin->AddAuthors(authors);
			fAboutWin->Show();
		}
			break;
		case IE_MAINWINDOW_MAINMENU_FILE_DOCS__:
		{
			BMessage message(B_REFS_RECEIVED);
			message.AddString("url", ProjectWebsite);
			be_roster->Launch("text/html", &message);
		}
			break;
		case IE_MAINWINDOW_MAINMENU_FILE_QUIT:    // "Quit" is selected from menu…
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;
		case IE_MAINWINDOW_MAINMENU_WINDOWS_SETTINGS:
		{
			const char* windowSettingsTitle = B_TRANSLATE("Settings");
			BWindow *settings = slayer->FindWindow(windowSettingsTitle);
			if (!settings)
				new SettingsWindow(windowSettingsTitle);
			else if (settings->Lock()) {
				settings->Activate(true);
				settings->Unlock();
			}
		}
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}

}


void MainWindow::Quit()
{
	slayer->options.wind_rect = Frame();
	teamView->SaveState(&slayer->options.columnsState);
//	slayer->options.wind_minimized = minimized;

	// What follows is a really ugly hack to detect if the user closed
	// the window with close button, or if Application wants to close
	// all windows:
	BMessage *msg = CurrentMessage();  // this is null if called outside BMessageReceived loop
	                                   // -> message from application
	if (slayer->docked && msg != NULL)
		Minimize(true);
	else
		BWindow::Quit();
}

void MainWindow::Minimize(bool minimize)
{
	minimized = minimize;
	BWindow::Minimize(minimize);

	// if window is minimized, remove update thread. If window un-minimized, restart it.
	if (minimized && refreshThread != NULL) {
		// Gotta get rid of the refresh thread
		refreshThread->Kill();
	}
	else if (!minimized && refreshThread != NULL) {
		refreshThread->Go();
	}
}

// unfortunately Minimize isn't called when the window is un-minimized,
// so here is a dirty hack around it.
void MainWindow::WindowActivated(bool active)
{
	if (active) {
		if (minimized) {
			minimized = false;
			// Update the teams
			UpdateTeams();
			// restart updating
			if (refreshThread != NULL) refreshThread->Go();
		}
	}
}
void MainWindow::UpdateTeams()
{
	ThreadItem *thread_item;
	TeamItem   *team_item;

	DisableUpdates();

	if (!team_items_list)
		team_items_list = new Hashtable;

	app_info inf;
	thread_info thinf;
	int32 th_cookie, te_cookie;
	int32 i;

	team_info teinf;

	te_cookie = 0;

	iteration = (iteration + 1) % 2;

	total_CPU_diff = 0;
	int idle_CPU_diff = 0;

	for (i = 0; get_next_team_info(&te_cookie, &teinf) == B_NO_ERROR; i++) {
		if (!(team_item = (TeamItem *)team_items_list->get(teinf.team))) {
			team_item = new TeamItem(&teinf);
			team_item->refreshed = iteration;
			team_items_list->put(teinf.team, team_item);
			teamView->AddRow(team_item);

			team_item->CPU_diff = 0;
			for (th_cookie = 0; get_next_thread_info(team_item->team, &th_cookie, &thinf) == B_OK;) {
				thread_item = new ThreadItem(&thinf);
				thread_item->refreshed = iteration;
				team_item->thread_items_list->put(thinf.thread, thread_item);
				teamView->AddRow(thread_item, team_item);
				if (teinf.team != 1 || strncmp(thinf.name, "idle thread ", 12) != 0) {
					team_item->CPU_diff += thread_item->CPU_diff;
				} else
					idle_CPU_diff += thread_item->CPU_diff;
			}
		}
		// update team
		else {
			team_item->update(&teinf);
			team_item->refreshed = iteration;

			team_item->CPU_diff = 0;
			for (th_cookie = 0; get_next_thread_info(team_item->team, &th_cookie, &thinf) == B_OK;) {
				if (!(thread_item = (ThreadItem *)team_item->thread_items_list->get(thinf.thread))) {
					thread_item = new ThreadItem(&thinf);
					thread_item->refreshed = iteration;
					team_item->thread_items_list->put(thinf.thread, thread_item);
					teamView->AddRow(thread_item, team_item);
				}
				// update thread
				else {
					thread_item->update(&thinf);
					thread_item->refreshed = iteration;
				}
				if (teinf.team != 1 || strncmp(thinf.name, "idle thread ", 12) != 0) {
					team_item->CPU_diff += thread_item->CPU_diff;
				} else
					idle_CPU_diff += thread_item->CPU_diff;
			}

		}
		total_CPU_diff += team_item->CPU_diff;
		if (total_CPU_diff < 0) printf("Error. CPU diff out of bounds\n");
	}
	total_CPU_diff += idle_CPU_diff;

	// division by zero && overflow handling
	if (total_CPU_diff <= 0) total_CPU_diff = 1;

	RemoveList.MakeEmpty();
	teamView->FullListDoForEach(postlistproc, (void *)this);
	RemoveProcessItems(&RemoveList);

	EnableUpdates();
}

void MainWindow::RemoveProcessItems(BList *items)
{
	BRow **p = (BRow **)items->Items();
	int32 i;
	for (i = 0; i < items->CountItems(); i++) {
		teamView->RemoveRow(p[i]);
		if (p[i]->HasLatch()) {
			team_items_list->del(((TeamItem *)p[i])->team);
			delete ((TeamItem *)p[i]);
		}
		else {
			// find which team this belongs to
			TeamItem *team_item = (TeamItem *)team_items_list->get(((ThreadItem *)p[i])->team);
			// can be null if the team is already taken away
			if (team_item != NULL)
				team_item->thread_items_list->del(((ThreadItem *)p[i])->thread);

			delete ((ThreadItem *)p[i]);
		}
	}
}


bool postlistproc(BRow *item, void *_wnd)
{
	MainWindow *wnd = (MainWindow *)_wnd;
	if (item->HasLatch()) {
		if (((TeamItem *)item)->refreshed != wnd->iteration)
			wnd->RemoveList.AddItem((void *)item);
		else {/*
			int32 ch = ((TeamItem *)item)->changed;
			while (ch) {
				BRect rect(0.0, 0.0, 0.0, 0.0);
				if (ch & TeamItem::name_chg) {
					rect = item->ItemColumnFrame(TeamListView::name_ndx, wnd->teamView);
					ch &= ~(TeamItem::name_chg);
				}
				else if (ch & TeamItem::areas_chg) {
					rect = item->ItemColumnFrame(TeamListView::areas_ndx, wnd->teamView);
					ch &= ~(TeamItem::areas_chg);
				}
				else ch = 0;
				if (rect.right && rect.bottom) wnd->teamView->Invalidate(rect);
			}
			((TeamItem *)item)->changed = 0;*/

			float CPU = ((float)((TeamItem *)item)->CPU_diff) / wnd->total_CPU_diff;
			if ((CPU != ((TeamItem *)item)->CPU) // && wnd->teamView->IsColumnVisible(6)
			    /*(slayer->options.shown_columns & Options::cpu_col)*/) {

				CPU = (CPU > 1.0 ? 1.0 : CPU < 0.0 ? 0.0 : CPU);
				((TeamItem *)item)->CPU = CPU;
				/*
				item->DrawItemColumn(wnd->teamView, item->ItemColumnFrame(
					TeamListView::CPU_ndx, wnd->teamView), TeamListView::CPU_ndx, true); */
				((BIntegerField*)(item->GetField(6)))->SetValue(CPU*100);
				((TeamItem *)item)->changed++;
			}
			if (((TeamItem *)item)->changed != 0) {
				wnd->teamView->UpdateRow(item);
				//item->Invalidate();
			}
			((TeamItem *)item)->changed = 0;
		}
	}
	else {
		if (((ThreadItem *)item)->refreshed != wnd->iteration)
			wnd->RemoveList.AddItem((void *)item);
		else { /*
			int32 ch = ((ThreadItem *)item)->changed;
			while (ch) {
				BRect rect(0.0, 0.0, 0.0, 0.0);
				if (ch & ThreadItem::name_chg) {
					rect = item->ItemColumnFrame(TeamListView::name_ndx, wnd->teamView);
					ch &= ~(ThreadItem::name_chg);
				}
				else if (ch & ThreadItem::priority_chg) {
					rect = item->ItemColumnFrame(TeamListView::priority_ndx, wnd->teamView);
					ch &= ~(ThreadItem::priority_chg);
				}
				else if (ch & ThreadItem::state_chg) {
					rect = item->ItemColumnFrame(TeamListView::state_ndx, wnd->teamView);
					ch &= ~(ThreadItem::state_chg);
				}
				else ch = 0;

				if (rect.right && rect.bottom) wnd->teamView->Invalidate(rect);
			}
			((ThreadItem *)item)->changed = 0; */

			float CPU = ((float)((ThreadItem *)item)->CPU_diff) / wnd->total_CPU_diff;
			if ((CPU != ((ThreadItem *)item)->CPU)
				/* && wnd->teamView->IsColumnVisible(6) */) {

			    CPU = (CPU > 1.0 ? 1.0 : CPU < 0.0 ? 0.0 : CPU);
				((ThreadItem *)item)->CPU = CPU;
				/*
				item->DrawItemColumn(wnd->teamView, item->ItemColumnFrame(
					TeamListView::CPU_ndx, wnd->teamView), TeamListView::CPU_ndx, true); */
					((BIntegerField*)(item->GetField(6)))->SetValue(CPU*100);
					((ThreadItem *)item)->changed++;
			}
			if (((ThreadItem *)item)->changed != 0){
				wnd->teamView->UpdateRow(item);
				//item->Invalidate();
			}
			((ThreadItem *)item)->changed = 0;
		}
	}
	return false;
}


void MainWindow::DoKill(void)
{
	BRow*		selected = NULL;
	while ((selected = teamView->CurrentSelection(selected))) {
		if (selected->HasLatch())
			kill_team(((TeamItem *)selected)->team);
		else
			kill_thread(((ThreadItem *)selected)->thread);
	}
}

void MainWindow::DoPriority(int32 priority)
{
	BRow*		selected = NULL;
	while ((selected = teamView->CurrentSelection(selected))) {
		// is a team or thread?
		if (selected->HasLatch())
			for (int i = 0; i < teamView->CountRows(selected); i++)
				set_thread_priority(((ThreadItem *)teamView->RowAt(i, selected))->thread, priority);
		else
			set_thread_priority(((ThreadItem *)selected)->thread, priority);
	}
}

void MainWindow::DoPriority()
{
	BTextControl *PriorityValue = (BTextControl *)FindView("MainPriorityValue");
	if (strcmp("", PriorityValue->Text())) {
		int32 value;
		value = atoi(PriorityValue->Text());
		DoPriority(value);
	}
}

void MainWindow::DoSuspend(void)
{
	BRow*		selected = NULL;
	while ((selected = teamView->CurrentSelection(selected))) {
		// is a team or thread?
		if (selected->HasLatch())
			for (int i = 0; i < teamView->CountRows(selected); i++)
				suspend_thread(((ThreadItem *)teamView->RowAt(i, selected))->thread);
		else
			suspend_thread(((ThreadItem *)selected)->thread);
	}
}

void MainWindow::DoResume(void)
{
	BRow*		selected = NULL;
	while ((selected = teamView->CurrentSelection(selected))) {
		// is a team or thread?
		if (selected->HasLatch())
			for (int i = 0; i < teamView->CountRows(selected); i++)
				resume_thread(((ThreadItem *)teamView->RowAt(i, selected))->thread);
		else
			resume_thread(((ThreadItem *)selected)->thread);
	}
}

void MainWindow::SetButtonState()
{
	BRow* sel = teamView->CurrentSelection();
	bool is_sel = (sel != NULL);

	fToolBar->FindButton(IE_MAINWINDOW_MAINKILL)->SetEnabled(is_sel);
	fToolBar->FindButton(IE_MAINWINDOW_MAINSUSPEND)->SetEnabled(is_sel);
	fToolBar->FindButton(IE_MAINWINDOW_MAINRESUME)->SetEnabled(is_sel);


}


BBitmap* MainWindow::ResourceVectorToBitmap(const char *resName, float iconSize)
{
	BResources res;
	size_t size;
	app_info appInfo;

	be_app->GetAppInfo(&appInfo);
	BFile appFile(&appInfo.ref, B_READ_ONLY);
	res.SetTo(&appFile);
	BBitmap *aBmp = NULL;
	const uint8* iconData = (const uint8*) res.LoadResource('VICN', resName, &size);

	if (size > 0 ) {
		aBmp = new BBitmap (BRect(0,0, iconSize, iconSize), 0, B_RGBA32);
		status_t result = BIconUtils::GetVectorIcon(iconData, size, aBmp);
		if (result != B_OK) {
			delete aBmp;
			aBmp = NULL;
		}
	}
	return aBmp;
}
