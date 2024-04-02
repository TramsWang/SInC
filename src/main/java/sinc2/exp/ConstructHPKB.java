package sinc2.exp;

import sinc2.kb.KbException;
import sinc2.util.kb.NumeratedKb;

import java.io.IOException;

public class ConstructHPKB {
    static final String ALBUS = "albus";
    static final String DRACO = "draco";
    static final String FEMALE = "female";
    static final String FRANK = "frank";
    static final String GINNY = "ginny";
    static final String HARRY = "harry";
    static final String HERMIONE = "hermione";
    static final String HUGO = "hugo";
    static final String JAMES = "james";
    static final String LILY = "lily";
    static final String LUCIUS = "lucius";
    static final String LUNA = "luna";
    static final String MALE = "male";
    static final String NEVILLE = "neville";
    static final String RON = "ron";
    static final String ROSE = "rose";
    static final String RUBEUS = "rubeus";
    static final String SIRIUS = "sirius";

    static final String FAMILY = "family";
    static final String MOTHER = "mother";
    static final String FATHER = "father";
    static final String GRANDPARENT = "grandparent";
    static final String PARENT = "parent";
    static final String GENDER = "gender";
    static final String FRIEND = "friend";
    static final String PERSON = "person";

    public static void main(String[] args) throws KbException, IOException {
        NumeratedKb kb = new NumeratedKb("HPKB");

        kb.addRecords(FAMILY, new String[][]{
                new String[] {LILY, JAMES, HARRY},
                new String[] {GINNY, HARRY, ALBUS},
                new String[] {GINNY, HARRY, SIRIUS},
                new String[] {HERMIONE, RON, HUGO},
                new String[] {HERMIONE, RON, ROSE},
        });
        kb.addRecords(MOTHER, new String[][]{
                new String[] {LILY, HARRY},
                new String[] {GINNY, ALBUS},
                new String[] {GINNY, SIRIUS},
                new String[] {HERMIONE, HUGO},
                new String[] {HERMIONE, ROSE},
        });
        kb.addRecords(FATHER, new String[][]{
                new String[] {JAMES, HARRY},
                new String[] {HARRY, SIRIUS},
                new String[] {RON, HUGO},
                new String[] {LUCIUS, DRACO},
                new String[] {FRANK, NEVILLE},
        });
        kb.addRecords(GRANDPARENT, new String[][]{
                new String[] {LILY, ALBUS},
                new String[] {LILY, SIRIUS},
                new String[] {JAMES, ALBUS},
                new String[] {JAMES, SIRIUS},
        });
        kb.addRecords(PARENT, new String[][]{
                new String[] {LILY, HARRY},
                new String[] {GINNY, ALBUS},
                new String[] {GINNY, SIRIUS},
                new String[] {HERMIONE, HUGO},
                new String[] {HERMIONE, ROSE},
                new String[] {JAMES, HARRY},
                new String[] {HARRY, SIRIUS},
                new String[] {HARRY, ALBUS},
                new String[] {RON, HUGO},
                new String[] {FRANK, NEVILLE},
        });
        kb.addRecords(GENDER, new String[][]{
                new String[] {JAMES, MALE},
                new String[] {HARRY, MALE},
                new String[] {RON, MALE},
                new String[] {LUCIUS, MALE},
                new String[] {FRANK, MALE},
                new String[] {LILY, FEMALE},
                new String[] {HERMIONE, FEMALE},
                new String[] {GINNY, FEMALE},
                new String[] {LUNA, FEMALE},
                new String[] {ROSE, FEMALE},
        });
        kb.addRecords(FRIEND, new String[][]{
                new String[] {HARRY, RON},
                new String[] {HARRY, NEVILLE},
                new String[] {HERMIONE, NEVILLE},
                new String[] {NEVILLE, GINNY},
                new String[] {RON, HARRY},
                new String[] {NEVILLE, HARRY},
                new String[] {NEVILLE, HERMIONE},
                new String[] {GINNY, NEVILLE}
        });
        kb.addRecords(PERSON, new String[][]{
                new String[] {LILY},
                new String[] {JAMES},
                new String[] {HARRY},
                new String[] {GINNY},
                new String[] {ALBUS},
                new String[] {SIRIUS},
                new String[] {HERMIONE},
                new String[] {RON},
                new String[] {HUGO},
                new String[] {ROSE},
                new String[] {LUCIUS},
                new String[] {FRANK},
                new String[] {RUBEUS},
        });

        kb.dump(".");
    }
}
