package sinc2.util;

import org.junit.jupiter.api.Test;

import java.util.Collection;

import static org.junit.jupiter.api.Assertions.*;

class ArrayOperationTest {
    @Test
    void testToCollection() {
        int[] arr = {1, 1, 3, 5, 2, 4, 8, 9, 0, 4, 2, 2, 5, 3, 5};
        Collection<Integer> collection = ArrayOperation.toCollection(arr);
        int idx = 0;
        for (Integer i: collection) {
            assertEquals(i, arr[idx]);
            idx++;
        }
    }
}