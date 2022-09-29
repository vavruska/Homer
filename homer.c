
#pragma rtl
#include <types.h>
#include <stdbool.h>
#include <string.h>

#include <finder.h>
#include <gsos.h>
#include <locator.h>
#include <memory.h>
#include <menu.h>
#include <misctool.h>
#include <orca.h>
#include <resources.h>
#include <window.h>

#define HOMER_NAME         "\pCCV~Homer~"
#define HOMER_MENU         "\pLocate in Finder"

//globals
word userID;
word finderSaidHello;
word myMenuId;

typedef struct StringList {
    word                count;
    char                wString;
} StringList;

typedef struct ExtStringList {
    word                count;
    word                offset;
    word                iconYBot;
    word                iconYMid;
    word                iconYText;
    word                iconHeight;
    word                iconWidth;
    Handle              iconHandle;
    char                wString;
} ExtStringList;


Word doHello(finderSaysHelloInPtr dataIn) {
    tellFinderAddToExtrasOut     addExtraOut = {0};
    MenuItemTemplate             myItem = { 0 };
    word result = 0;
    static char menuItem[] = HOMER_MENU;

    if (finderSaidHello == 0) {
        myItem.version = 0x8000;
        myItem.itemChar = 'L';
        myItem.itemAltChar = 'l';
        myItem.itemTitleRef = (Ref)menuItem;
        
        // add the menu item to the Extras Menu        
        SendRequest(tellFinderAddToExtras, stopAfterOne + sendToName,
                    (Long)&NAME_OF_FINDER, (Long)&myItem, (ptr)&addExtraOut);
        if (!toolerror()) {
            myMenuId = addExtraOut.menuItemID;
            DisableMItem(myMenuId); //disabled by default
            result = 0x8000;
            finderSaidHello = 1;
        }
    }

    return result;
}

word doGoodbye(void) {
    word result = 0;

    if (finderSaidHello) {
        tellFinderRemoveFromExtrasOut removeExtraOut;

        finderSaidHello = 0;

        SendRequest(tellFinderRemoveFromExtras, stopAfterOne + sendToName,
                    (Long)&NAME_OF_FINDER, (Long)myMenuId, (Ptr)&removeExtraOut);

        result = 0x8000;
    }
    return result;
}

word doGoAway(srqGoAwayOutPtr dataOut) {
    word result = doGoodbye();

    dataOut->resultID = userID;
    dataOut->resultFlags = 0x8000;
    return result;
}

void openPathWindow(GSString255Ptr path, bool isEasyMount) {
    char *replace[1];
    FileInfoRecGS info = { 5, path };
    GetFileInfoGS(&info);
    if ((toolerror() == fileNotFound) || (toolerror() == pathNotFound)) {
        //filenotfound alert
        Handle pathHand = NewHandle(path->length+1, userID, attrLocked, NULL);
        if (!toolerror()) {
            memset(*pathHand, 0, path->length + 1);
            PtrToHand((pointer) path->text, pathHand, path->length);
            replace[0] = *pathHand;
            SysBeep2(sbOperationImpossible);
            AlertWindow(awButtonLayout, (pointer) &replace, (long)"42/The path \"*0\" no longer exists./^#0");
            DisposeHandle(pathHand);
        }
    } else {
        bool found = false;
        WindowPtr cur = FrontWindow();
        tellFinderOpenWindowOut dataOut;
        tellFinderSetSelectedIconsOut setOut;
        tellFinderGetSelectedIconsOut getOut;
        StringList **icons;
        ExtStringList **selected;
        GSString255 locPath;
        word newX, newY;
        Rect r;
        long dataSize;

        memcpy(&locPath, path, sizeof(GSString255));
        while (locPath.length && (locPath.text[locPath.length - 1] != ':') &&
               (locPath.text[locPath.length - 1] != '/')) {
            locPath.length--;
        }
        if (locPath.length) {
            //remove the trailing delimeter
            locPath.length--;
        }
        while (cur) {
            tellFinderGetWindowInfoOut winOut;
            SendRequest(tellFinderGetWindowInfo, stopAfterOne + sendToName,
                        (Long)&NAME_OF_FINDER, (Long)&locPath, (Ptr)&winOut);
            if (winOut.windowType == 0x0002) {
                GSString255Ptr windPath = (GSString255Ptr)winOut.windPath;
                if (memcpy(path, windPath, windPath->length) == 0) {
                    found = true;
                    SelectWindow(cur);
                    break;
                }
            }
            cur = GetNextWindow(cur);
        }
        if (!found) {
            SendRequest(tellFinderOpenWindow, stopAfterOne + sendToName,
                        (Long)&NAME_OF_FINDER, (Long)&locPath, (Ptr)&dataOut);
        }
        icons = (StringList **) NewHandle(sizeof(StringList) + sizeof(GSString255), userID, attrLocked, NULL);
        (*icons)->count = 1;
        memcpy(&(*icons)->wString, path, sizeof(GSString255));

        SendRequest(tellFinderSetSelectedIcons, stopAfterOne + sendToName,
                    (Long)&NAME_OF_FINDER, (Long)icons, (Ptr)&setOut);
        DisposeHandle((Handle) icons);
        //get the selected icon with the extended StringList
        SendRequest(tellFinderGetSelectedIcons, stopAfterOne + sendToName,
                    (Long)&NAME_OF_FINDER, (Long)0x80000000L, (Ptr)&getOut);
        selected = (ExtStringList **)getOut.stringListHandle;
        if (((*selected)->count == 1) && isEasyMount) {
            cur = FrontWindow();
            dataSize = GetDataSize(cur);
            GetRectInfo(&r, cur);
            newY = (((cur->portRect.v2 - cur->portRect.v1) - (r.v2 - r.v1)) / 2) + (r.v1 - 1);
            if ((*selected)->iconYText) {
                //list view
                newY = (*selected)->iconYText - newY;
            } else {
                //icon view
                newY = (*selected)->iconYBot - newY;
            }
            newY = (newY & 0x8000) ? 0 : newY; //protect against negatives
            //if the icon is at the bottom of the directory the
            if (newY > ((dataSize & 0xFFFF) - (cur->portRect.v2 - cur->portRect.v1))) {
                newY = (dataSize & 0xFFFF) - (cur->portRect.v2 - cur->portRect.v1);
            }
            if ((*selected)->iconYText) {
                //list view default the X coord to 0
                newX = 0; 
            } else {
                //icon view so adjust the x coord
                newX = (*selected)->iconYMid - (*selected)->iconWidth;
                newX -= ((cur->portRect.h2 - cur->portRect.h1) / 2);
                newX = (newX & 0x8000) ? 0 : newX;
            }
            SetContentOrigin(newX, newY, cur);
        }
    }
}

void checkEasyMount(GSString255Ptr path) {
    GSString255 infoPath = { 0 }; 
    OpenRecGS openRec = { 12, 0 };
    openRec.pathname = path;
    char *afp;

    OpenGS(&openRec);
    if (!toolerror()) {
        Handle file = NewHandle(openRec.eof, userID, attrLocked, NULL);
        IORecGS readRec = {4, openRec.refNum, *file, openRec.eof, 0};
        RefNumRecGS closeRec = { 1, openRec.refNum };

        ReadGS(&readRec);
        if (!toolerror() && (openRec.eof == readRec.transferCount)) {
            //see if the data is for a local easymount document
            char *data = *file;
            word state = 0;
            bool good = true;

            while ((data <= (*file) + openRec.eof) && (state < 3)) {
                switch (state) {
                case 0: 
                    for (; data < ((*file) + 16); data++) {
                        if (*data != 0) {
                            good = false;
                            break;
                        }
                    }
                    break;
                case 1: 
                    data = *file + 0xad;
                    if ((*data != 1) || (*(++data) != 0)) {
                        good = false;
                    }
                    break;
                case 2: 
                    data++;
                }
                if (good) {
                    state++;
                } else {
                    break;
                }
            }
            if (good) {
                infoPath.length = *data;
                data++;
                strncpy(infoPath.text, data, infoPath.length);
                openPathWindow(&infoPath, true);
            } else {
                afp =  *file + (*file)[0] + 2;
                SysBeep2(sbOperationImpossible);
                if (strncmp(afp, "AFPServer", 9) == 0) {
                    AlertWindow(awButtonLayout, NULL, (long)"32/Homer is unable to handle AppleShare EasyMount Document./^#0");
                }
            }
        } else {
            SysBeep2(sbOperationFailed);
        }
        CloseGS(&closeRec);
        DisposeHandle(file);
    } else {
        SysBeep2(sbOperationFailed);
    }
}

word doExtras(long menuId) {
    tellFinderGetSelectedIconsOut dataOut = {0};
    StringList *iconList;
    word handled = 0x0000;

    if (menuId == myMenuId) {
        SendRequest(tellFinderGetSelectedIcons, stopAfterOne + sendToName,
                (Long)&NAME_OF_FINDER, (Long)0L, (Ptr)&dataOut);
        iconList = (StringList *)*(dataOut.stringListHandle);

        if (iconList->count == 1) {
            FileInfoRecGS info = { 4, 0 };
            info.pathname = (GSString255Ptr) &(iconList->wString);
            GetFileInfoGS(&info);
            if (!toolerror()) {
                handled = 0x8000;
                if ((info.fileType == 0x00E2) && (info.auxType = 0xFFFF)) {
                    //look inside the easymount document open the path
                    checkEasyMount(info.pathname);
                } else {
                    openPathWindow(info.pathname, false);
                }
            } else {
                SysBeep2(sbOperationFailed);
            }
        }
    }
    return handled;
}

void scanIcons(void) {
    tellFinderGetSelectedIconsOut dataOut;
    tellFinderGetWindowInfoOut wInfo = {0};
    StringList *iconList;
    bool enabled = false;

    SendRequest(tellFinderGetSelectedIcons, stopAfterOne + sendToName,
                (Long)&NAME_OF_FINDER, (Long)0L, (Ptr)&dataOut);
    if (!toolerror()) {
        iconList = (StringList *)*dataOut.stringListHandle;
        if (iconList->count == 1) {
            enabled = true;
        }
    }
    if (enabled) {
        EnableMItem(myMenuId);
    } else {
        DisableMItem(myMenuId);
    }
}


#pragma toolparms 1
#pragma databank 1
pascal word myRequest(word request, Long dataIn, Long dataOut) {
    word currentApp;
    word result = 0x0000;

    currentApp = GetCurResourceApp();

    if (finderSaidHello != 0) {
        SetCurResourceApp(userID);
    }

    switch (request) {
    case srqGoAway:
        result = doGoAway((srqGoAwayOutPtr)dataOut);
        break;
    case finderSaysHello:
        result = doHello((finderSaysHelloInPtr)dataIn);
        break;
    case finderSaysGoodbye:
        result = doGoodbye();
        break;
    case finderSaysExtrasChosen:
        result = doExtras(dataIn);
        break;
    case finderSaysSelectionChanged:
        scanIcons();
        break;
    default:
        break;
    }

    SetCurResourceApp(currentApp);

    return result;
}
#pragma databank 0
#pragma toolparms 0


int main(void) {
    finderSaidHello = 0;
    userID = MMStartUp();
    static char homer[] = HOMER_NAME;

    if (!toolerror()) {
        AcceptRequests(homer, userID, &myRequest);
    }

    return 0;
}

