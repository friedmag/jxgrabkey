package jxgrabkey;

public class HotkeyConflictException extends Exception {

    public HotkeyConflictException() {
        super();
    }

    public HotkeyConflictException(String message) {
        super(message);
    }

    public HotkeyConflictException(String message, Throwable cause) {
        super(message, cause);
    }

    public HotkeyConflictException(Throwable cause) {
        super(cause);
    }
}
