package sinc2.impl.est;

import java.util.Objects;

public class VarPair {
    final int vid1;
    final int vid2;

    public VarPair(int vid1, int vid2) {
        this.vid1 = vid1;
        this.vid2 = vid2;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        VarPair varPair = (VarPair) o;
        return vid1 == varPair.vid1 && vid2 == varPair.vid2;
    }

    @Override
    public int hashCode() {
        return Objects.hash(vid1, vid2);
    }
}
