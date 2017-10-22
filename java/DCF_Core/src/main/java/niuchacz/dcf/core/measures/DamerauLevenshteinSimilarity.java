package niuchacz.dcf.core.measures;

public class DamerauLevenshteinSimilarity implements Similarity {

	private final DamerauLevenshteinDistance dist;
	public DamerauLevenshteinSimilarity() {
		this(1, 1, 1);
	}

	public DamerauLevenshteinSimilarity(int costInsert, int costDelete, int costModify) {
		dist = new DamerauLevenshteinDistance(costInsert, costDelete, costModify);
	}
	
	public double similarity(String x, String y) {
		return 1-(double)dist.distance(x, y)/(double)Math.max(x.length(), y.length());
	}

}
