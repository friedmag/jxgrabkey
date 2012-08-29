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

#include <pthread.h>
#include <unistd.h>
#include <streambuf>
#include <iostream>
#include <sstream>
#include <vector>

using namespace std;

Display *dpy = NULL;
vector<KeyStruct> keys;
vector<CallbackStruct> callbacks;
vector<string> displays;
vector<Display*> dps;
bool debug = false;
bool isListening = false;
bool errorInListen = false;
bool doListen = true;
bool registerHotkeyIsWaitingForException = false;
bool registerHotkeyHasException = false;

pthread_mutex_t x_lock;

unsigned int numlock_mask = 0;
unsigned int scrolllock_mask = 0;
unsigned int capslock_mask = 0;

static void unregisterHotkey(JNIEnv* _env, int i) {
  ungrabKey(_env, keys.at(i));
  keys.erase(keys.begin() + i);
}

JNIEXPORT void JNICALL Java_com_jxgrabkey_JXGrabKey_clean(JNIEnv *_env,
		jobject _obj) {

	while (!isListening && !errorInListen) {
		if (debug) {
			ostringstream sout;
			sout << "clean() - sleeping " << std::dec << SLEEP_TIME_MS
					<< " ms for listen() to be ready";
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

	pthread_mutex_lock(&x_lock);
	for (int i = 0; i < keys.size(); i++) {
    unregisterHotkey(_env, i);
	}
	pthread_mutex_unlock(&x_lock);

	doListen = false;
}

JNIEXPORT void JNICALL Java_com_jxgrabkey_JXGrabKey_registerHotkey__III(
		JNIEnv *_env, jobject _obj, jint _id, jint _mask, jint _key) {

	if (debug) {
		ostringstream sout;
		sout << "++ registerHotkey(" << std::dec << _id << ", 0x" << std::hex
				<< _mask << ", 0x" << std::hex << _key << ")";
		printToDebugCallback(_env, sout.str().c_str());
	}

	while (!isListening && !errorInListen) {
		if (debug) {
			ostringstream sout;
			sout << "registerHotkey() - sleeping " << std::dec << SLEEP_TIME_MS
					<< " ms for listen() to be ready";
			printToDebugCallback(_env, sout.str().c_str());
		}
		usleep(SLEEP_TIME_MS * MICROSECOND_TO_MILLISECOND_MULTIPLICATOR);
	}

	if (errorInListen) {
		if (debug) {
			ostringstream sout;
			sout
					<< "registerHotkey() - WARNING: aborting because of error in listen(): errorInListen = "
					<< std::dec << errorInListen;
			printToDebugCallback(_env, sout.str().c_str());
		}
		return;
	}

	pthread_mutex_lock(&x_lock);
	for (int i = 0; i < keys.size(); i++) {
		if (keys[i].id == _id) {
			Java_com_jxgrabkey_JXGrabKey_unregisterHotKey(_env, _obj, _id);
			break;
		}
	}

	struct KeyStruct key;
	key.id = _id;
	key.key = XKeysymToKeycode(dpy, _key);
	key.mask = _mask;
	keys.push_back(key);
	grabKey(_env, key);
	pthread_mutex_unlock(&x_lock);

	if (debug) {
		ostringstream sout;

		sout << "registerHotkey() - found in X11 mask (0x" << std::hex
				<< key.mask << "): ";

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
		sout << "registerHotkey() - converted X11 keysym '" << XKeysymToString(
				_key) << "' (0x" << std::hex << _key << ") to X11 key (0x"
				<< std::hex << (int) key.key << ")";

		printToDebugCallback(_env, sout.str().c_str());
	}

	//Flush X to receive errors
	registerHotkeyIsWaitingForException = true;
	XSync(dpy, false);
	if (registerHotkeyHasException) {
		ostringstream message;
		message << "Unable to register hotkey with id = " << _id
				<< ". Each hotkey combination can only be grabbed by one application at a time!";

		jclass cls = _env->FindClass("com/jxgrabkey/HotkeyConflictException");
		if (cls != NULL) {
			_env->ThrowNew(cls, message.str().c_str());
			//Don't print anything to console until function return,
			//or else java will not register the exception!
		} else {
			if (debug) {
				ostringstream sout;
				sout
						<< "registerHotkey() - WARNING: Unable to throw HotkeyConflictException, class not found!";
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

JNIEXPORT void JNICALL Java_com_jxgrabkey_JXGrabKey_unregisterHotKey(JNIEnv *_env,
		jobject _obj, jint _id) {

	if (debug) {
		ostringstream sout;
		sout << "++ unregisterHotkey(" << std::dec << _id << ")";
		printToDebugCallback(_env, sout.str().c_str());
	}

	while (!isListening && !errorInListen) {
		if (debug) {
			ostringstream sout;
			sout << "unregisterHotkey() - sleeping " << std::dec
					<< SLEEP_TIME_MS << " ms for listen() to be ready";
			printToDebugCallback(_env, sout.str().c_str());
		}
		usleep(SLEEP_TIME_MS * MICROSECOND_TO_MILLISECOND_MULTIPLICATOR);
	}
	if (errorInListen) {
		if (debug) {
			ostringstream sout;
			sout
					<< "unregisterHotkey() - WARNING: aborting because of error in listen()";
			printToDebugCallback(_env, sout.str().c_str());
		}
		return;
	}

	pthread_mutex_lock(&x_lock);
	for (int i = 0; i < keys.size(); i++) {
		if (keys.at(i).id == _id) {
      unregisterHotkey(_env, i);
			break;
		}
	}
	pthread_mutex_unlock(&x_lock);

	if (debug) {
		ostringstream sout;
		sout << "-- unregisterHotkey()";
		printToDebugCallback(_env, sout.str().c_str());
	}
}

JNIEXPORT void JNICALL Java_com_jxgrabkey_JXGrabKey_setDisplays(JNIEnv *_env,
		jobject _obj, jobjectArray _displays) {
  printf("setDisplays called!\n"); fflush(stdout);
  jstring jstr;
  const char* cstr;
  displays.clear();
  for (int i = 0; i < _env->GetArrayLength(_displays); ++i) {
    jstr = (jstring)_env->GetObjectArrayElement(_displays, i);
    cstr = _env->GetStringUTFChars(jstr, NULL);
    displays.push_back(cstr);
    _env->ReleaseStringUTFChars(jstr, cstr);
  }
}

JNIEXPORT void JNICALL Java_com_jxgrabkey_JXGrabKey_listen(JNIEnv *_env,
		jobject _obj) {

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

	jclass cls = _env->FindClass("com/jxgrabkey/JXGrabKey");
	if (cls == NULL) {
		if (debug) {
			ostringstream sout;
			sout << "listen() - ERROR: cannot find class jxgrabkey.JXGrabKey";
			printToDebugCallback(_env, sout.str().c_str());
		}
		errorInListen = true;
		return;
	}

	jmethodID mid = _env->GetStaticMethodID(cls, "fireKeyEvent", "(ILcom/jxgrabkey/JXEvent;)V");
	if (mid == NULL) {
		if (debug) {
			ostringstream sout;
			sout << "listen() - ERROR: cannot find method fireKeyEvent(JXEvent)";
			printToDebugCallback(_env, sout.str().c_str());
		}
		errorInListen = true;
		return;
	}

  for (int i = 0; i < displays.size(); ++i) {
    printf("register %s\n", displays[i].c_str()); fflush(stdout);
    if (debug) {
      ostringstream sout;
      sout << "listen() - registering for " << displays[i];
      printToDebugCallback(_env, sout.str().c_str());
    }
    dpy = XOpenDisplay(displays[i].c_str());
    dps.push_back(dpy);

    if (!dpy) {
      if (debug) {
        ostringstream sout;
        sout << "listen() - ERROR: cannot open display " << XDisplayName(
            NULL);
        printToDebugCallback(_env, sout.str().c_str());
      }
      errorInListen = true;
      return;
    }

    getOffendingModifiers(dpy);

    int ret = XAllowEvents(dpy, AsyncKeyboard, CurrentTime);
    if (debug && !ret) {
      ostringstream sout;
      sout << "listen() - WARNING: XAllowEvents() returned false";
      printToDebugCallback(_env, sout.str().c_str());
    }

    for (int screen = 0; screen < ScreenCount(dpy); screen++) {
      ret = XSelectInput(dpy, RootWindow(dpy, screen), KeyPressMask);
      if (debug && !ret) {
        ostringstream sout;
        sout << "listen() - WARNING: XSelectInput() on screen " << std::dec
          << screen << " returned false";
        printToDebugCallback(_env, sout.str().c_str());
      }
    }
  }

	XSetErrorHandler((XErrorHandler) xErrorHandler);
	pthread_mutex_init(&x_lock, NULL);

	doListen = true;
	isListening = true;

	XEvent ev;
  Display* found;

	while (doListen) {

    found = NULL;
    while (found == NULL && doListen) {
      pthread_mutex_lock(&x_lock);
      for (int i = 0; i < dps.size(); ++i) {
        if (XPending(dps[i]) && doListen) { //Don't block on XNextEvent(), this breaks XGrabKey()!
          found = dps[i];
        }
      }
      pthread_mutex_unlock(&x_lock);
      if (!found && doListen) {
        usleep(SLEEP_TIME_MS * MICROSECOND_TO_MILLISECOND_MULTIPLICATOR);
      }
    }

		if (doListen) {
			XNextEvent(found, &ev);

			if (ev.type == KeyPress) {
				pthread_mutex_lock(&x_lock);
				for (int i = 0; i < keys.size(); i++) {
          KeyStruct& key = keys.at(i);
					ev.xkey.state &= ~(numlock_mask | capslock_mask
							| scrolllock_mask); //Filter offending modifiers
					if (ev.xkey.keycode == key.key && ev.xkey.state
							== key.mask) {
						if (debug) {
							ostringstream sout;
							sout << "listen() - received: id = " << std::dec
									<< key.id
									<< "; type = KeyPress; x11Keycode = '"
									<< XKeysymToString(XKeycodeToKeysym(found,
											ev.xkey.keycode, 0)) << "' (0x"
									<< std::hex << ev.xkey.keycode
									<< "); x11Mask = 0x" << std::hex
									<< ev.xkey.state << endl;
							printToDebugCallback(_env, sout.str().c_str());
						}

            CallbackStruct callback;
            callback.id = key.id;
            callback.point = getPoint(_env, found);
            callbacks.push_back(callback);
						break;
					}
				}
				pthread_mutex_unlock(&x_lock);

        printToDebugCallback(_env, "listen() - sending callbacks");
        for (int i = 0; i < callbacks.size(); ++i) {
          _env->CallStaticVoidMethod(cls, mid, callbacks[i].id, callbacks[i].point);
        }
        callbacks.clear();
        printToDebugCallback(_env, "listen() - callbacks complete");
			}
		}
	}

  printToDebugCallback(_env, "listen() - exiting!");
	isListening = false;

	pthread_mutex_destroy(&x_lock);
  for (int i = 0; i < dps.size(); ++i)
    XCloseDisplay(dps[i]);
}

JNIEXPORT void JNICALL Java_com_jxgrabkey_JXGrabKey_setDebug(JNIEnv *_env,
		jobject _obj, jboolean _debug) {
	debug = _debug;
}

void getOffendingModifiers(Display* _dpy) {

	//Took this code from project xbindkeys
	int i;
	XModifierKeymap *modmap;
	KeyCode nlock, slock;
	static int mask_table[8] = { ShiftMask, LockMask, ControlMask, Mod1Mask,
			Mod2Mask, Mod3Mask, Mod4Mask, Mod5Mask };

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
			static jclass cls = env->FindClass("com/jxgrabkey/JXGrabKey");
			static jmethodID mid = env->GetStaticMethodID(cls, "debugCallback",
					"(Ljava/lang/String;)V");

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
		sout << "xErrorHandler() - Caught error: serial = " << std::dec
				<< _event->serial;
		sout << "; resourceid = " << std::dec << _event->resourceid;
		sout << "; type = " << std::dec << _event->type;
		sout << "; error_code = " << std::dec << (int) _event->error_code;
		sout << "; request_code = " << std::dec << (int) _event->request_code;
		sout << "; minor_code = " << std::dec << (int) _event->minor_code;
		printToDebugCallback(NULL, sout.str().c_str());
	}

	return NULL;
}

void grabKey(JNIEnv *_env, KeyStruct key) {
  Mask modifierCombinations[] = { key.mask, key.mask | numlock_mask, key.mask
    | scrolllock_mask, key.mask | capslock_mask, key.mask
      | numlock_mask | scrolllock_mask, key.mask | numlock_mask
      | capslock_mask, key.mask | scrolllock_mask | capslock_mask,
      key.mask | numlock_mask | scrolllock_mask | capslock_mask };

  for (int i = 0; i < dps.size(); ++i) {
    Display* _dpy = dps[i];
    for (int screen = 0; screen < ScreenCount(_dpy); screen++) {
      for (int m = 0; m < 8; m++) {
        int ret =
          XGrabKey(_dpy, key.key, modifierCombinations[m],
              RootWindow(_dpy, screen), True, GrabModeAsync,
              GrabModeAsync);
        if (debug && !ret) {
          ostringstream sout;
          sout << "grabKey() - WARNING: XGrabKey() on screen "
            << std::dec << screen << " for mask combination "
            << std::dec << m << " returned false";
          printToDebugCallback(_env, sout.str().c_str());
        }
      }
    }
  }
}

void ungrabKey(JNIEnv *_env, KeyStruct key) {
  Mask modifierCombinations[] = { key.mask, key.mask | numlock_mask, key.mask
    | scrolllock_mask, key.mask | capslock_mask, key.mask
      | numlock_mask | scrolllock_mask, key.mask | numlock_mask
      | capslock_mask, key.mask | scrolllock_mask | capslock_mask,
      key.mask | numlock_mask | scrolllock_mask | capslock_mask };

  for (int i = 0; i < dps.size(); ++i) {
    Display* _dpy = dps[i];
    for (int screen = 0; screen < ScreenCount(_dpy); screen++) {
      for (int m = 0; m < 8; m++) {
        int ret = XUngrabKey(_dpy, key.key, modifierCombinations[m],
            RootWindow(_dpy, screen));
        if (debug && !ret) {
          ostringstream sout;
          sout << "ungrabKey() - WARNING: XUngrabKey() on screen "
            << std::dec << screen << " for mask combination "
            << std::dec << m << " returned false";
          printToDebugCallback(_env, sout.str().c_str());
        }
      }
    }
  }
}

jobject getPoint(JNIEnv* _env, Display* _dpy) {
  jclass pointClass = _env->FindClass("com/jxgrabkey/JXEvent");
  jmethodID cons = _env->GetMethodID(pointClass, "<init>", "(Ljava/lang/String;III)V");
  Window window_returned;
  int root_x, root_y;
  int win_x, win_y;
  unsigned int mask_return;
  for (int i = 0; i < ScreenCount(_dpy); i++) {
    Bool result = XQueryPointer(_dpy, RootWindow(_dpy, i), &window_returned,
        &window_returned, &root_x, &root_y, &win_x, &win_y,
        &mask_return);
    if (result == True) {
      return _env->NewObject(pointClass, cons, _env->NewStringUTF(DisplayString(_dpy)), i, win_x, win_y);
    }
  }
  return NULL;
}
