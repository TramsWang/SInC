package sinc2.impl.base;

import sinc2.kb.IntTable;

/**
 * The Complied Block (CB) structure. Every member is read only, as operations on the cache should follow "copy-on-write"
 * strategy.
 *
 * @since 2.0
 */
public class CompliedBlock {
    /** Partially Assigned Record (PAR) */  //Todo: This information is redundant
    public final int[] partAsgnRecord;
    /** Compliance Set (CS) */
    public final int[][] complSet;
    /** The IntTable here serves as the indices of each argument */
    public IntTable indices;

    public CompliedBlock(int[] partAsgnRecord, int[][] complSet) {
        this.partAsgnRecord = partAsgnRecord;
        this.complSet = complSet;
        this.indices = null;
    }

    public CompliedBlock(int[] partAsgnRecord, int[][] complSet, IntTable indices) {
        this.partAsgnRecord = partAsgnRecord;
        this.complSet = complSet;
        this.indices = indices;
    }

    /**
     * Build the indices if it is null
     */
    public void buildIndices() {
        if (null == indices) {
            indices = new IntTable(complSet);
        }
    }
}
