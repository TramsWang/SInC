package sinc2.impl.est;

import java.util.Objects;

/**
 * This node structure is to represent how a predicate in the rule is linked to the head
 *
 * @since 2.1
 */
public class LinkTreeNode {
    /** The index of the predecessor predicate that this predicate is linked to */
    public int predNodeIdx;
    /** The index of the argument in the predecessor that the link is established */
    public int predKeyArgIdx;
    /** The index of the argument in this predicate that the link is established */
    public int keyColIdx;
    /** The level/depth of this predicate in a link tree (where root is 0) */
    public int level = 0;
    public double[] factorKey2Others;
    public double[] factorOthers2Key;

    public LinkTreeNode(int arity, int keyColIdx, int predNodeIdx, int predKeyArgIdx) {
        this.predNodeIdx = predNodeIdx;
        this.predKeyArgIdx = predKeyArgIdx;
        this.keyColIdx = keyColIdx;
        this.factorKey2Others = new double[arity];
        this.factorOthers2Key = new double[arity];
    }

    public LinkTreeNode(LinkTreeNode another) {
        this.predNodeIdx = another.predNodeIdx;
        this.predKeyArgIdx = another.predKeyArgIdx;
        this.keyColIdx = another.keyColIdx;
        this.level = another.level;
        this.factorKey2Others = new double[another.factorKey2Others.length];
        this.factorOthers2Key = new double[another.factorOthers2Key.length];
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        LinkTreeNode that = (LinkTreeNode) o;
        return predNodeIdx == that.predNodeIdx && predKeyArgIdx == that.predKeyArgIdx && keyColIdx == that.keyColIdx && level == that.level;
    }

    @Override
    public int hashCode() {
        return Objects.hash(predNodeIdx, predKeyArgIdx, keyColIdx, level);
    }
}
