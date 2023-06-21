package sinc2.kb;

/**
 * This class is used to label the position of an argument in a list of fragmented cache, a variable, or a constant.
 *
 * @since 2.3
 */
public class CacheFragArgLoc {
    /* Type flags */
    public static final byte TYPE_VAR = 0;
    public static final byte TYPE_CONST = 1;
    public static final byte TYPE_LOC = 2;

    /* Members */
    public final byte type;
    public int vid;
    public final int constant;
    public final int fragIdx;
    public final int tabIdx;
    public final int colIdx;

    public static CacheFragArgLoc createVid(int vid) {
        return new CacheFragArgLoc(TYPE_VAR, vid, 0, 0, 0, 0);
    }

    public static CacheFragArgLoc createConstant(int constant) {
        return new CacheFragArgLoc(TYPE_CONST, 0, constant, 0, 0, 0);
    }

    public static CacheFragArgLoc createLoc(int fragIdx, int tabIdx, int colIdx) {
        return new CacheFragArgLoc(TYPE_LOC, 0, 0, fragIdx, tabIdx, colIdx);
    }

    protected CacheFragArgLoc(byte type, int vid, int constant, int fragIdx, int tabIdx, int colIdx) {
        this.type = type;
        this.vid = vid;
        this.constant = constant;
        this.fragIdx = fragIdx;
        this.tabIdx = tabIdx;
        this.colIdx = colIdx;
    }

    public byte getType() {
        return type;
    }
}
