package niuchacz.dcf.core.measures;

public class LevenshteinDistance implements Distance {
	
	private final int costInsert;
	private final int costDelete;
	private final int costModify;
	
	public LevenshteinDistance() {
		this(1, 1, 1);
	}

	public LevenshteinDistance(int costInsert, int costDelete, int costModify) {
		this.costInsert = costInsert;
		this.costDelete = costDelete;
		this.costModify = costModify;
	}
	
	private int min(int a, int b, int c) {
		return Math.min(a, Math.min(b, c));
	}

	public int distance(String x, String y) {
		int[][] d = new int[x.length()][y.length()];
		
		for(int i = 0;i < x.length();i++)
			d[i][0] = i;
		for(int i = 0;i < y.length();i++)
			d[0][i] = i;
		for(int i = 1;i < x.length();i++)
			for(int j = 1;j < y.length();j++) {
				if(x.charAt(i) == y.charAt(j))
					d[i][j] = d[i-1][j-1];
				else d[i][j] = min(d[i-1][j] + costDelete, d[i][j-1] + costInsert, d[i-1][j-1] + costModify);
			}
		return d[x.length()-1][y.length()-1];
	}

}
