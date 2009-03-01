/*  Copyright 2008  Edwin Stang (edwinstang@gmail.com),
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

import java.awt.event.KeyEvent;
import java.util.Vector;

/**
 * This class implements the API access.
 * All public methods are synchronized, hence thread-safe.
 *
 * @author subes
 */
public class JXGrabKey {

    private static final int SLEEP_WHILE_LISTEN_EXITS = 100;

    private static boolean debug;

    private static JXGrabKey instance;
    private static Thread thread;
    private static Vector<HotkeyListener> listeners = new Vector<HotkeyListener>();

    /**
     * This constructor starts a seperate Thread for the main listen loop.
     */
    private JXGrabKey() {
        thread = new Thread(){
            @Override
            public void run() {
                listen();
                debugCallback("-- listen()");
            }
        };
        thread.start();
    }

    /**
     * Retrieves the singleton. Initializes it, if not yet done.
     *
     * @return
     */
    public static synchronized JXGrabKey getInstance(){
        if(instance == null){
            instance = new JXGrabKey();
        }
        return instance;
    }

    /**
     * Adds a HotkeyListener.
     *
     * @param listener
     */
    public synchronized void addHotkeyListener(HotkeyListener listener){
        if(listener == null){
            throw new IllegalArgumentException("listener must not be null");
        }
        JXGrabKey.listeners.add(listener);
    }

    /**
     * Removes a HotkeyListener.
     *
     * @param listener
     */
    public synchronized void removeHotkeyListener(HotkeyListener listener){
        if(listener == null){
            throw new IllegalArgumentException("listener must not be null");
        }
        JXGrabKey.listeners.remove(listener);
    }

    /**
     * Unregisters all hotkeys, removes all HotkeyListeners,
     * stops the main listen loop and deinitializes the singleton.
     */
    public synchronized void cleanUp(){
        clean();
        if(thread.isAlive()){
            while(thread.isAlive()){
                try {
                    Thread.sleep(SLEEP_WHILE_LISTEN_EXITS);
                } catch (InterruptedException e) {
                    debugCallback("cleanUp() - InterruptedException: "+e.getMessage());
                }
            }
            instance = null; //next time getInstance is called, reinitialize JXGrabKey
        }
        if(listeners.size() > 0){
            listeners.clear();
        }
    }

    /**
     * Registers a X11 hotkey.
     *
     * @param id
     * @param x11Mask
     * @param x11Keysym
     * @throws jxgrabkey.HotkeyConflictException
     */
    public synchronized void registerX11Hotkey(int id, int x11Mask, int x11Keysym) throws HotkeyConflictException{
        registerHotkey(id, x11Mask, x11Keysym);
    }

    /**
     * Converts an AWT hotkey into a X11 hotkey and registers it.
     *
     * @param id
     * @param awtMask
     * @param awtKey
     * @throws jxgrabkey.HotkeyConflictException
     */
    public synchronized void registerAwtHotkey(int id, int awtMask, int awtKey) throws HotkeyConflictException{
        debugCallback("++ registerAwtHotkey("+id+", 0x"+
                Integer.toHexString(awtMask)+", 0x"+
                Integer.toHexString(awtKey)+")");

        int x11Mask = X11MaskDefinitions.awtToX11Mask(awtMask);
        int x11Keysym = X11KeysymDefinitions.awtToX11Keysym(awtKey);

        debugCallback("registerAwtHotkey() - converted AWT mask '"+
                KeyEvent.getKeyModifiersText(awtMask)+"' (0x"+Integer.toHexString(awtMask)+
                ") to X11 mask (0x"+Integer.toHexString(x11Mask)+")");

        debugCallback("registerAwtHotkey() - converted AWT key '"+
                KeyEvent.getKeyText(awtKey)+"' (0x"+Integer.toHexString(awtKey)+
                ") to X11 keysym (0x"+Integer.toHexString(x11Keysym)+")");

        registerHotkey(id, x11Mask, x11Keysym);

        debugCallback("-- registerAwtHotkey()");
    }

    /**
     * Enables/Disables printing of debug messages.
     *
     * @param enabled
     */
    public synchronized static void setDebugOutput(boolean enabled){
        debug = enabled;
        setDebug(enabled);
    }

    /**
     * Notifies HotkeyListeners about a received KeyEvent.
     *
     * This method is used by the C++ code.
     * Do not use this method from externally.
     *
     * @param id
     */
    public synchronized static void fireKeyEvent(int id){
        for(int i = 0; i < listeners.size(); i++){
            listeners.get(i).onHotkey(id);
        }
    }

    /**
     * Either gives debug messages to a HotkeyListenerDebugEnabled if registered,
     * or prints to console otherwise.
     * Does only print if debug is enabled.
     *
     * This method is both used by the C++ and Java code, so it should not be synchronized.
     * Don't use this method from externally.
     *
     * @param debugmessage
     */
    public static void debugCallback(String debugmessage){
        if(debug){
            debugmessage.trim();
            if(debugmessage.charAt(debugmessage.length()-1) != '\n'){
                debugmessage += "\n";
            }else{
                while(debugmessage.endsWith("\n\n")){
                    debugmessage = debugmessage.substring(0, debugmessage.length()-1);
                }
            }

            boolean found = false;
            for(HotkeyListener l : listeners){
                if(l instanceof HotkeyListenerDebugEnabled){
                    ((HotkeyListenerDebugEnabled)l).debugCallback(debugmessage);
                    found = true;
                }
            }

            if(found == false){
                System.out.print(debugmessage);
            }
        }
    }

    /**
     * This method unregisters a hotkey.
     * If the hotkey is not yet registered, nothing will happen.
     *
     * @param id
     */
    public synchronized native void unregisterHotKey(int id);

    private native void listen();

    private static native void setDebug(boolean debug);

    private native void clean();

    private native void registerHotkey(int id, int mask, int key) throws HotkeyConflictException;

}
