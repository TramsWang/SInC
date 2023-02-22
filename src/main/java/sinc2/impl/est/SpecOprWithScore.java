package sinc2.impl.est;

import sinc2.rule.Eval;
import sinc2.rule.SpecOpr;

public class SpecOprWithScore {
    public final SpecOpr opr;
    public final Eval estEval;

    public SpecOprWithScore(SpecOpr opr, Eval estEval) {
        this.opr = opr;
        this.estEval = estEval;
    }
}
