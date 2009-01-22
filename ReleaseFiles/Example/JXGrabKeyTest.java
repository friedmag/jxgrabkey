import java.awt.event.KeyEvent;
import java.io.File;

import jxgrabkey.HotkeyListener;
import jxgrabkey.JXGrabKey;

public class JXGrabKeyTest {

	private static final int MY_HOTKEY_INDEX = 1;
	private static boolean hotkeyEventReceived = false;
	
	public static void main(String[] args) throws Exception {
		//Load JXGrabKey lib
		System.load(new File("lib/libJXGrabKey.so").getCanonicalPath());
		
		//Enable Debug Output
		JXGrabKey.setDebugOutput(true);
		
		//Register some Hotkey (CTRL+ALT+SHIFT+K)
		JXGrabKey.getInstance().registerAWTHotkey(MY_HOTKEY_INDEX,
				KeyEvent.CTRL_MASK | KeyEvent.ALT_MASK | KeyEvent.SHIFT_MASK,
				KeyEvent.VK_K);
				
		//Implement HotkeyListener
		HotkeyListener hotkeyListener = new jxgrabkey.HotkeyListener(){
			public void onHotkey(int hotkey_idx) {
				if (hotkey_idx != MY_HOTKEY_INDEX)
					return;
				hotkeyEventReceived = true;
			}
        };
        
        //Add HotkeyListener
		JXGrabKey.getInstance().addHotkeyListener(hotkeyListener);
		
		//Wait for Hotkey Event
		while(!hotkeyEventReceived){
			Thread.sleep(1000);
		}
		
		// Shutdown JXGrabKey
		JXGrabKey.getInstance().unregisterHotKey(MY_HOTKEY_INDEX);
		JXGrabKey.getInstance().removeHotkeyListener(hotkeyListener);
		JXGrabKey.getInstance().cleanUp();
	}
}
