package sinc2.impl.est;

import java.util.Objects;

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
