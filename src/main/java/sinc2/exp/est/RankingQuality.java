package sinc2.exp.est;

import com.google.gson.Gson;

import java.io.FileReader;
import java.io.IOException;
import java.util.Arrays;

public class RankingQuality {

    public static void main(String[] args) throws IOException {
        if (1 != args.length) {
            System.err.println("Usage: <ranking file>");
            return;
        }
        RankingQuality test = new RankingQuality();
        int[][][][] rankings = test.loadRanking(args[0]);
        test.showInversionNumbers(rankings);
        test.showObservationRatio(rankings);
    }

    int[][][][] loadRanking(String filePath) throws IOException {
        Gson gson = new Gson();
        FileReader reader = new FileReader(filePath);
        int[][][][] rankings = gson.fromJson(reader, int[][][][].class);
        reader.close();
        return rankings;
    }

    void showInversionNumbers(int[][][][] allRankings) {   // int[relation idx][rule length][beams][rank]
        System.out.println("Inversion Numbers:");
        System.out.println("Rel\t|r|=0\t|r|=1\t...");
        int[][][] inversion_numbers = new int[allRankings.length][][];
        double[][][] normalized_inversion_numbers = new double[allRankings.length][][];

        /* Show inversion numbers */
        for (int rel_idx = 0; rel_idx < allRankings.length; rel_idx++) {
            System.out.print(rel_idx);
            System.out.print('\t');
            int[][][] rankings_in_relation = allRankings[rel_idx];
            int[][] inversion_numbers_in_relation = new int[rankings_in_relation.length][];
            double[][] normalized_inversion_numbers_in_relation = new double[rankings_in_relation.length][];
            for (int rule_length = 0; rule_length < rankings_in_relation.length; rule_length++) {
                int[][] rankings_in_beams = rankings_in_relation[rule_length];
                int[] inversion_numbers_in_rule_lengths = new int[rankings_in_beams.length];
                double[] normalized_inversion_numbers_in_rule_lengths = new double[rankings_in_beams.length];
                for (int beam_idx = 0; beam_idx < rankings_in_beams.length; beam_idx++) {
                    int[] rankings = rankings_in_beams[beam_idx];
                    int inversion_number = calcInversionNumber(rankings);
                    inversion_numbers_in_rule_lengths[beam_idx] = inversion_number;
                    normalized_inversion_numbers_in_rule_lengths[beam_idx] = inversion_number * 2.0 / (rankings.length * (rankings.length - 1));
                }
                System.out.print(Arrays.toString(inversion_numbers_in_rule_lengths));
                System.out.print('\t');
                inversion_numbers_in_relation[rule_length] = inversion_numbers_in_rule_lengths;
                normalized_inversion_numbers_in_relation[rule_length] = normalized_inversion_numbers_in_rule_lengths;
            }
            System.out.println();
            inversion_numbers[rel_idx] = inversion_numbers_in_relation;
            normalized_inversion_numbers[rel_idx] = normalized_inversion_numbers_in_relation;
        }

        /* Show normalized inversion numbers */
        System.out.println("\nNormalized Inversion Numbers:");
        System.out.println("Rel\t|r|=0\t|r|=1\t...");
        for (int rel_idx = 0; rel_idx < allRankings.length; rel_idx++) {
            System.out.print(rel_idx);
            System.out.print('\t');
            double[][] normalized_inversion_numbers_in_relation = normalized_inversion_numbers[rel_idx];
            for (int rule_length = 0; rule_length < normalized_inversion_numbers_in_relation.length; rule_length++) {
                double[] normalized_inversion_numbers_in_rule_lengths = normalized_inversion_numbers_in_relation[rule_length];
                System.out.print(Arrays.toString(normalized_inversion_numbers_in_rule_lengths));
                System.out.print('\t');
            }
            System.out.println();
        }

        /* Show averaged inversion numbers */
        System.out.println("\nAvg. Inversion Numbers:");
        System.out.println("Rel\t|r|=0\t|r|=1\t...");
        for (int rel_idx = 0; rel_idx < allRankings.length; rel_idx++) {
            System.out.print(rel_idx);
            System.out.print('\t');
            int[][] inversion_numbers_in_relation = inversion_numbers[rel_idx];
            for (int rule_length = 0; rule_length < inversion_numbers_in_relation.length; rule_length++) {
                int[] inversion_numbers_in_rule_lengths = inversion_numbers_in_relation[rule_length];
                System.out.printf("%.2f\t", average(inversion_numbers_in_rule_lengths));
            }
            System.out.println();
        }

        /* Show averaged normalized inversion numbers */
        System.out.println("\nAvg. Normalized Inversion Numbers: (1e-2)");
        System.out.println("Rel\t|r|=0\t|r|=1\t...");
        for (int rel_idx = 0; rel_idx < allRankings.length; rel_idx++) {
            System.out.print(rel_idx);
            System.out.print('\t');
            double[][] normalized_inversion_numbers_in_relation = normalized_inversion_numbers[rel_idx];
            for (int rule_length = 0; rule_length < normalized_inversion_numbers_in_relation.length; rule_length++) {
                double[] normalized_inversion_numbers_in_rule_lengths = normalized_inversion_numbers_in_relation[rule_length];
                System.out.printf("%.2f\t", average(normalized_inversion_numbers_in_rule_lengths) * 100);
            }
            System.out.println();
        }
    }

    double average(int[] arr) {
        double sum = 0.0;
        for (int i: arr) {
            sum += i;
        }
        return sum / arr.length;
    }

    double average(double[] arr) {
        double sum = 0.0;
        for (double d: arr) {
            sum += d;
        }
        return sum / arr.length;
    }

    int calcInversionNumber(int[] arr) {
        int cnt = 0;
        for (int i = 0; i < arr.length; i++) {
            for (int j = i + 1; j < arr.length; j++) {
                cnt += (arr[i] > arr[j]) ? 1 : 0;
            }
        }
        return cnt;
    }

    void showObservationRatio(int[][][][] allRankings) {    // int[relation idx][rule length][beams][rank]
        double[][][][] observation_ratios = new double[allRankings.length][][][];
        for (int rel_idx = 0; rel_idx < allRankings.length; rel_idx++) {
            int[][][] rankings_in_relation = allRankings[rel_idx];
            double[][][] obs_rat_in_relation = new double[rankings_in_relation.length][][];
            observation_ratios[rel_idx] = obs_rat_in_relation;
            for (int rule_length = 0; rule_length < rankings_in_relation.length; rule_length++) {
                int[][] rankings_at_length = rankings_in_relation[rule_length];
                double[][] obs_rat_at_length = new double[rankings_at_length.length][];
                obs_rat_in_relation[rule_length] = obs_rat_at_length;
                for (int beam_idx = 0; beam_idx < rankings_at_length.length; beam_idx++) {
                    obs_rat_at_length[beam_idx] = calcObservationRatios(rankings_at_length[beam_idx]);
                }
            }
        }

        /* Show observation ratios */
        System.out.println("\nObservation Ratios:");
        for (int rel_idx = 0; rel_idx < allRankings.length; rel_idx++) {
            System.out.printf("Relation@%d:\n", rel_idx);
            System.out.println("|r|\tBeams");
            double[][][] obs_rat_in_relation = observation_ratios[rel_idx];
            for (int rule_length = 0; rule_length < obs_rat_in_relation.length; rule_length++) {
                System.out.printf("%d\t", rule_length);
                double[][] obs_rat_at_length = obs_rat_in_relation[rule_length];
                for (int beam_idx = 0; beam_idx < obs_rat_at_length.length; beam_idx++) {
                    System.out.print(Arrays.toString(obs_rat_at_length[beam_idx]));
                    System.out.print('\t');
                }
                System.out.println();
            }
        }

        /* Show max. observation ratios */
        System.out.println("\nMax. Observation Ratios:");
        System.out.println("Rel\t|r|=0\t|r|=1\t...");
        for (int rel_idx = 0; rel_idx < allRankings.length; rel_idx++) {
            System.out.print(rel_idx);
            System.out.print('\t');
            double[][][] obs_rat_in_relation = observation_ratios[rel_idx];
            for (int rule_length = 0; rule_length < obs_rat_in_relation.length; rule_length++) {
                double[][] obs_rat_at_length = obs_rat_in_relation[rule_length];
                double[] max_obs_rat_at_length = new double[obs_rat_at_length.length];
                for (int beam_idx = 0; beam_idx < obs_rat_at_length.length; beam_idx++) {
                    max_obs_rat_at_length[beam_idx] = max(obs_rat_at_length[beam_idx]);
                }
                System.out.printf("%s\t", Arrays.toString(max_obs_rat_at_length));
            }
            System.out.println();
        }

        /* Show avg. max. observation ratios */
        System.out.println("\nAvg. Max. Observation Ratios:");
        System.out.println("Rel\t|r|=0\t|r|=1\t...");
        for (int rel_idx = 0; rel_idx < allRankings.length; rel_idx++) {
            System.out.print(rel_idx);
            System.out.print('\t');
            double[][][] obs_rat_in_relation = observation_ratios[rel_idx];
            for (int rule_length = 0; rule_length < obs_rat_in_relation.length; rule_length++) {
                double[][] obs_rat_at_length = obs_rat_in_relation[rule_length];
                double[] max_obs_rat_at_length = new double[obs_rat_at_length.length];
                for (int beam_idx = 0; beam_idx < obs_rat_at_length.length; beam_idx++) {
                    max_obs_rat_at_length[beam_idx] = max(obs_rat_at_length[beam_idx]);
                }
                System.out.printf("%.2f\t", average(max_obs_rat_at_length));
            }
            System.out.println();
        }

        /* Show avg. observation ratios */
        System.out.println("\nAvg. Observation Ratios:");
        System.out.println("Rel\t|r|=0\t|r|=1\t...");
        for (int rel_idx = 0; rel_idx < allRankings.length; rel_idx++) {
            System.out.print(rel_idx);
            System.out.print('\t');
            double[][][] obs_rat_in_relation = observation_ratios[rel_idx];
            for (int rule_length = 0; rule_length < obs_rat_in_relation.length; rule_length++) {
                System.out.printf("%.2f\t", average(obs_rat_in_relation[rule_length]));
            }
            System.out.println();
        }
    }

    double[] calcObservationRatios(int[] rankings) {
        double[] observation_ratios = new double[rankings.length];
        for (int i = 0; i < rankings.length; i++) {
            observation_ratios[i] = 1.0 * (rankings[i] + 1) / (i + 1);  // est. ranking / actual ranking
        }
        return observation_ratios;
    }

    double max(double[] arr) {
        double max = arr[0];
        for (double d: arr) {
            max = Math.max(d, max);
        }
        return max;
    }

    double average(double[][] arr) {
        double sum = 0;
        int size = 0;
        for (double[] arr2: arr) {
            size += arr2.length;
            for (double d: arr2) {
                sum += d;
            }
        }
        return sum / size;
    }
}
