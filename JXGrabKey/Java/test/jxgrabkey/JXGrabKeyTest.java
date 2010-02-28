
package jxgrabkey;

import java.awt.event.KeyEvent;
import java.io.File;
import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;

/**
 *
 * @author subes
 */
public class JXGrabKeyTest {

    @BeforeClass
    public static void setUpClass() throws Exception {
        System.load(new File("../C++/dist/Release/GNU-Linux-x86/libJXGrabKey.so").getCanonicalPath());
        JXGrabKey.setDebugOutput(true);
        JXGrabKey.getInstance();
    }

    @AfterClass
    public static void tearDownClass() throws Exception {
        JXGrabKey.getInstance().cleanUp();
    }

    @Test
    public void testRegisterAwtHotkey() throws HotkeyConflictException{
        int id = 0;
        int awtMask = KeyEvent.ALT_DOWN_MASK;
        int awtKey = KeyEvent.VK_F;
        JXGrabKey.getInstance().registerAwtHotkey(id, awtMask, awtKey);
    }

}