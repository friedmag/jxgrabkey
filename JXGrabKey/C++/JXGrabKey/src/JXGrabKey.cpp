/*	Copyright 2008  Edwin Stang (edwinstang@gmail.com), 
 *
 *  This file is part of JXGrabKey.
 *
 *  JXGrabKey is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  JXGrabKey is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with JXGrabKey.  If not, see <http://www.gnu.org/licenses/>.
 */

#define SLEEP_TIME_MS 100
#define MICROSECOND_TO_MILLISECOND_MULTIPLICATOR 1000

#include "JXGrabKey.h"

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <streambuf>
#include <iostream>
#include <sstream>
#include <vector>

using namespace std;

Display *dpy = NULL;
vector<KeyStruct> keys;
bool debug = false;
bool isListening = false;
bool errorInListen = false;
bool doListen = true;
bool registerHotkeyIsWaitingForException = false;
bool registerHotkeyHasException = false;

unsigned int numlock_mask = 0;
unsigned int scrolllock_mask = 0;
unsigned int capslock_mask = 0;

JNIEXPORT void JNICALL Java_jxgrabkey_JXGrabKey_clean
(JNIEnv *_env, jobject _obj) {

    while (!isListening && !errorInListen) {
        if (debug) {
            ostringstream sout;
            sout << "clean() - sleeping " << std::dec << SLEEP_TIME_MS << " ms for listen() to be ready";
            printToDebugCallback(_env, sout.str().c_str());
        }
        usleep(SLEEP_TIME_MS * MICROSECOND_TO_MILLISECOND_MULTIPLICATOR);
    }
    if (errorInListen) {
        if (debug) {
            ostringstream sout;
            sout << "clean() - WARNING: aborting because of error in listen()";
            printToDebugCallback(_env, sout.str().c_str());
        }
        return;
    }

    for (int i = 0; i < keys.size(); i++) {
        Java_jxgrabkey_JXGrabKey_unregisterHotKey(_env, _obj, keys.at(i).id);
    }

    doListen = false;
}

JNIEXPORT void JNICALL Java_jxgrabkey_JXGrabKey_registerHotkey__III
(JNIEnv *_env, jobject _obj, jint _id, jint _mask, jint _key) {

    if (debug) {
        ostringstream sout;
        sout << "++ registerHotkey(" << std::dec << _id << ", 0x" << std::hex << _mask << ", 0x" << std::hex << _key << ")";
        printToDebugCallback(_env, sout.str().c_str());
    }

    while (!isListening && !errorInListen) {
        if (debug) {
            ostringstream sout;
            sout << "registerHotkey() - sleeping " << std::dec << SLEEP_TIME_MS << " ms for listen() to be ready";
            printToDebugCallback(_env, sout.str().c_str());
        }
        usleep(SLEEP_TIME_MS * MICROSECOND_TO_MILLISECOND_MULTIPLICATOR);
    }

    if (errorInListen) {
        if (debug) {
            ostringstream sout;
            sout << "registerHotkey() - WARNING: aborting because of error in listen(): errorInListen = " << std::dec << errorInListen;
            printToDebugCallback(_env, sout.str().c_str());
        }
        return;
    }

    for (int i = 0; i < keys.size(); i++) {
        if (keys[i].id == _id) {
            Java_jxgrabkey_JXGrabKey_unregisterHotKey(_env, _obj, _id);
            break;
        }
    }

    struct KeyStruct key;
    key.id = _id;

    key.key = XKeysymToKeycode(dpy, _key);
    key.mask = _mask;

    keys.push_back(key);

    if (debug) {
        ostringstream sout;

        sout << "registerHotkey() - found in X11 mask (0x" << std::hex << key.mask << "): ";

        if (key.mask & ShiftMask) {
            sout << "'Shift' ";
        }
        if (key.mask & LockMask) {
            sout << "'Lock' ";
        }
        if (key.mask & ControlMask) {
            sout << "'Control' ";
        }
        if (key.mask & Mod1Mask) {
            sout << "'Mod1' ";
        }
        if (key.mask & Mod2Mask) {
            sout << "'Mod2' ";
        }
        if (key.mask & Mod3Mask) {
            sout << "'Mod3' ";
        }
        if (key.mask & Mod4Mask) {
            sout << "'Mod4' ";
        }
        if (key.mask & Mod5Mask) {
            sout << "'Mod5' ";
        }
        
        sout << endl;
        sout << "registerHotkey() - converted X11 keysym '" << XKeysymToString(_key) << "' (0x" << std::hex << _key << ") to X11 key (0x" << std::hex << (int) key.key << ")";


        printToDebugCallback(_env, sout.str().c_str());
    }

    grabKey(_env, key);

    //Flush X to receive errors
    registerHotkeyIsWaitingForException = true;
    XSync(dpy, false);
    if (registerHotkeyHasException) {
        ostringstream message;
        message << "Unable to register hotkey with id = " << _id << ". Each hotkey combination can only be grabbed by one application at a time!";

        jclass cls = _env->FindClass("jxgrabkey/HotkeyConflictException");
        if (cls != NULL) {
            _env->ThrowNew(cls, message.str().c_str());
            //Don't print anything to console until function return,
            //or else java will not register the exception!
        } else {
            if (debug) {
                ostringstream sout;
                sout << "registerHotkey() - WARNING: Unable to throw HotkeyConflictException, class not found!";
                sout << " - " << message.str().c_str();
                printToDebugCallback(NULL, sout.str().c_str());
            }
        }
    }
    registerHotkeyIsWaitingForException = false;
    if (registerHotkeyHasException) {
        registerHotkeyHasException = false;
        return;
    }

    if (debug) {
        ostringstream sout;
        sout << "-- registerHotkey()";
        printToDebugCallback(_env, sout.str().c_str());
    }
}

JNIEXPORT void JNICALL Java_jxgrabkey_JXGrabKey_unregisterHotKey
(JNIEnv *_env, jobject _obj, jint _id) {

    if (debug) {
        ostringstream sout;
        sout << "++ unregisterHotkey(" << std::dec << _id << ")";
        printToDebugCallback(_env, sout.str().c_str());
    }

    while (!isListening && !errorInListen) {
        if (debug) {
            ostringstream sout;
            sout << "unregisterHotkey() - sleeping " << std::dec << SLEEP_TIME_MS << " ms for listen() to be ready";
            printToDebugCallback(_env, sout.str().c_str());
        }
        usleep(SLEEP_TIME_MS * MICROSECOND_TO_MILLISECOND_MULTIPLICATOR);
    }
    if (errorInListen) {
        if (debug) {
            ostringstream sout;
            sout << "unregisterHotkey() - WARNING: aborting because of error in listen()";
            printToDebugCallback(_env, sout.str().c_str());
        }
        return;
    }

    for (int i = 0; i < keys.size(); i++) {
        if (keys.at(i).id == _id) {
            ungrabKey(_env, keys.at(i));
            keys.erase(keys.begin() + i);
            break;
        }
    }

    if (debug) {
        ostringstream sout;
        sout << "-- unregisterHotkey()";
        printToDebugCallback(_env, sout.str().c_str());
    }
}

JNIEXPORT void JNICALL Java_jxgrabkey_JXGrabKey_listen
(JNIEnv *_env, jobject _obj) {

    if (debug) {
        ostringstream sout;
        sout << "++ listen()";
        printToDebugCallback(_env, sout.str().c_str());
    }

    if (isListening) {
        if (debug) {
            ostringstream sout;
            sout << "listen() - WARNING: already listening, aborting";
            printToDebugCallback(_env, sout.str().c_str());
        }
        return;
    }

    jclass cls = _env->FindClass("jxgrabkey/JXGrabKey");
    if (cls == NULL) {
        if (debug) {
            ostringstream sout;
            sout << "listen() - ERROR: cannot find class jxgrabkey.JXGrabKey";
            printToDebugCallback(_env, sout.str().c_str());
        }
        errorInListen = true;
        return;
    }

    jmethodID mid = _env->GetStaticMethodID(cls, "fireKeyEvent", "(I)V");
    if (mid == NULL) {
        if (debug) {
            ostringstream sout;
            sout << "listen() - ERROR: cannot find method fireKeyEvent(int)";
            printToDebugCallback(_env, sout.str().c_str());
        }
        errorInListen = true;
        return;
    }

    dpy = XOpenDisplay(NULL);

    if (!dpy) {
        if (debug) {
            ostringstream sout;
            sout << "listen() - ERROR: cannot open display " << XDisplayName(NULL);
            printToDebugCallback(_env, sout.str().c_str());
        }
        errorInListen = true;
        return;
    }

    getOffendingModifiers(dpy);
    
    int ret = XAllowEvents(dpy, AsyncKeyboard, CurrentTime);
    if(debug && !ret){
        ostringstream sout;
        sout << "listen() - WARNING: XAllowEvents() returned false";
        printToDebugCallback(_env, sout.str().c_str());
    }

    for (int screen = 0; screen < ScreenCount(dpy); screen++) {
        ret = XSelectInput(dpy, RootWindow(dpy, screen), KeyPressMask);
        if(debug && !ret){
            ostringstream sout;
            sout << "listen() - WARNING: XSelectInput() on screen " << std::dec << screen << " returned false";
            printToDebugCallback(_env, sout.str().c_str());
        }
    }

    XSetErrorHandler((XErrorHandler) xErrorHandler);

    doListen = true;
    isListening = true;

    XEvent ev;

    while (doListen) {

        while (!XPending(dpy) && doListen) { //Don't block on XNextEvent(), this breaks XGrabKey()!
            usleep(SLEEP_TIME_MS * MICROSECOND_TO_MILLISECOND_MULTIPLICATOR);
        }

        if (doListen) {
            XNextEvent(dpy, &ev);

            if (ev.type == KeyPress) {
                for (int i = 0; i < keys.size(); i++) {
                    ev.xkey.state &= ~(numlock_mask | capslock_mask | scrolllock_mask); //Filter offending modifiers
                    if (ev.xkey.keycode == keys.at(i).key && ev.xkey.state == keys.at(i).mask) {
                        if (debug) {
                            ostringstream sout;
                            sout << "listen() - received: id = " << std::dec << keys.at(i).id << "; type = KeyPress; x11Keycode = '" << XKeysymToString(XKeycodeToKeysym(dpy, ev.xkey.keycode, 0)) << "' (0x" << std::hex << ev.xkey.keycode << "); x11Mask = 0x" << std::hex << ev.xkey.state << endl;
                            printToDebugCallback(_env, sout.str().c_str());
                        }
                        _env->CallStaticVoidMethod(cls, mid, keys.at(i).id);
                        break;
                    }
                }
            }
        }
    }

    isListening = false;

    XCloseDisplay(dpy);
}

JNIEXPORT void JNICALL Java_jxgrabkey_JXGrabKey_setDebug
(JNIEnv *_env, jobject _obj, jboolean _debug) {
    debug = _debug;
}

void getOffendingModifiers(Display* _dpy) {

    //Took this code from project xbindkeys
    int i;
    XModifierKeymap *modmap;
    KeyCode nlock, slock;
    static int mask_table[8] = {
        ShiftMask, LockMask, ControlMask, Mod1Mask,
        Mod2Mask, Mod3Mask, Mod4Mask, Mod5Mask
    };

    nlock = XKeysymToKeycode(_dpy, XK_Num_Lock);
    slock = XKeysymToKeycode(_dpy, XK_Scroll_Lock);

    modmap = XGetModifierMapping(_dpy);

    if (modmap != NULL && modmap->max_keypermod > 0) {
        for (i = 0; i < 8 * modmap->max_keypermod; i++) {
            if (modmap->modifiermap[i] == nlock && nlock != 0)
                numlock_mask = mask_table[i / modmap->max_keypermod];
            else if (modmap->modifiermap[i] == slock && slock != 0)
                scrolllock_mask = mask_table[i / modmap->max_keypermod];
        }
    }

    capslock_mask = LockMask;

    if (modmap)
        XFreeModifiermap(modmap);
}

void printToDebugCallback(JNIEnv *_env, const char* message) {
    if (debug) {
        static JNIEnv *env = _env;
        if (env != NULL) {
            static jclass cls = env->FindClass("jxgrabkey/JXGrabKey");
            static jmethodID mid = env->GetStaticMethodID(cls, "debugCallback", "(Ljava/lang/String;)V");

            if (mid != NULL) {
                env->CallStaticVoidMethod(cls, mid, env->NewStringUTF(message));
            } else {
                cout << "JAVA DEBUG CALLBACK NOT FOUND - " << message << endl;
                fflush(stdout);
            }
        } else {
            cout << "JAVA DEBUG CALLBACK NOT INITIALIZED - " << message << endl;
            fflush(stdout);
        }
    }
}

static int *xErrorHandler(Display *_dpy, XErrorEvent *_event) {
    if (registerHotkeyIsWaitingForException) {
        //The exception has to be thrown in registerHotkey,
        //coz this functions runs in a different thread!
        registerHotkeyHasException = true;
    }

    if (debug) {
        ostringstream sout;
        sout << "xErrorHandler() - Caught error: serial = " << std::dec << _event->serial;
        sout << "; resourceid = " << std::dec << _event->resourceid;
        sout << "; type = " << std::dec << _event->type;
        sout << "; error_code = " << std::dec << (int) _event->error_code;
        sout << "; request_code = " << std::dec << (int) _event->request_code;
        sout << "; minor_code = " << std::dec << (int) _event->minor_code;
        printToDebugCallback(NULL, sout.str().c_str());
    }
    
    return NULL;
}

void grabKey(JNIEnv *_env, KeyStruct key){
    Mask modifierCombinations[] = {
        key.mask,
        key.mask | numlock_mask,
        key.mask | scrolllock_mask,
        key.mask | capslock_mask,
        key.mask | numlock_mask | scrolllock_mask,
        key.mask | numlock_mask | capslock_mask,
        key.mask | scrolllock_mask | capslock_mask,
        key.mask | numlock_mask | scrolllock_mask | capslock_mask
    };

    for (int screen = 0; screen < ScreenCount(dpy); screen++) {
        for(int m = 0; m < 8; m++){
            int ret = XGrabKey(dpy, key.key, modifierCombinations[m], RootWindow(dpy, screen), True, GrabModeAsync, GrabModeAsync);
            if(debug && !ret){
                ostringstream sout;
                sout << "grabKey() - WARNING: XGrabKey() on screen " << std::dec << screen
                     << " for mask combination " << std::dec << m
                     << " returned false";
                printToDebugCallback(_env, sout.str().c_str());
            }
        }
    }
}

void ungrabKey(JNIEnv *_env, KeyStruct key){
    Mask modifierCombinations[] = {
        key.mask,
        key.mask | numlock_mask,
        key.mask | scrolllock_mask,
        key.mask | capslock_mask,
        key.mask | numlock_mask | scrolllock_mask,
        key.mask | numlock_mask | capslock_mask,
        key.mask | scrolllock_mask | capslock_mask,
        key.mask | numlock_mask | scrolllock_mask | capslock_mask
    };

    for (int screen = 0; screen < ScreenCount(dpy); screen++) {
        for(int m = 0; m < 8; m++){
            int ret = XUngrabKey(dpy, key.key, modifierCombinations[m], RootWindow(dpy, screen));
            if(debug && !ret){
                ostringstream sout;
                sout << "ungrabKey() - WARNING: XUngrabKey() on screen " << std::dec << screen
                     << " for mask combination " << std::dec << m
                     << " returned false";
                printToDebugCallback(_env, sout.str().c_str());
            }
        }
    }
}
