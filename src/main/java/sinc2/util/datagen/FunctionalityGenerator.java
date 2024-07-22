package sinc2.util.datagen;

import sinc2.kb.KbException;
import sinc2.kb.SimpleKb;
import sinc2.util.kb.NumeratedKb;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public class FunctionalityGenerator {

    static final String PRED_FATHER = "father";
    static final String PRED_MOTHER = "mother";
    static final String PRED_PARENT = "parent";
    static final String PRED_GENDER = "gender";
    static final String PRED_BROTHER = "brother";
    static final String PRED_SISTER = "sister";
    static final String PRED_SIBLING = "sibling";

    static final String GENDER_FEMALE = "female";
    static final String GENDER_MALE = "male";

    static final String MEMBER_FATHER = "f";
    static final String MEMBER_MOTHER = "m";
    static final String MEMBER_SON = "s";
    static final String MEMBER_DAUGHTER = "d";

    static class Triple {
        String subj;
        String pred;
        String obj;

        public Triple(String subj, String pred, String obj) {
            this.subj = subj;
            this.pred = pred;
            this.obj = obj;
        }
    }

    public static void main(String[] args) throws KbException, IOException {
        for (int functionality = 1; functionality <= 10; functionality++) {
            generateVariousFunctionality(".", "FixFunc_" + functionality, 20, functionality);
            generateVariousFunctionalityDeviation(".", "DevFunc_" + functionality, 20, functionality);
        }
    }

    public static void generateVariousFunctionality(
            String dumpPath, String kbName, int families, int functionality
    ) throws KbException, IOException {
        NumeratedKb kb = new NumeratedKb(kbName);
        for (int f = 0; f < families; f++) {
            /* father/mother/parent */
            for (int i = 0; i < functionality; i++) {
                kb.addRecord(PRED_FATHER, new String[]{MEMBER_FATHER + '_' + f, MEMBER_SON + i + '_' + f});
                kb.addRecord(PRED_FATHER, new String[]{MEMBER_FATHER + '_' + f, MEMBER_DAUGHTER + i + '_' + f});
                kb.addRecord(PRED_MOTHER, new String[]{MEMBER_MOTHER + '_' + f, MEMBER_SON + i + '_' + f});
                kb.addRecord(PRED_MOTHER, new String[]{MEMBER_MOTHER + '_' + f, MEMBER_DAUGHTER + i + '_' + f});
                kb.addRecord(PRED_PARENT, new String[]{MEMBER_FATHER + '_' + f, MEMBER_SON + i + '_' + f});
                kb.addRecord(PRED_PARENT, new String[]{MEMBER_FATHER + '_' + f, MEMBER_DAUGHTER + i + '_' + f});
                kb.addRecord(PRED_PARENT, new String[]{MEMBER_MOTHER + '_' + f, MEMBER_SON + i + '_' + f});
                kb.addRecord(PRED_PARENT, new String[]{MEMBER_MOTHER + '_' + f, MEMBER_DAUGHTER + i + '_' + f});
            }

            /* brother/sister/sibling */
            for (int i = 0; i < functionality; i++) {
                for (int j = 0; j < functionality; j++) {
                    if (i != j) {
                        kb.addRecord(PRED_BROTHER, new String[]{MEMBER_SON + i + '_' + f, MEMBER_SON + j + '_' + f});
                        kb.addRecord(PRED_SISTER, new String[]{MEMBER_DAUGHTER + i + '_' + f, MEMBER_DAUGHTER + j + '_' + f});
                        kb.addRecord(PRED_SIBLING, new String[]{MEMBER_SON + i + '_' + f, MEMBER_SON + j + '_' + f});
                        kb.addRecord(PRED_SIBLING, new String[]{MEMBER_DAUGHTER + i + '_' + f, MEMBER_DAUGHTER + j + '_' + f});
                    }
                    kb.addRecord(PRED_BROTHER, new String[]{MEMBER_SON + i + '_' + f, MEMBER_DAUGHTER + j + '_' + f});
                    kb.addRecord(PRED_SISTER, new String[]{MEMBER_DAUGHTER + i + '_' + f, MEMBER_SON + j + '_' + f});
                    kb.addRecord(PRED_SIBLING, new String[]{MEMBER_SON + i + '_' + f, MEMBER_DAUGHTER + j + '_' + f});
                    kb.addRecord(PRED_SIBLING, new String[]{MEMBER_DAUGHTER + i + '_' + f, MEMBER_SON + j + '_' + f});
                }
            }

            /* gender */
            kb.addRecord(PRED_GENDER, new String[]{MEMBER_FATHER + '_' + f, GENDER_MALE});
            kb.addRecord(PRED_GENDER, new String[]{MEMBER_MOTHER + '_' + f, GENDER_FEMALE});
            for (int i = 0; i < functionality; i++) {
                kb.addRecord(PRED_GENDER, new String[]{MEMBER_SON + i + '_' + f, GENDER_MALE});
                kb.addRecord(PRED_GENDER, new String[]{MEMBER_DAUGHTER + i + '_' + f, GENDER_FEMALE});
            }
        }

        kb.dump(dumpPath);
    }

    public static void generateVariousFunctionalityDeviation(
            String dumpPath, String kbName, int families, int maxFunctionality
    ) throws KbException, IOException {
        NumeratedKb kb = new NumeratedKb(kbName);
        for (int f = 0; f < families; f++) {
            int functionality = f % maxFunctionality + 1;

            /* father/mother/parent */
            for (int i = 0; i < functionality; i++) {
                kb.addRecord(PRED_FATHER, new String[]{MEMBER_FATHER + '_' + f, MEMBER_SON + i + '_' + f});
                kb.addRecord(PRED_FATHER, new String[]{MEMBER_FATHER + '_' + f, MEMBER_DAUGHTER + i + '_' + f});
                kb.addRecord(PRED_MOTHER, new String[]{MEMBER_MOTHER + '_' + f, MEMBER_SON + i + '_' + f});
                kb.addRecord(PRED_MOTHER, new String[]{MEMBER_MOTHER + '_' + f, MEMBER_DAUGHTER + i + '_' + f});
                kb.addRecord(PRED_PARENT, new String[]{MEMBER_FATHER + '_' + f, MEMBER_SON + i + '_' + f});
                kb.addRecord(PRED_PARENT, new String[]{MEMBER_FATHER + '_' + f, MEMBER_DAUGHTER + i + '_' + f});
                kb.addRecord(PRED_PARENT, new String[]{MEMBER_MOTHER + '_' + f, MEMBER_SON + i + '_' + f});
                kb.addRecord(PRED_PARENT, new String[]{MEMBER_MOTHER + '_' + f, MEMBER_DAUGHTER + i + '_' + f});
            }

            /* brother/sister/sibling */
            for (int i = 0; i < functionality; i++) {
                for (int j = 0; j < functionality; j++) {
                    if (i != j) {
                        kb.addRecord(PRED_BROTHER, new String[]{MEMBER_SON + i + '_' + f, MEMBER_SON + j + '_' + f});
                        kb.addRecord(PRED_SISTER, new String[]{MEMBER_DAUGHTER + i + '_' + f, MEMBER_DAUGHTER + j + '_' + f});
                        kb.addRecord(PRED_SIBLING, new String[]{MEMBER_SON + i + '_' + f, MEMBER_SON + j + '_' + f});
                        kb.addRecord(PRED_SIBLING, new String[]{MEMBER_DAUGHTER + i + '_' + f, MEMBER_DAUGHTER + j + '_' + f});
                    }
                    kb.addRecord(PRED_BROTHER, new String[]{MEMBER_SON + i + '_' + f, MEMBER_DAUGHTER + j + '_' + f});
                    kb.addRecord(PRED_SISTER, new String[]{MEMBER_DAUGHTER + i + '_' + f, MEMBER_SON + j + '_' + f});
                    kb.addRecord(PRED_SIBLING, new String[]{MEMBER_SON + i + '_' + f, MEMBER_DAUGHTER + j + '_' + f});
                    kb.addRecord(PRED_SIBLING, new String[]{MEMBER_DAUGHTER + i + '_' + f, MEMBER_SON + j + '_' + f});
                }
            }

            /* gender */
            kb.addRecord(PRED_GENDER, new String[]{MEMBER_FATHER + '_' + f, GENDER_MALE});
            kb.addRecord(PRED_GENDER, new String[]{MEMBER_MOTHER + '_' + f, GENDER_FEMALE});
            for (int i = 0; i < functionality; i++) {
                kb.addRecord(PRED_GENDER, new String[]{MEMBER_SON + i + '_' + f, GENDER_MALE});
                kb.addRecord(PRED_GENDER, new String[]{MEMBER_DAUGHTER + i + '_' + f, GENDER_FEMALE});
            }
        }

        kb.dump(dumpPath);
    }

}
