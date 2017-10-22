package niuchacz.dcf.core.measures;

public class LevenshteinSimilarity implements Similarity {

	private final LevenshteinDistance dist;
	public LevenshteinSimilarity() {
		this(1, 1, 1);
	}

	public LevenshteinSimilarity(int costInsert, int costDelete, int costModify) {
		dist = new LevenshteinDistance(costInsert, costDelete, costModify);
	}
	
	public double similarity(String x, String y) {
		return 1-(double)dist.distance(x, y)/(double)Math.max(x.length(), y.length());
	}

}
