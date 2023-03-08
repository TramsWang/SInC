package sinc2.util;

import java.util.Collection;
import java.util.Iterator;
import java.util.List;
import java.util.Set;

/**
 * Helper functions for array operations.
 *
 * @since 2.0
 */
public class ArrayOperation {

    /**
     * Convert an "Integer" list to an "int" array.
     */
    public static int[] toArray(List<Integer> list){
        int[] array = new int[list.size()];
        for (int i = 0; i < array.length; i++) {
            array[i] = list.get(i);
        }
        return array;
    }

    /**
     * Convert an "Integer" set to an "int" array.
     */
    public static int[] toArray(Set<Integer> set) {
        int[] array = new int[set.size()];
        int idx = 0;
        for (int i: set) {
            array[idx] = i;
            idx++;
        }
        return array;
    }

    /**
     * Initialize an array with initial value.
     */
    public static int[] initArrayWithValue(int length, int initValue) {
        int[] arr = new int[length];
        for (int i = 0; i < length; i++) {
            arr[i] = initValue;
        }
        return arr;
    }

    public static Collection<Integer> toCollection(int[] arr) {
        class AbstractCollection implements Collection<Integer> {
            final protected int[] arr;

            public AbstractCollection(int[] arr) {
                this.arr = arr;
            }

            @Override
            public int size() {
                return arr.length;
            }

            @Override
            public boolean isEmpty() {
                return 0 == arr.length;
            }

            @Override
            public boolean contains(Object o) {
                if (o instanceof Integer) {
                    boolean contains = false;
                    int oo = (Integer) o;
                    for (int i : arr) {
                        if (i == oo) {
                            return true;
                        }
                    }
                }
                return false;
            }

            @Override
            public Iterator<Integer> iterator() {
                class AbstractIterator implements Iterator<Integer> {
                    int idx = 0;
                    final int[] arr;

                    public AbstractIterator(int[] arr) {
                        this.arr = arr;
                    }

                    @Override
                    public boolean hasNext() {
                        return idx < arr.length;
                    }

                    @Override
                    public Integer next() {
                        int i = arr[idx];
                        idx++;
                        return i;
                    }
                }
                return new AbstractIterator(arr);
            }

            @Override
            public Integer[] toArray() {
                throw new Error("Method not supported");
            }

            @Override
            public <T> T[] toArray(T[] ts) {
                throw new Error("Method not supported");
            }

            @Override
            public boolean add(Integer integer) {
                throw new Error("Method not supported");
            }

            @Override
            public boolean remove(Object o) {
                throw new Error("Method not supported");
            }

            @Override
            public boolean containsAll(Collection<?> collection) {
                throw new Error("Method not supported");
            }

            @Override
            public boolean addAll(Collection<? extends Integer> collection) {
                throw new Error("Method not supported");
            }

            @Override
            public boolean removeAll(Collection<?> collection) {
                throw new Error("Method not supported");
            }

            @Override
            public boolean retainAll(Collection<?> collection) {
                throw new Error("Method not supported");
            }

            @Override
            public void clear() {
                throw new Error("Method not supported");
            }
        }
        return new AbstractCollection(arr);
    }
}
