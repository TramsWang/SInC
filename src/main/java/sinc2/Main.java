package sinc2;

import org.apache.commons.cli.*;
import sinc2.common.SincException;
import sinc2.impl.est.EstSinc;
import sinc2.impl.negsamp.SincWithAdvNegSampling;
import sinc2.impl.negsamp.SincWithFragmentedCachedRule;
import sinc2.impl.negsamp.SincWithNegSampling;
import sinc2.rule.EvalMetric;

public class Main {

    public static final String DEFAULT_PATH = ".";
    public static final int DEFAULT_THREADS = 1;
    public static final int DEFAULT_BEAM_WIDTH = 3;
    public static final double DEFAULT_FACT_COVERAGE = 0.05;
    public static final double DEFAULT_CONSTANT_COVERAGE = 0.25;
    public static final double DEFAULT_STOP_COMPRESSION_RATE = 0.9;
    public static final double DEFAULT_OBSERVATION_RATIO = 2.0;
    public static final EvalMetric DEFAULT_EVAL_METRIC = EvalMetric.CompressionCapacity;
    public static final float DEFAULT_BUDGET_FACTOR = 0;
    public static final int HELP_WIDTH = 125;

    private static final String SHORT_OPT_HELP = "h";
    private static final String SHORT_OPT_INPUT = "I";
    private static final String SHORT_OPT_OUTPUT = "O";
    private static final String SHORT_OPT_NEG_INPUT = "N";
//    private static final String SHORT_OPT_NEG_ADVERSARIAL = "A";
    private static final String SHORT_OPT_NEG_BUDGET_FACTOR = "g";
    private static final String SHORT_OPT_NEG_WEIGHT = "w";
    private static final String SHORT_OPT_THREAD = "t";
    private static final String SHORT_OPT_VALIDATE = "v";
    private static final String SHORT_OPT_BEAM_WIDTH = "b";
    private static final String SHORT_OPT_EVAL_METRIC = "e";
    private static final String SHORT_OPT_FACT_COVERAGE = "f";
    private static final String SHORT_OPT_CONSTANT_COVERAGE = "c";
    private static final String SHORT_OPT_STOP_COMPRESSION_RATE = "p";
    private static final String SHORT_OPT_OBSERVATION_RATIO = "o";
    private static final String LONG_OPT_HELP = "help";
    private static final String LONG_OPT_INPUT = "input";
    private static final String LONG_OPT_OUTPUT = "output";
    private static final String LONG_OPT_NEG_INPUT = "neg-samples";
//    private static final String LONG_OPT_NEG_ADVERSARIAL = "neg-adv";
    private static final String LONG_OPT_NEG_BUDGET_FACTOR = "neg-budget";
    private static final String LONG_OPT_NEG_WEIGHT = "neg-weight";
    private static final String LONG_OPT_THREAD = "thread";
    private static final String LONG_OPT_VALIDATE = "validate";
    private static final String LONG_OPT_BEAM_WIDTH = "beam-width";
    private static final String LONG_OPT_EVAL_METRIC = "eval-metric";
    private static final String LONG_OPT_FACT_COVERAGE = "fact-coverage";
    private static final String LONG_OPT_CONSTANT_COVERAGE = "const-coverage";
    private static final String LONG_OPT_STOP_COMPRESSION_RATE = "stop-comp-rate";
    private static final String LONG_OPT_OBSERVATION_RATIO = "observation-ratio";

    private static final Option OPTION_HELP = Option.builder(SHORT_OPT_HELP).longOpt(LONG_OPT_HELP)
            .desc("Display this help").build();
    private static final Option OPTION_INPUT_PATH = Option.builder(SHORT_OPT_INPUT).longOpt(LONG_OPT_INPUT)
            .numberOfArgs(2).argName("path> <name").type(String.class)
            .desc("The path to the input KB and the name of the KB").build();
    private static final Option OPTION_OUTPUT_PATH = Option.builder(SHORT_OPT_OUTPUT).longOpt(LONG_OPT_OUTPUT)
            .numberOfArgs(2).argName("path> <name").type(String.class)
            .desc("The path to where the output/compressed KB is stored and the name of the output KB").build();
    private static final Option OPTION_NEG_INPUT = Option.builder(SHORT_OPT_NEG_INPUT).longOpt(LONG_OPT_NEG_INPUT)
            .numberOfArgs(2).argName("path> <name")
            .desc("The path to the negative KB and the name of the KB. If specified, negative sampling is turned on.").build();
//    private static final Option OPTION_NEG_ADVERSARIAL = Option.builder(SHORT_OPT_NEG_ADVERSARIAL).longOpt(LONG_OPT_NEG_ADVERSARIAL)
//            .desc("Using adversarial negative sampling.").build();
    private static final Option OPTION_NEG_BUDGET_FACTOR = Option.builder(SHORT_OPT_NEG_BUDGET_FACTOR).longOpt(LONG_OPT_NEG_BUDGET_FACTOR)
            .hasArg().argName("budget").type(Float.class).desc("The budget factor of negative sampling. If negative sampling is adopted and budget factor >= 1, the adversarial sampling will be employed").build();
    private static final Option OPTION_NEG_WEIGHT = Option.builder(SHORT_OPT_NEG_WEIGHT).longOpt(LONG_OPT_NEG_WEIGHT)
            .desc("Whether negative samples have different weight. This is only affective when negative sampling is turned on.").build();
    private static final Option OPTION_THREAD = Option.builder(SHORT_OPT_THREAD).longOpt(LONG_OPT_THREAD)
            .argName("#threads").hasArg().type(Integer.class).desc("The number of threads").build();
    private static final Option OPTION_VALIDATE = Option.builder(SHORT_OPT_VALIDATE).longOpt(LONG_OPT_VALIDATE)
            .desc("Validate result after compression").build();
    private static final Option OPTION_BEAM_WIDTH = Option.builder(SHORT_OPT_BEAM_WIDTH).longOpt(LONG_OPT_BEAM_WIDTH)
            .desc(String.format("Beam search width (Default %d)", DEFAULT_BEAM_WIDTH)).argName("b").hasArg().type(Integer.class).build();
    private static final Option OPTION_EVAL_METRIC = Option.builder(SHORT_OPT_EVAL_METRIC).longOpt(LONG_OPT_EVAL_METRIC)
            .argName("name").hasArg().type(String.class).build();
    private static final Option OPTION_FACT_COVERAGE = Option.builder(SHORT_OPT_FACT_COVERAGE).longOpt(LONG_OPT_FACT_COVERAGE)
            .desc(String.format("Set fact coverage threshold (Default %.2f)", DEFAULT_FACT_COVERAGE)).argName("fc").hasArg().type(Double.class).build();
    private static final Option OPTION_CONSTANT_COVERAGE = Option.builder(SHORT_OPT_CONSTANT_COVERAGE).longOpt(LONG_OPT_CONSTANT_COVERAGE)
            .desc(String.format("Set constant coverage threshold (Default %.2f)", DEFAULT_CONSTANT_COVERAGE)).argName("cc").hasArg().type(Double.class).build();
    private static final Option OPTION_STOP_COMPRESSION_RATE = Option.builder(SHORT_OPT_STOP_COMPRESSION_RATE).longOpt(LONG_OPT_STOP_COMPRESSION_RATE)
            .desc(String.format("Set stopping compression rate (Default %.2f)", DEFAULT_STOP_COMPRESSION_RATE)).argName("scr").hasArg().type(Double.class).build();
    private static final Option OPTION_OBSERVATION_RATIO = Option.builder(SHORT_OPT_OBSERVATION_RATIO).longOpt(LONG_OPT_OBSERVATION_RATIO)
            .desc(String.format("Use rule mining estimation and set observation ratio (Default %.2f). " +
                    "Estimation is adopted by default. If the value is set smaller than 1.0, estimation is turned off and " +
                    "the basic model is applied.", DEFAULT_OBSERVATION_RATIO)).argName("or").hasArg().type(Double.class).build();

    static {
        /* List Available Eval Metrics */
        EvalMetric[] metrics = EvalMetric.values();
        StringBuilder eval_metric_desc_builder = new StringBuilder("Select in the evaluation metrics (Default ")
                .append(DEFAULT_EVAL_METRIC.getSymbol()).append("). Available options are: ")
                .append(metrics[0].getSymbol()).append('(').append(metrics[0].getDescription()).append(')');
        for (int i = 1; i < metrics.length; i++) {
            EvalMetric metric = metrics[i];
            eval_metric_desc_builder.append(", ").append(metric.getSymbol()).append('(').append(metric.getDescription()).append(')');
        }
        OPTION_EVAL_METRIC.setDescription(eval_metric_desc_builder.toString());
    }

    public static void main(String[] args) throws Exception {
        Options options = buildOptions();
        SInC sinc = parseArgs(options, args);
        if (null != sinc) {
            sinc.run();
        }
    }

    protected static SInC parseArgs(Options options, String[] args) throws Exception {
        CommandLineParser parser = new DefaultParser();
        CommandLine cmd = parser.parse(options, args);

        /* Help */
        if (cmd.hasOption(OPTION_HELP)) {
            HelpFormatter formatter = new HelpFormatter();
            formatter.setWidth(HELP_WIDTH);
            formatter.printHelp("java -jar sinc.jar", options, true);
            return null;
        }

        /* Input/Output */
        String input_path = DEFAULT_PATH;
        String output_path = DEFAULT_PATH;
        String input_kb_name = null;
        String output_kb_name = null;
        if (cmd.hasOption(OPTION_INPUT_PATH)) {
            String[] values = cmd.getOptionValues(OPTION_INPUT_PATH);
            input_path = values[0];
            input_kb_name = values[1];
            System.out.printf("Input path set to: %s/%s\n", input_path, input_kb_name);
        }
        if (cmd.hasOption(OPTION_OUTPUT_PATH)) {
            String[] values = cmd.getOptionValues(OPTION_OUTPUT_PATH);
            output_path = values[0];
            output_kb_name = values[1];
            System.out.printf("Output path set to: %s/%s\n", output_path, output_kb_name);
        }
        if (null == input_kb_name) {
            System.err.println("Missing input KB name");
            return null;
        }
        output_kb_name = (null == output_kb_name) ? input_kb_name + "_comp" : output_kb_name;

        /* Assign negative sampling parameters */
        String neg_base_path = null;
        String neg_kb_name = null;
        boolean neg_weight = cmd.hasOption(OPTION_NEG_WEIGHT);
        if (cmd.hasOption(OPTION_NEG_INPUT)) {
            String[] values = cmd.getOptionValues(OPTION_NEG_INPUT);
            neg_base_path = values[0];
            neg_kb_name = values[1];
            System.out.printf("Negative sampling on: %s/%s (weight=%b)\n", neg_base_path, neg_kb_name, neg_weight);
        }
        float budget_factor = DEFAULT_BUDGET_FACTOR;
        if (cmd.hasOption(OPTION_NEG_BUDGET_FACTOR)) {
            String value = cmd.getOptionValue(OPTION_NEG_BUDGET_FACTOR);
            if (null != value) {
                budget_factor = Float.parseFloat(value);
                System.out.printf("Budget factor set to (Adversarial %s): %.2f\n", budget_factor >= 1 ? "ON" : "OFF", budget_factor);
            }
        }

        /* Assign Run-time parameters */
        int threads = DEFAULT_THREADS;
        if (cmd.hasOption(OPTION_THREAD)) {
            String value = cmd.getOptionValue(OPTION_THREAD);
            if (null != value) {
                threads = Integer.parseInt(value);
                System.out.println("#Threads set to: " + threads);
            }
        }
        boolean validation = cmd.hasOption(SHORT_OPT_VALIDATE);
        int beam = DEFAULT_BEAM_WIDTH;
        if (cmd.hasOption(SHORT_OPT_BEAM_WIDTH)) {
            String value = cmd.getOptionValue(SHORT_OPT_BEAM_WIDTH);
            if (null != value) {
                beam = Integer.parseInt(value);
                System.out.println("Beamwidth set to: " + beam);
            }
        }
        EvalMetric metric = DEFAULT_EVAL_METRIC;
        if (cmd.hasOption(SHORT_OPT_EVAL_METRIC)) {
            String value = cmd.getOptionValue(SHORT_OPT_EVAL_METRIC);
            if (null != value) {
                metric = EvalMetric.getBySymbol(value);
                if (null == metric) {
                    throw new SincException("Unknown evaluation metric: " + value);
                }
                System.out.println("Evaluation metric set to: " + metric.getSymbol());
            }
        }
        double fc = DEFAULT_FACT_COVERAGE;
        if (cmd.hasOption(SHORT_OPT_FACT_COVERAGE)) {
            String value = cmd.getOptionValue(SHORT_OPT_FACT_COVERAGE);
            if (null != value) {
                fc = Double.parseDouble(value);
                System.out.println("Fact coverage set to: " + fc);
            }
        }
        double cc = DEFAULT_CONSTANT_COVERAGE;
        if (cmd.hasOption(SHORT_OPT_CONSTANT_COVERAGE)) {
            String value = cmd.getOptionValue(SHORT_OPT_CONSTANT_COVERAGE);
            if (null != value) {
                cc = Double.parseDouble(value);
                System.out.println("Constant coverage set to: " + cc);
            }
        }
        double scr = DEFAULT_STOP_COMPRESSION_RATE;
        if (cmd.hasOption(OPTION_STOP_COMPRESSION_RATE)) {
            String value = cmd.getOptionValue(OPTION_STOP_COMPRESSION_RATE);
            if (null != value) {
                scr = Double.parseDouble(value);
                System.out.println("Stopping compression rate set to: " + scr);
            }
        }
        double or = DEFAULT_OBSERVATION_RATIO;
        if (cmd.hasOption(OPTION_OBSERVATION_RATIO)) {
            String value = cmd.getOptionValue(OPTION_OBSERVATION_RATIO);
            if (null != value) {
                or = Double.parseDouble(value);
                System.out.println("Observation ratio set to: " + or);
            }
        }

        /* Create SInC Object */
        SincConfig config = new SincConfig(
                input_path, input_kb_name, output_path, output_kb_name,
                threads, validation, beam, metric, fc, cc, scr, or,
                neg_base_path, neg_kb_name, budget_factor, neg_weight
        );
        if (1.0 > or) {
            if (null != neg_base_path) {
                /* Negative sampling */
                if (1.0 > budget_factor) {
                    return new SincWithNegSampling(config);
                } else {
                    return new SincWithAdvNegSampling(config);
                }
            }
            return new SincWithFragmentedCachedRule(config);
        } else {
            return new EstSinc(config);
        }
    }

    protected static Options buildOptions() {
        Options options = new Options();

        /* Help */
        options.addOption(OPTION_HELP);

        /* Input/output options */
        options.addOption(OPTION_INPUT_PATH);
        options.addOption(OPTION_OUTPUT_PATH);

        /* Negative sampling options */
        options.addOption(OPTION_NEG_INPUT);
        options.addOption(OPTION_NEG_BUDGET_FACTOR);
        options.addOption(OPTION_NEG_WEIGHT);

        /* Run-time parameter options */
        options.addOption(OPTION_THREAD);
        options.addOption(OPTION_VALIDATE);
        options.addOption(OPTION_BEAM_WIDTH);
        options.addOption(OPTION_EVAL_METRIC);
        options.addOption(OPTION_FACT_COVERAGE);
        options.addOption(OPTION_CONSTANT_COVERAGE);
        options.addOption(OPTION_STOP_COMPRESSION_RATE);
        options.addOption(OPTION_OBSERVATION_RATIO);

        return options;
    }
}
