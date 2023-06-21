package sinc2.kb;

import sinc2.impl.negsamp.CacheFragment;

public class AdversarialSampledResult {
    public final IntTable negTable;
    public final CacheFragment negCache;

    public AdversarialSampledResult(IntTable negTable, CacheFragment negCache) {
        this.negTable = negTable;
        this.negCache = negCache;
    }
}
