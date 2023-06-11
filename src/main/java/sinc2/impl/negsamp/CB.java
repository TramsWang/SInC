package sinc2.impl.negsamp;

import sinc2.kb.IntTable;

/**
 * A simplified complied block.
 */
public class CB {
    /** Compliance Set (CS) */
    public final int[][] complianceSet;
    /** The IntTable here serves as the indices of each argument */
    public IntTable indices;

    public CB(int[][] complianceSet) {
        this.complianceSet = complianceSet;
        this.indices = null;
    }

    public CB(int[][] complianceSet, IntTable indices) {
        this.complianceSet = complianceSet;
        this.indices = indices;
    }

    /**
     * Build the indices if it is null
     */
    public void buildIndices() {
        if (null == indices) {
            indices = new IntTable(complianceSet);
        }
    }
}
