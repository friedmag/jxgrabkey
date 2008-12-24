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

#define SLEEP_TIME 100

#include "JXGrabKey.h"
#include <X11/Xlib.h>
#include <iostream>
#include <sstream>
#include <vector>

using namespace std;

struct KeyStruct {
    int id;
    KeyCode key;
    Mask mask;
};

Display *dpy;
bool debug = false;
bool isListening = false;
bool errorInListen = false;
vector<KeyStruct> keys;

JNIEXPORT void JNICALL Java_jxgrabkey_JXGrabKey_clean
  (JNIEnv *_env, jobject _obj){
    if(debug){
        ostringstream sout;
        sout << "++ clean()" << endl;
        printToDebugCallback(_env, sout.str().c_str());
    }
    while(!isListening && !errorInListen){
        if(debug){
            ostringstream sout;
            sout << "clean() - sleeping " << std::dec << SLEEP_TIME << " ms for listen() to be ready" << endl;
            printToDebugCallback(_env, sout.str().c_str());
        }
        usleep(SLEEP_TIME*1000);
    }
    if(errorInListen){
        if(debug){
            ostringstream sout;
            sout << "clean() - aborting because of error in listen(): errorInListen = " << std::dec << errorInListen << endl;
            printToDebugCallback(_env, sout.str().c_str());
        }
        return;
    }

    for(int i = 0; i < keys.size(); i++){
        Java_jxgrabkey_JXGrabKey_unregisterHotKey(_env, _obj, keys.at(i).id);
    }
    
    if(debug){
        ostringstream sout;
        sout << "-- clean()" << endl;
        printToDebugCallback(_env, sout.str().c_str());
    }
}

JNIEXPORT void JNICALL Java_jxgrabkey_JXGrabKey_registerHotkey__III
  (JNIEnv *_env, jobject _obj, jint _id, jint _mask, jint _key){
    if(debug){
        ostringstream sout;
        sout << "++ registerHotkey(" << std::dec << _id << ", 0x" << std::hex << _mask << ", 0x" << std::hex << _key << ")" << endl;
        printToDebugCallback(_env, sout.str().c_str());
    }
    
    while(!isListening && !errorInListen){
        if(debug){
            ostringstream sout;
            sout << "registerHotkey() - sleeping " << std::dec << SLEEP_TIME << " ms for listen() to be ready" << endl;
            printToDebugCallback(_env, sout.str().c_str());
        }
        usleep(SLEEP_TIME*1000);
    }
    
    if(errorInListen){
        if(debug){
            ostringstream sout;
            sout << "registerHotkey() - aborting because of error in listen(): errorInListen = " << std::dec << errorInListen << endl;
            printToDebugCallback(_env, sout.str().c_str());
        }
        return;
    }
    
    struct KeyStruct key;
    key.id = _id;
    
    key.key = XKeysymToKeycode(dpy, _key);
    key.mask = _mask;

    keys.push_back(key);
    
    if(debug){
        ostringstream sout;

        sout << "registerHotkey() - converted x11Keysym '" <<  XKeysymToString(_key) << "' (0x" << std::hex << _key << ") to x11Keycode (0x" << std::hex << (int)key.key << ")" << endl;

        sout << "registerHotkey() - found in x11Mask (0x" << std::hex << key.mask << "): ";

        if(key.mask & ShiftMask){
            sout << "'Shift' ";
        }
        if(key.mask & LockMask){
            sout << "'Lock' ";
        }
        if(key.mask & ControlMask){
            sout << "'Control' ";
        }
        if(key.mask & Mod1Mask){
            sout << "'Mod1' ";
        }
        if(key.mask & Mod2Mask){
            sout << "'Mod2' ";
        }
        if(key.mask & Mod3Mask){
            sout << "'Mod3' ";
        }
        if(key.mask & Mod4Mask){
            sout << "'Mod4' ";
        }
        if(key.mask & Mod5Mask){
            sout << "'Mod5' ";
        }
        
        sout << endl;

        printToDebugCallback(_env, sout.str().c_str());
    }

    for (int screen = 0; screen < ScreenCount(dpy); screen++){
        int ret = XGrabKey(dpy, key.key, key.mask, RootWindow(dpy, screen), True, GrabModeAsync, GrabModeAsync);
        if(debug){
            ostringstream sout;
            sout << "registerHotkey() - XGrabKey() returned '" << getErrorString(ret) << "' (" << std::dec << ret << ")" << endl;
            printToDebugCallback(_env, sout.str().c_str());
        }
    }
    
    if(debug){
        ostringstream sout;
        sout << "-- registerHotkey()" << endl;
        printToDebugCallback(_env, sout.str().c_str());
    }
}

JNIEXPORT void JNICALL Java_jxgrabkey_JXGrabKey_unregisterHotKey
  (JNIEnv *_env, jobject _obj, jint _id){
    if(debug){
        ostringstream sout;
        sout << "++ unregisterHotkey(" << std::dec << _id << ")" << endl;
        printToDebugCallback(_env, sout.str().c_str());
    }
    
    while(!isListening && !errorInListen){
        if(debug){
            ostringstream sout;
            sout << "unregisterHotkey() - sleeping " << std::dec << SLEEP_TIME << " ms for listen() to be ready" << endl;
            printToDebugCallback(_env, sout.str().c_str());
        }
        usleep(SLEEP_TIME*1000);
    }
    if(errorInListen){
        if(debug){
            ostringstream sout;
            sout << "unregisterHotkey() - aborting because of error in listen(): errorInListen = " << std::dec << errorInListen << endl;
            printToDebugCallback(_env, sout.str().c_str());
        }
        return;
    }
    
    for(int i = 0; i < keys.size(); i++){
        if(keys.at(i).id == _id){
            for (int screen = 0; screen < ScreenCount(dpy); screen++){
                int ret = XUngrabKey(dpy, keys.at(i).key, keys.at(i).mask, RootWindow(dpy, screen));
                if(debug){
                    ostringstream sout;
                    sout << "unregisterHotkey() - XUngrabKey() returned '" << getErrorString(ret) << "' (" << std::dec << ret << ")" << endl;
                    printToDebugCallback(_env, sout.str().c_str());
                }
            }
            keys.erase(keys.begin()+i);
            break;
        }
    }
    
    if(debug){
        ostringstream sout;
        sout << "-- unregisterHotkey()" << endl;
        printToDebugCallback(_env, sout.str().c_str());
    }
}

JNIEXPORT void JNICALL Java_jxgrabkey_JXGrabKey_listen
  (JNIEnv *_env, jobject _obj){ 
    
    if(debug){
        ostringstream sout;
        sout << "++ listen()" << endl;
        printToDebugCallback(_env, sout.str().c_str());
    }
    
    if(isListening){
        if(debug){
            ostringstream sout;
            sout << "listen() - WARNING: already listening, aborting: isListening = " << std::dec << isListening << endl;
            printToDebugCallback(_env, sout.str().c_str());
        }
        return;
    }
    
    jclass cls = _env->FindClass("jxgrabkey/JXGrabKey");
    if(cls == NULL){
        if(debug){
            ostringstream sout;
            sout << "listen() - ERROR: cannot find class jxgrabkey.JXGrabKey" << endl;
            printToDebugCallback(_env, sout.str().c_str());
        }
        errorInListen = true;
        return;
    }
    
    jmethodID mid = _env->GetStaticMethodID(cls, "fireKeyEvent", "(I)V" );
    if(mid == NULL){
        if(debug){
            ostringstream sout;
            sout << "listen() - ERROR: cannot find method fireKeyEvent(int)" << endl;
            printToDebugCallback(_env, sout.str().c_str());
        }
        errorInListen = true;
        return;
    }

    dpy = XOpenDisplay(NULL);
    
    if(!dpy){
        if(debug){
            ostringstream sout;
            sout << "listen() - ERROR: cannot open display " << XDisplayName(NULL) << endl;
            printToDebugCallback(_env, sout.str().c_str());
        }
        errorInListen = true;
        return;
    }

    isListening = true;

    XEvent ev;
    
    if(debug){
        ostringstream sout;
        sout << "listen() - finished initialization on display " << XDisplayName(NULL) << endl;
        printToDebugCallback(_env, sout.str().c_str());
    }
    
    while(true){
        while(!XPending(dpy)){ //Don't block on XNextEvent(), this breaks XGrabKey()!
            usleep(SLEEP_TIME*1000);
        }
        
        XNextEvent(dpy, &ev);
        if(debug){
            ostringstream sout;
            switch(ev.type){
                case KeyPress:
                    sout << "listen() - received: type = KeyPress; x11Keycode = '" << XKeysymToString(XKeycodeToKeysym(dpy, ev.xkey.keycode, 0)) << "' (0x" << std::hex << ev.xkey.keycode << "); x11Mask = 0x" << std::hex << ev.xkey.state << endl;
                    break;
                case KeyRelease:
                    sout << "listen() - received: type = KeyRelease; x11Keycode = '" << XKeysymToString(XKeycodeToKeysym(dpy, ev.xkey.keycode, 0)) << "' (0x" << std::hex << ev.xkey.keycode << "); x11Mask = 0x" << std::hex << ev.xkey.state << endl;
                    break;
                default:
                    sout << "listen() - received unknown XEvent: type = " << std::dec << ev.type << endl;
            }
            printToDebugCallback(_env, sout.str().c_str());
        }
        if(ev.type == KeyPress){
            for(int i = 0; i < keys.size(); i++){
                if(ev.xkey.keycode == keys.at(i).key && ev.xkey.state == keys.at(i).mask){
                    if(debug){
                        ostringstream sout;
                        sout << "listen() - found suitable key registration: id = " << std::dec << keys.at(i).id << endl;;
                        printToDebugCallback(_env, sout.str().c_str());
                    }
                    _env->CallStaticVoidMethod(cls, mid, keys.at(i).id);
                    break;
                }
            }
        }
    }
}

JNIEXPORT void JNICALL Java_jxgrabkey_JXGrabKey_setDebug
  (JNIEnv *_env, jobject _obj, jboolean _debug){
    debug = _debug;
}

const char* getErrorString(int errorCode){
    switch(errorCode){
        case BadAccess:
            return "BadAccess";
        case BadValue:
            return "BadValue";
        case BadWindow:
            return "BadWindow";
        case 1:
            return "Success";
        default:
            return "Unknown";
    }
}

void printToDebugCallback(JNIEnv *_env, const char* message){
    if(debug){
        static jclass cls = _env->FindClass("jxgrabkey/JXGrabKey");
        static jmethodID mid = _env->GetStaticMethodID(cls, "debugCallback", "(Ljava/lang/String;)V" );

        if(mid != NULL){
            _env->CallStaticVoidMethod(cls, mid, _env->NewStringUTF(message));
        }else{
            cout << "DEBUG HANDLER NOT FOUND - " << message;
            fflush(stdout);
        }
    }
}
