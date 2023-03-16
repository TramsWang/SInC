package sinc2.common;

/**
 * Debug level, indicating "conditional compilation". The higher the level is, the more debugging information will be
 * recorded. Higher level information contains all lower level info.
 *
 * @since 2.0
 */
public final class DebugLevel {
    /** No debug information is used */
    public static final int NONE = 0;
    /** Show verbose output */
    public static final int VERBOSE = 1;
    /** Show debug output */
    public static final int DEBUG = 2;
    /** Change this static variable to alter the compilation level */
    public static final int LEVEL = NONE;
}
