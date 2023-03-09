package sinc2.kb;

/**
 * This class is used to denote which records are already entailed in the relation and which are not
 *
 * @since 2.1
 */
public class SplitRecords {
    public final int[][] entailedRecords;
    public final int[][] nonEntailedRecords;

    public SplitRecords(int[][] entailedRecords, int[][] nonEntailedRecords) {
        this.entailedRecords = entailedRecords;
        this.nonEntailedRecords = nonEntailedRecords;
    }
}
