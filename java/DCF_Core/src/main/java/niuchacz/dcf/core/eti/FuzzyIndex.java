package niuchacz.dcf.core.eti;

import niuchacz.dcf.core.Index;

public interface FuzzyIndex extends Index {
	public FuzzyResult fetchScored(String text, int lenDiff, double similarityThreshold, int minNgrams);
	public FuzzyResult fetchScoredWithPrefix(String text, String prefix, int lenDiff, double similarityThreshold, int minNgrams);
}
