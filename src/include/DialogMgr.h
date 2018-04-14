#if !defined(_DIALOG_H_)
#define _DIALOG_H_

/*
 * Copyright 1986, 1989, 1990 by Abacus Research and Development, Inc.
 * All rights reserved.
 *

 */

#include "WindowMgr.h"
#include "TextEdit.h"

#define MODULE_NAME DialogMgr
#include <rsys/api-module.h>

namespace Executor
{
enum
{
    ctrlItem = 4,
    btnCtrl = 0,
    chkCtrl = 1,
    radCtrl = 2,
    resCtrl = 3,
    statText = 8,
    editText = 16,
    iconItem = 32,
    picItem = 64,
    userItem = 0,
    itemDisable = 128,
};

enum
{
    OK = 1,
    Cancel = 2,
};

enum
{
    stopIcon = 0,
    noteIcon = 1,
    cautionIcon = 2,
};

struct DialogRecord
{
    GUEST_STRUCT;
    GUEST<WindowRecord> window;
    GUEST<Handle> items;
    GUEST<TEHandle> textH;
    GUEST<INTEGER> editField;
    GUEST<INTEGER> editOpen;
    GUEST<INTEGER> aDefItem;
};
typedef DialogRecord *DialogPeek;

typedef CWindowPtr CDialogPtr;
typedef WindowPtr DialogPtr;

/* dialog record accessors */
#define DIALOG_WINDOW(dialog) ((WindowPtr) & ((DialogPeek)dialog)->window)

#define DIALOG_ITEMS_X(dialog) (((DialogPeek)(dialog))->items)
#define DIALOG_TEXTH_X(dialog) (((DialogPeek)(dialog))->textH)
#define DIALOG_EDIT_FIELD_X(dialog) (((DialogPeek)(dialog))->editField)
#define DIALOG_EDIT_OPEN_X(dialog) (((DialogPeek)(dialog))->editOpen)
#define DIALOG_ADEF_ITEM_X(dialog) (((DialogPeek)(dialog))->aDefItem)

#define DIALOG_ITEMS(dialog) (MR(DIALOG_ITEMS_X(dialog)))
#define DIALOG_TEXTH(dialog) (MR(DIALOG_TEXTH_X(dialog)))
#define DIALOG_EDIT_FIELD(dialog) (CW(DIALOG_EDIT_FIELD_X(dialog)))
#define DIALOG_EDIT_OPEN(dialog) (CW(DIALOG_EDIT_OPEN_X(dialog)))
#define DIALOG_ADEF_ITEM(dialog) (CW(DIALOG_ADEF_ITEM_X(dialog)))

struct DialogTemplate
{
    GUEST_STRUCT;
    GUEST<Rect> boundsRect;
    GUEST<INTEGER> procID;
    GUEST<BOOLEAN> visible;
    GUEST<BOOLEAN> filler1;
    GUEST<BOOLEAN> goAwayFlag;
    GUEST<BOOLEAN> filler2;
    GUEST<LONGINT> refCon;
    GUEST<INTEGER> itemsID;
    GUEST<Str255> title;
};

typedef DialogTemplate *DialogTPtr;

typedef GUEST<DialogTPtr> *DialogTHndl;

// This has a 50% chance of being right.
// It does not seem to be used, however.
typedef struct PACKED
{
    unsigned boldItm4 : 1;
    unsigned boxDrwn4 : 1;
    unsigned sound4 : 2;
    unsigned boldItm3 : 1;
    unsigned boxDrwn3 : 1;
    unsigned sound3 : 2;
    unsigned boldItm2 : 1;
    unsigned boxDrwn2 : 1;
    unsigned sound2 : 2;
    unsigned boldItm1 : 1;
    unsigned boxDrwn1 : 1;
    unsigned sound1 : 2;
} StageList;

struct AlertTemplate
{
    GUEST_STRUCT;
    GUEST<Rect> boundsRect;
    GUEST<INTEGER> itemsID;
    GUEST<StageList> stages;
};
typedef AlertTemplate *AlertTPtr;

typedef GUEST<AlertTPtr> *AlertTHndl;

enum
{
    overlayDITL = 0,
    appendDITLRight = 1,
    appendDITLBottom = 2,
};

typedef int16_t DITLMethod;

enum
{
    TEdoFont = 1,
    TEdoFace = 2,
    TEdoSize = 4,
    TEdoColor = 8,
    TEdoAll = 15,
};

enum
{
    TEaddSize = 16,
};

enum
{
    doBColor = 8192,
    doMode = 16384,
    doFontName = 32768,
};

typedef UPP<void(INTEGER soundNumber)> SoundProcPtr;
typedef UPP<Boolean(DialogPtr theDialog, EventRecord *theEvent, GUEST<INTEGER> *itemHit)> ModalFilterProcPtr;
typedef UPP<Boolean(DialogPtr theDialog, EventRecord *theEvent, GUEST<INTEGER> *itemHit, void *yourDataPtr)> ModalFilterYDProcPtr;
typedef UPP<void(DialogPtr theDialog, INTEGER itemNo)> UserItemProcPtr;

const LowMemGlobal<ProcPtr> ResumeProc { 0xA8C }; // DialogMgr IMI-411 (true);
const LowMemGlobal<INTEGER> ANumber { 0xA98 }; // DialogMgr IMI-423 (true);
const LowMemGlobal<INTEGER> ACount { 0xA9A }; // DialogMgr IMI-423 (true);
const LowMemGlobal<SoundProcPtr> DABeeper { 0xA9C }; // DialogMgr IMI-411 (true);
const LowMemGlobal<Handle[4]> DAStrings { 0xAA0 }; // DialogMgr IMI-421 (true);
const LowMemGlobal<INTEGER> DlgFont { 0xAFA }; // DialogMgr IMI-412 (true);

DISPATCHER_TRAP(DialogDispatch, 0xAA68, D0W);

extern INTEGER C_Alert(INTEGER id,
                                   ModalFilterProcPtr fp);
PASCAL_TRAP(Alert, 0xA985);
extern INTEGER C_StopAlert(INTEGER id,
                                       ModalFilterProcPtr fp);
PASCAL_TRAP(StopAlert, 0xA986);
extern INTEGER C_NoteAlert(INTEGER id,
                                       ModalFilterProcPtr fp);
PASCAL_TRAP(NoteAlert, 0xA987);
extern INTEGER C_CautionAlert(INTEGER id,
                                          ModalFilterProcPtr fp);
PASCAL_TRAP(CautionAlert, 0xA988);
extern void C_CouldAlert(INTEGER id);
PASCAL_TRAP(CouldAlert, 0xA989);
extern void C_FreeAlert(INTEGER id);
PASCAL_TRAP(FreeAlert, 0xA98A);
extern void C_CouldDialog(INTEGER id);
PASCAL_TRAP(CouldDialog, 0xA979);
extern void C_FreeDialog(INTEGER id);
PASCAL_TRAP(FreeDialog, 0xA97A);
extern DialogPtr C_NewDialog(Ptr dst,
                                         Rect *r, StringPtr tit, BOOLEAN vis, INTEGER procid,
                                         WindowPtr behind, BOOLEAN gaflag, LONGINT rc, Handle items);
PASCAL_TRAP(NewDialog, 0xA97D);
extern DialogPtr C_GetNewDialog(INTEGER id,
                                            Ptr dst, WindowPtr behind);
PASCAL_TRAP(GetNewDialog, 0xA97C);
extern void C_CloseDialog(DialogPtr dp);
PASCAL_TRAP(CloseDialog, 0xA982);
extern void C_DisposeDialog(DialogPtr dp);
PASCAL_TRAP(DisposeDialog, 0xA983);
extern BOOLEAN C_ROMlib_myfilt(DialogPtr dlg, EventRecord *evt,
                                      GUEST<INTEGER> *ith);
PASCAL_FUNCTION(ROMlib_myfilt);

extern void C_ModalDialog(ModalFilterProcPtr fp,
                                      GUEST<INTEGER> *item);
PASCAL_TRAP(ModalDialog, 0xA991);
extern BOOLEAN C_IsDialogEvent(
    EventRecord *evt);
PASCAL_TRAP(IsDialogEvent, 0xA97F);
extern void C_DrawDialog(DialogPtr dp);
PASCAL_TRAP(DrawDialog, 0xA981);
extern INTEGER C_FindDialogItem(DialogPtr dp,
                                       Point pt);
PASCAL_TRAP(FindDialogItem, 0xA984);
extern void C_UpdateDialog(DialogPtr dp,
                                     RgnHandle rgn);
PASCAL_TRAP(UpdateDialog, 0xA978);
extern BOOLEAN C_DialogSelect(
    EventRecord *evt, GUEST<DialogPtr> *dpp, GUEST<INTEGER> *item);
PASCAL_TRAP(DialogSelect, 0xA980);
extern void DialogCut(DialogPtr dp);
extern void DialogCopy(DialogPtr dp);
extern void DialogPaste(DialogPtr dp);
extern void DialogDelete(DialogPtr dp);
extern void C_ROMlib_mysound(INTEGER i);
PASCAL_FUNCTION(ROMlib_mysound);
extern void C_ErrorSound(SoundProcPtr sp);
PASCAL_TRAP(ErrorSound, 0xA98C);
extern void C_InitDialogs(ProcPtr rp);
PASCAL_TRAP(InitDialogs, 0xA97B);
extern void SetDialogFont(INTEGER i);
extern void C_ParamText(StringPtr p0,
                                    StringPtr p1, StringPtr p2, StringPtr p3);
PASCAL_TRAP(ParamText, 0xA98B);
extern void C_GetDialogItem(DialogPtr dp,
                                   INTEGER itemno, GUEST<INTEGER> *itype, GUEST<Handle> *item, Rect *r);
PASCAL_TRAP(GetDialogItem, 0xA98D);
extern void C_SetDialogItem(DialogPtr dp,
                                   INTEGER itemno, INTEGER itype, Handle item, Rect *r);
PASCAL_TRAP(SetDialogItem, 0xA98E);
extern void C_GetDialogItemText(Handle item,
                                   StringPtr text);
PASCAL_TRAP(GetDialogItemText, 0xA990);
extern void C_SetDialogItemText(Handle item,
                                   StringPtr text);
PASCAL_TRAP(SetDialogItemText, 0xA98F);
extern void C_SelectDialogItemText(DialogPtr dp,
                                   INTEGER itemno, INTEGER start, INTEGER stop);
PASCAL_TRAP(SelectDialogItemText, 0xA97E);
extern INTEGER GetAlertStage(void);
extern void ResetAlertStage(void);
extern void C_HideDialogItem(DialogPtr dp,
                                    INTEGER item);
PASCAL_TRAP(HideDialogItem, 0xA827);
extern void C_ShowDialogItem(DialogPtr dp,
                                    INTEGER item);
PASCAL_TRAP(ShowDialogItem, 0xA828);

extern CDialogPtr C_NewColorDialog(Ptr, Rect *, StringPtr, BOOLEAN, INTEGER, WindowPtr, BOOLEAN, LONGINT, Handle);
PASCAL_TRAP(NewColorDialog, 0xAA4B);

extern OSErr C_GetStdFilterProc(GUEST<ProcPtr> *proc);
PASCAL_SUBTRAP(GetStdFilterProc, 0xAA68, 0x0203, DialogDispatch);
extern OSErr C_SetDialogDefaultItem(DialogPtr dialog,
                                                int16_t new_item);
PASCAL_SUBTRAP(SetDialogDefaultItem, 0xAA68, 0x0304, DialogDispatch);
extern OSErr C_SetDialogCancelItem(DialogPtr dialog,
                                               int16_t new_item);
PASCAL_SUBTRAP(SetDialogCancelItem, 0xAA68, 0x0305, DialogDispatch);
extern OSErr C_SetDialogTracksCursor(DialogPtr dialog,
                                                 Boolean tracks);
PASCAL_SUBTRAP(SetDialogTracksCursor, 0xAA68, 0x0306, DialogDispatch);

extern void AppendDITL(DialogPtr, Handle, DITLMethod);
extern void ShortenDITL(DialogPtr, int16_t);
extern int16_t CountDITL(DialogPtr);
}

#endif /* _DIALOG_H_ */
