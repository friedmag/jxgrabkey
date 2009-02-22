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

#include <jni.h>
#include <X11/Xlib.h>

#ifdef __cplusplus
extern "C" {
#endif

struct KeyStruct {
    int id;
    KeyCode key;
    Mask mask;
};

void getOffendingModifiers(Display* _dpy);
static int *xErrorHandler(Display *_dpy, XErrorEvent *_event);
void printToDebugCallback(JNIEnv *_env, const char* message);
void grabKey(JNIEnv *_env, KeyStruct key);
void ungrabKey(JNIEnv *_env, KeyStruct key);

//The following function headers have been generated

/*
 * Class:     jxgrabkey_JXGrabKey
 * Method:    clean
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_jxgrabkey_JXGrabKey_clean(JNIEnv *, jobject);

/*
 * Class:     jxgrabkey_JXGrabKey
 * Method:    registerHotkey
 * Signature: (III)V
 */
JNIEXPORT void JNICALL Java_jxgrabkey_JXGrabKey_registerHotkey__III(JNIEnv *, jobject, jint, jint, jint);

/*
 * Class:     jxgrabkey_JXGrabKey
 * Method:    unregisterHotKey
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_jxgrabkey_JXGrabKey_unregisterHotKey(JNIEnv *, jobject, jint);

/*
 * Class:     jxgrabkey_JXGrabKey
 * Method:    listen
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_jxgrabkey_JXGrabKey_listen(JNIEnv *, jobject);

/*
 * Class:     jxgrabkey_JXGrabKey
 * Method:    setDebug
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_jxgrabkey_JXGrabKey_setDebug(JNIEnv *, jobject, jboolean);

#ifdef __cplusplus
}
#endif