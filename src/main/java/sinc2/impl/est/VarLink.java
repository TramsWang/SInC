package sinc2.impl.est;

import java.util.Objects;

/**
 * This class is to denote the correspondence of two LVs in a certain predicate. E.g., the variables X and Y in the 2nd
 * predicate of the following rule:
 *
 *   p(X, Y) :- q(X, Z), r(Z, Y)
 *
 * Such links compose link paths between LVs, such as "X->Z->Y" in the above rule.
 *
 * @since 2.1
 */
public class VarLink {
    public final int predIdx;
    public final int fromVid;
    public final int fromArgIdx;
    public final int toVid;
    public final int toArgIdx;

    public VarLink(int predIdx, int fromVid, int fromArgIdx, int toVid, int toArgIdx) {
        this.predIdx = predIdx;
        this.fromVid = fromVid;
        this.fromArgIdx = fromArgIdx;
        this.toVid = toVid;
        this.toArgIdx = toArgIdx;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        VarLink varLink = (VarLink) o;
        return fromVid == varLink.fromVid && toVid == varLink.toVid;
    }

    @Override
    public int hashCode() {
        return Objects.hash(fromVid, toVid);
    }
}
