package niuchacz.dcf.core.ngram;

import opennlp.tools.tokenize.Tokenizer;
import opennlp.tools.util.Span;

public class NgramTokenizer implements Tokenizer {
	
	private int nGramSize;
	
	public NgramTokenizer(int nGramSize) {
		this.nGramSize = nGramSize;
	}

	public String[] tokenize(String text) {
		if(text == null) {
			return null;
		} else if(text.length() < nGramSize) {
			return new String[]{text};
		}
		int maxNGrams = text.length()-nGramSize+1;
		String[] nGrams = new String[maxNGrams];
		for(int i = 0;i < maxNGrams;i++) {
			nGrams[i] = text.substring(i, Math.min(i+nGramSize, text.length()));
		}
		return nGrams;
	}

	public Span[] tokenizePos(String text) {
		return null;
	}

}
