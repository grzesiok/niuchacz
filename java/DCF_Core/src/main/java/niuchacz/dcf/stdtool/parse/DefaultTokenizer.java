package niuchacz.dcf.stdtool.parse;

import java.util.LinkedList;
import java.util.List;

import niuchacz.dcf.stdtool.cache.ICacheEntry;
import opennlp.tools.stemmer.PorterStemmer;
import opennlp.tools.stemmer.Stemmer;
import opennlp.tools.tokenize.SimpleTokenizer;
import opennlp.tools.tokenize.Tokenizer;

public abstract class DefaultTokenizer {
	protected final static Tokenizer tokenizer = SimpleTokenizer.INSTANCE;
	private final static Stemmer stemmer = new PorterStemmer();
	private final int maxTokenSize;
	
	public DefaultTokenizer(int maxTokenSize) {
		this.maxTokenSize = maxTokenSize;
	}
	
	public enum Decision {
		ContinueScanning,//still token was not recognized
		StopScanning,//N tokens was recognized as single token
		HardStopScanning//Each token after HARD_STOP will be removed
	}
	
	protected abstract Decision scan(String nonStdToken);
	
	protected abstract ICacheEntry apply(String nonStdToken);
	
	protected abstract String[] tokenize(String text);
	
	public String stemToken(String token) {
		String stdToken = "";
		for(String nonStdToken : tokenize(token)) {
			stdToken += stemmer.stem(nonStdToken);
		}
		return stdToken;
	}
	
	public List<ICacheEntry> tokenise(String text) {
		String[] nonStdTokens = tokenize(text.toLowerCase());
		String[] stemmedTokens = new String[nonStdTokens.length];
		List<ICacheEntry> stdTokens = new LinkedList<ICacheEntry>();

		// steam each word to standarize input before parsing it
		for(int i = 0;i < nonStdTokens.length;i++) {
			stemmedTokens[i] = (String) stemmer.stem(nonStdTokens[i]);
		}
		int currPos = 0;
		String currentNonStdToken = null;
		while(currPos < stemmedTokens.length) {
			for(int j = Math.min(maxTokenSize, stemmedTokens.length-currPos);j > 0;j--) {
				currentNonStdToken = "";
				for(int k = 0;k < j;k++) {
					currentNonStdToken += stemmedTokens[currPos+k];
				}
				Decision decision = scan(currentNonStdToken);
				if(decision.equals(Decision.StopScanning)) {
					currPos += j-1;
					System.out.println("currentNonStdToken[words="+j+",max_words="+maxTokenSize+",all_words="+stemmedTokens.length+"]="+currentNonStdToken+" <- HIT");
					break;
				} else if(decision.equals(Decision.HardStopScanning)) {
					System.out.println("currentNonStdToken[words="+j+",max_words="+maxTokenSize+",all_words="+stemmedTokens.length+"]="+currentNonStdToken+" <- HARDSTOP");
					return stdTokens;
				} else {
					System.out.println("currentNonStdToken[words="+j+",max_words="+maxTokenSize+",all_words="+stemmedTokens.length+"]="+currentNonStdToken);
					currentNonStdToken = null;
				}
			}
			if(currentNonStdToken == null) {
				currentNonStdToken = stemmedTokens[currPos];
			}
			ICacheEntry entry = apply(currentNonStdToken);
			if(entry != null) {
				stdTokens.add(apply(currentNonStdToken));
			}
			currPos++;
		}
		return stdTokens;
	}
}
