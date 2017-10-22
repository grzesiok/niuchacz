package niuchacz.dcf.core.measures;

public class NGramCarefullSimilarity implements Similarity {

    private final int n;

    public NGramCarefullSimilarity(int n) {
        this.n = n;
    }

    public NGramCarefullSimilarity() {
        this.n = 2;
    }

	public double similarity(String x, String y) {
		int lenA = x.length()-n+1;
		int lenB = y.length()-n+1;
		int[] d = new int[lenA+lenB];

		int numExactChars = 0;
		int similarity = 0;
		int div = 1;

		for(int i = 0;i < lenA;i++)
		{
			for(int j = 0;j < lenB;j++)
			{
				numExactChars = 0;
				for(int k = 0;k < n;k++)
				{
					numExactChars += (x.charAt(i+k) == y.charAt(j+k)) ? 1 : 0;
				}
				d[i] = Math.max(d[i], numExactChars);
				d[lenA+j] = Math.max(d[lenA+j], numExactChars);
			}
		}
		for(int i = 0;i < d.length;i++)
			similarity += d[i];
		if(lenA != 0)
			div = 2*lenA*n;
		return (similarity*100)/div;
	}

}
