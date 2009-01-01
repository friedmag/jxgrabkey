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

package jxgrabkey;

import java.awt.event.InputEvent;
import java.awt.event.KeyEvent;
import java.util.ArrayList;

public class JXGrabKey {

    private static boolean debug = false;

    public static final int X11_SHIFT_MASK = 1 << 0;
    public static final int X11_LOCK_MASK = 1 << 1;
    public static final int X11_CONTROL_MASK = 1 << 2;
    public static final int X11_MOD1_MASK = 1 << 3;
    public static final int X11_MOD2_MASK = 1 << 4;
    public static final int X11_MOD3_MASK = 1 << 5;
    public static final int X11_MOD4_MASK = 1 << 6;
    public static final int X11_MOD5_MASK = 1 << 7;

    private static JXGrabKey instance;
    private static ArrayList<HotkeyListener> listeners;

    private JXGrabKey() {
        listeners = new ArrayList<HotkeyListener>();
        new Thread(){
            @Override
            public void run() {
                listen();
                throw new IllegalStateException("JXGrabKey library stopped with listen()!");
            }
        }.start();
        try{
            Thread.sleep(100);
        }catch(InterruptedException e){}
    }

    public static JXGrabKey getInstance(){
        if(instance == null){
            instance = new JXGrabKey();
        }
        return instance;
    }

    public void addHotkeyListener(HotkeyListener listener){
        JXGrabKey.listeners.add(listener);
    }

    public void removeHotkeyListener(HotkeyListener listener){
        JXGrabKey.listeners.remove(listener);
    }

    public void cleanUp(){
        clean();
        listeners.clear();
    }

    private native void clean();

    public native void registerHotkey(int id, int mask, int key);

    public void registerAwtHotkey(int id, int mask, int key){

        debugCallback("++ registerAWTHotkey()");

        int x11Mask = 0;

        if ((mask & InputEvent.SHIFT_MASK) != 0) {
            x11Mask |= X11_SHIFT_MASK;
        }
        if ((mask & InputEvent.ALT_MASK) != 0) {
            x11Mask |= X11_MOD1_MASK;
        }
        if ((mask & InputEvent.CTRL_MASK) != 0) {
            x11Mask |= X11_CONTROL_MASK;
        }
        if ((mask & InputEvent.META_MASK) != 0) {
            x11Mask |= X11_MOD2_MASK;
        }
        if ((mask & InputEvent.ALT_GRAPH_MASK) != 0) {
            x11Mask |= X11_MOD5_MASK;
        }

        int keysym = X11KeysymDefinitions.AwtToX11Keysym(key);

        debugCallback("registerAWTHotkey() - converted AWTKeycode '"+KeyEvent.getKeyText(key)+"' (0x"+Integer.toHexString(key)+") to x11Keysym 0x"+Integer.toHexString(keysym));

        debugCallback("registerAWTHotkey() - converted AWTMask '"+KeyEvent.getKeyModifiersText(mask)+"' (0x"+Integer.toHexString(mask)+") to x11Mask 0x"+Integer.toHexString(x11Mask));

        registerHotkey(id, x11Mask, keysym);

        debugCallback("-- registerAWTHotkey()");
    }

    public native void unregisterHotKey(int id);

    public native void listen();

    private static native void setDebug(boolean debug);

    public static void setDebugOutput(boolean enabled){
        debug = enabled;
        setDebug(enabled);
    }

    public static void fireKeyEvent(int id){
        for(int i = 0; i < listeners.size(); i++){
            if(listeners.get(i) != null){
                listeners.get(i).onHotkey(id);
            }else{
                listeners.remove(i);
            }
        }
    }

    public static void debugCallback(String debugmessage){
        if(debug){
            boolean found = false;
            for(HotkeyListener l : listeners){
                if(l instanceof HotkeyListenerDebugEnabled){
                    ((HotkeyListenerDebugEnabled)l).debugCallback(debugmessage);
                    found = true;
                }
            }

            if(found == false){
                System.out.println(debugmessage);
            }
        }
    }

}
