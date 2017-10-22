package niuchacz.dcf.core.eti;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

import niuchacz.dcf.core.measures.DamerauLevenshteinSimilarity;
import niuchacz.dcf.core.measures.Similarity;
import niuchacz.dcf.core.ngram.NgramTokenizer;
import opennlp.tools.tokenize.Tokenizer;

public class ErrorToleranceIndex implements FuzzyIndex {
	
	private Tokenizer tokenizer;
	private Similarity measure;
	private Map<String, Set<String>> ngramIndex;

	public ErrorToleranceIndex(int ngramSize) {
		this(ngramSize, 1, 1, 1);
	}

	public ErrorToleranceIndex(int ngramSize, int costInsert, int costDelete, int costModify) {
		tokenizer = new NgramTokenizer(ngramSize);
		measure = new DamerauLevenshteinSimilarity(costInsert, costDelete, costModify);
		ngramIndex = new HashMap<String, Set<String>>();
	}
	
	protected void insertToken(String ngram, String refText) {
		if(!ngramIndex.containsKey(ngram)) {
			ngramIndex.put(ngram, new HashSet<String>());
		}
		ngramIndex.get(ngram).add(refText);
	}

	public void insert(String text) {
		text.replaceAll(" ", "");
		for(String key : tokenizer.tokenize(text)) {
			insertToken(key, text);
		}
	}

	protected void deleteToken(String ngram, String refText) {
		if(!ngramIndex.containsKey(ngram)) {
			return;
		}
		ngramIndex.get(ngram).remove(refText);
		if(ngramIndex.get(ngram).size() == 0) {
			ngramIndex.remove(ngram);
		}
	}

	public void delete(String text) {
		text.replaceAll(" ", "");
		for(String key : tokenizer.tokenize(text)) {
			deleteToken(key, text);
		}
	}
	
	public void dump() {
		for(String key : ngramIndex.keySet()) {
			System.out.println(key+" => "+ngramIndex.get(key));
		}
	}

	public FuzzyResult fetchScored(String text, int lenDiff, double similarityThreshold, int minNgrams) {
		return fetchScoredWithPrefix(text, null, lenDiff, similarityThreshold, minNgrams);
	}

	public FuzzyResult fetchScoredWithPrefix(String text, String prefix, int lenDiff, double similarityThreshold, int minNgrams) {
		int maxOccurence = 0, currentOccurence;
		double similarity;
		Map<String, Integer> tokenDedup = new HashMap<String, Integer>();
		FuzzyResult result = new FuzzyResult();
		
		text.replaceAll(" ", "");
		for(String ngram : tokenizer.tokenize(text)) {
			if(!ngramIndex.containsKey(ngram))
				continue;
			for(String originalText : ngramIndex.get(ngram)) {
				if(text.length()+lenDiff < originalText.length() || text.length()-lenDiff > originalText.length())
					continue;
				if(prefix != null && originalText.startsWith(prefix))
					continue;
				if(tokenDedup.containsKey(originalText)){
					currentOccurence = tokenDedup.get(originalText)+1;
					tokenDedup.replace(originalText, currentOccurence);
				} else {
					currentOccurence = 1;
					tokenDedup.put(originalText, currentOccurence);
				}
				maxOccurence = Math.max(maxOccurence, currentOccurence);
			}
		}
		result.similarity = 0;
	    if(maxOccurence < minNgrams)
	      return result;
		for(String key : tokenDedup.keySet()) {
			if(tokenDedup.get(key) == maxOccurence) {
				similarity = measure.similarity(key, text);
				//System.out.println("dist["+text+","+key+"]="+similarity);
				if(similarity >= similarityThreshold && similarity > result.similarity) {
					result.text = key;
					result.similarity = similarity;
				}
			}
		}
		return result;
	}
}
