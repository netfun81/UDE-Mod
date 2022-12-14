#ifndef UWM_WORKSPACES_H
#define UWM_WORKSPACES_H

// BEGIN: New includes for new workspace/layer definitions
#include <X11/Xcms.h>

#include "nodes.h"
// END: New includes for new workspace/layer definition

//BEGIN: Old workspace function definitions
#define OnActiveWS(A) (((A)==TheScreen.desktop.ActiveWorkSpace)||((A)==(-1)))

void BroadcastWorkSpacesInfo();
void ChangeWS(short WS);
void StickyWin(UltimateContext *uc);
void WithWin2WS(UltimateContext *uc,short ws);
void Win2WS(UltimateContext *uc,short ws);
void SetWSBackground();
// END: Old workspace function definitions

// BEGIN: New Workspace/Layer Definitions
// New Structure Information
// New Workspace Structure Information Flags
#define UDE_LWS_WS_WorkspaceNumber		0x00000001
#define UDE_LWS_WS_WorkspaceName		0x00000002
#define UDE_LWS_WS_ScreenCommand		0x00000004
#define UDE_LWS_WS_ScreenPixmap			0x00000008
#define UDE_LWS_WS_InactiveBorder		0x00000010
#define UDE_LWS_WS_ActiveBorder			0x00000020
#define	UDE_LWS_WS_InactiveLight		0x00000040
#define UDE_LWS_WS_InactiveShadow		0x00000080
#define UDE_LWS_WS_ActiveLight			0x00000100
#define UDE_LWS_WS_ActiveShadow			0x00000200
#define UDE_LWS_WS_ActiveTitleFont		0x00000400
#define	UDE_LWS_WS_InactiveTitleFont    0x00000800
#define UDE_LWS_WS_LayersList			0x00001000
#define UDE_LWS_WS_WindowsList			0x00002000

// New Layer Structure Information Flags
#define UDE_LWS_LY_LayerNumber			0x00000001
#define UDE_LWS_LY_LayerName			0x00000002
#define UDE_LWS_LY_LayerCommand			0x00000004
#define UDE_LWS_LY_LayerPixmap			0x00000008
#define UDE_LWS_LY_InactiveBorder		0x00000010
#define UDE_LWS_LY_ActiveBorder			0x00000020
#define	UDE_LWS_LY_InactiveLight		0x00000040
#define UDE_LWS_LY_InactiveShadow		0x00000080
#define UDE_LWS_LY_ActiveLight			0x00000100
#define UDE_LWS_LY_ActiveShadow			0x00000200
#define UDE_LWS_LY_ActiveTitleFont		0x00000400
#define	UDE_LWS_LY_InactiveTitleFont	        0x00000800
#define UDE_LWS_LY_LayersList			0x00001000
#define UDE_LWS_LY_WindowsList			0x00002000
#define UDE_LWS_LY_Sticky		       	0x00004000

// New Workspace Structure Information
typedef struct {
	int 		WorkSpaceNumber;
	char 		WorkSpaceName[32];
	char		*ScreenCommand;
	Pixmap		ScreenPixmap;
	XcmsColor	InactiveBorder;
	XcmsColor	ActiveBorder;
	XcmsColor	InactiveLight;
	XcmsColor	InactiveShadow;
	XcmsColor	ActiveLight;
	XcmsColor	ActiveShadow;
	XcmsColor	ActiveTitleFont;
	XcmsColor	InactiveTitleFont;
	NodeList	Layers;
	NodeList	Windows;
	long int	flags;
} UDE_LWS_Workspace;

// New Layer Structure Information
typedef struct {
	int 		LayerNumber;
	char 		LayerName[32];
	char		*LayerCommand;
	Pixmap		LayerPixmap;
	XcmsColor	InactiveBorder;
	XcmsColor	ActiveBorder;
	XcmsColor	InactiveLight;
	XcmsColor	InactiveShadow;
	XcmsColor	ActiveLight;
	XcmsColor	ActiveShadow;
	XcmsColor	ActiveTitleFont;
	XcmsColor	InactiveTitleFont;
	NodeList	Layers;
	NodeList	Windows;
	Bool		Sticky;
	long int	flags;
} UDE_LWS_Layer;

// These three lists will be declared in workspaces.c, and declared static
// so that no other module has direct access to them
extern NodeList Workspaces;
extern NodeList Layers;
extern NodeList WS_Layer_Defaults; // This will be the list which handles setting up
							// defaults for where windows will be mapped to.
// End three listings

void UDE_LWS_InitializeWSL(void);
void UDE_LWS_FinalizeWSL(void);
UDE_LWS_Workspace *UDE_LWS_AllocWorkspace(void);
UDE_LWS_Layer *UDE_LWS_AllocLayer(void);
Bool UDE_LWS_SetWorkspaceAttributes(UDE_LWS_Workspace *, long int flags, UDE_LWS_Workspace *);
Bool UDE_LWS_SetLayerAttributes(UDE_LWS_Layer *, long int flags, UDE_LWS_Layer *);
Bool UDE_LWS_FreeWorkspace(UDE_LWS_Workspace *);
Bool UDE_LWS_FreeLayer(UDE_LWS_Layer *);
Bool UDE_LWS_AddLayerToWorkspace(UDE_LWS_Workspace *);
Bool UDE_LWS_AddLayerToLayer(UDE_LWS_Layer *);
Bool UDE_LWS_AddWindowToWorkspace(Window, UDE_LWS_Workspace *);
Bool UDE_LWS_AddWindowToLayer(Window, UDE_LWS_Layer *);
Bool UDE_LWS_RemoveWindowFromWorkspace(Window, UDE_LWS_Workspace *);
Bool UDE_LWS_RemoveWindowFromLayer(Window, UDE_LWS_Layer *);
Bool UDE_LWS_RemoveWindowFromAll(Window);
Bool UDE_LWS_RemoveLayerFromWorkspace(UDE_LWS_Workspace *);
Bool UDE_LWS_RemoveLayerFromLayer(UDE_LWS_Layer *);
Bool UDE_LWS_KillWSPrograms(UDE_LWS_Workspace *);
Bool UDE_LWS_KillLYPrograms(UDE_LWS_Layer *);
Bool UDE_LWS_HupWSPrograms(UDE_LWS_Workspace *);
Bool UDE_LWS_HupLyPrograms(UDE_LWS_Layer *);
Bool UDE_LWS_HideLayer(UDE_LWS_Layer *);
Bool UDE_LWS_LayerToFront(UDE_LWS_Layer *);
Bool UDE_LWS_LayerToBack(UDE_LWS_Layer *);
Bool UDE_LWS_RefreshDisplay(void);
// END: New Workspace/Layer Definitions

#endif
