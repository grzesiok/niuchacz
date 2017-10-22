package niuchacz.dcf.stdtool;

import java.util.Date;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.TreeSet;

import niuchacz.dcf.stdtool.NameStdTool.Result;
import niuchacz.dcf.stdtool.cache.ACacheEntry.TokenType;
import niuchacz.dcf.stdtool.cache.ICacheEntry;
import niuchacz.dcf.stdtool.cache.NameCacheEntry;
import niuchacz.dcf.stdtool.parse.DefaultTokenizer;

public class NameStdTool {
	private Map<String, NameCacheEntry> cacheToknes = new HashMap<String, NameCacheEntry>();
	private final int maxTokenSize = 5;
	private DefaultTokenizer tokenizer;
	
	public NameStdTool() {
        init();
	}
	
	public static class Result {
		
		public static class Entry {
			public String companyName;
			public String legalForm;
			public String acronym;
			public String metaCompanyName;
			public int specialFlag;
			private transient TreeSet<String> companyNameTreeSet = new TreeSet<String>();
			
			public Entry() {
				this("", null, "", null, 0);
			}
			
			public Entry(String companyName, String legalForm, String acronym, String metaCompanyName, int specialFlag) {
				this.companyName = companyName;
				this.legalForm = legalForm;
				this.acronym = acronym;
				this.metaCompanyName = metaCompanyName;
				this.specialFlag = specialFlag;
			}
			
			public boolean isEmpty() {
				return companyNameTreeSet.size() == 0;
			}
			
			public boolean addCompanyNamePart(String str) {
				return companyNameTreeSet.add(str);
			}
			
			public void completeEntry() {
				Iterator<String> iter = companyNameTreeSet.iterator();
				StringBuilder companyNameSB = new StringBuilder();
				while(iter.hasNext()) {
					companyNameSB.append(iter.next());
				}
				companyNameTreeSet = null;
				this.companyName = companyNameSB.toString();
			}
			
			@Override
			public boolean equals(Object obj) {
		        if(obj == null || !(obj instanceof Entry)) {
		            return false;
		        }
		        Entry result = (Entry)obj;
		    	if(!this.companyName.equals(result.companyName))
		    		return false;
		    	if(!this.legalForm.equals(result.legalForm))
		    		return false;
		    	if(!this.acronym.equals(result.acronym))
		    		return false;
		    	if(this.specialFlag != result.specialFlag)
		    		return false;
		    	return true;
			}

			@Override
			public String toString() {
				return "{companyName=\'"+companyName+"\'"+
					   ",legalForm=\'"+legalForm+"\'"+
					   ",acronym=\'"+acronym+"\'"+
					   ",metaCompanyName=\'"+metaCompanyName+"\'"+
					   ",specialFlag=\'"+specialFlag+"\'}";
			}
		}
		
		private String companyNameFull;
		private List<Entry> altCompanyNames = new LinkedList<Entry>();
		
		public Result(String companyNameFull) {
			this.companyNameFull = companyNameFull;
		}
		
		public Iterator<Entry> iterator() {
			return altCompanyNames.iterator();
		}
		
		public void addCompanyName(Entry entry) {
			altCompanyNames.add(entry);
		}
		
		public String getCompanyNameFull() {
			return this.companyNameFull;
		}

		@Override
		public String toString() {
			Iterator<Result.Entry> iter = iterator();
			String resultString = "";
	    	while(iter.hasNext()) {
		    	Result.Entry entry = iter.next();
		    	if(resultString.length() > 0) {
		    		resultString += ",\n"+entry.toString();
		    	} else {
		    		resultString += entry.toString();
		    	}
	    	}
			return "{"+resultString+"}";
		}
		
		@Override
		public boolean equals(Object obj) {
	        if(obj == null || !(obj instanceof Result)) {
	            return false;
	        }
	        Result otherResult = (Result)obj;
	    	Iterator<Result.Entry> otherIter = otherResult.iterator();
	    	Iterator<Result.Entry> thisIter = this.iterator();
	    	while(otherIter.hasNext() && thisIter.hasNext()) {
	    		if(!thisIter.next().equals(otherIter.next()))
	    			return false;
	    	}
	    	return true;
		}
	}
	
	public void init() {
		tokenizer = new DefaultTokenizer(maxTokenSize) {
			private ICacheEntry stdToken;
			
			protected Decision scan(String nonStdToken) {
				stdToken = cacheToknes.get(nonStdToken);
				if(stdToken != null && stdToken.getTokenType() != null) {
					if(stdToken.getTokenType().equals(NameCacheEntry.NameTokenType.HardStop)) {
						return Decision.HardStopScanning;
					}
					return Decision.StopScanning;
				}
				return Decision.ContinueScanning;
			}
			
			protected ICacheEntry apply(String nonStdToken) {
				if(stdToken == null) {
					stdToken = new NameCacheEntry(nonStdToken, nonStdToken, NameCacheEntry.NameTokenType.Unknown, null);
				}
				return stdToken;
			}
			
			protected String[] tokenize(String text) {
				return tokenizer.tokenize(text);
			}
		};
		NameCacheEntry[] data = {
			new NameCacheEntry("DBA", null, NameCacheEntry.NameTokenType.DBASeparatorWithHWM, new Date()),
			new NameCacheEntry("AS", null, NameCacheEntry.NameTokenType.DBASeparatorHWM, new Date()),
			new NameCacheEntry("DOING BUSSINES AS", null, NameCacheEntry.NameTokenType.DBASeparator, new Date()),
			new NameCacheEntry("C/O", null, NameCacheEntry.NameTokenType.DBASeparator, new Date()),
			new NameCacheEntry("CARE OF", null, NameCacheEntry.NameTokenType.DBASeparator, new Date()),
			new NameCacheEntry("BUSSINES", "BIZ", NameCacheEntry.NameTokenType.Unknown, new Date()),
			new NameCacheEntry("CORP", "CORP", NameCacheEntry.NameTokenType.LegalForm, new Date()),
			new NameCacheEntry("CO", "CO", NameCacheEntry.NameTokenType.LegalForm, new Date()),
			new NameCacheEntry("COMPANY", "COMP", NameCacheEntry.NameTokenType.LegalForm, new Date()),
			new NameCacheEntry("LTD", "LTD", NameCacheEntry.NameTokenType.LegalForm, new Date()),
			new NameCacheEntry("POLICE DEP", "POLICEDEP", NameCacheEntry.NameTokenType.Special, new Date())
			};
		for(NameCacheEntry dataEntry : data) {
			String newAltToken = tokenizer.stemToken(dataEntry.getAltToken().toLowerCase());
			dataEntry.setAltToken(newAltToken);
			if(dataEntry.getMainToken() != null)
				dataEntry.setMainToken(dataEntry.getMainToken().toLowerCase());
			cacheToknes.put(dataEntry.getAltToken(), dataEntry);
		}
	}
	
	public String fetchBestLegalForm(String currentLegalForm, String newLegalForm) {
		if(currentLegalForm == null)
			return newLegalForm;
		if(newLegalForm.equals("co")
			|| newLegalForm.equals("corp")
			|| newLegalForm.equals("inc")
			|| currentLegalForm.charAt(0) == 'L' && newLegalForm.equals("ltd")
			|| currentLegalForm.contains(newLegalForm)) {
				return currentLegalForm;
		} else if(newLegalForm.charAt(0) == 'L' || currentLegalForm.contains("ltd")) {
			currentLegalForm = currentLegalForm.replace("ltd", "").concat(" ").concat(newLegalForm);
		} else {
			currentLegalForm = currentLegalForm.concat(" ").concat(newLegalForm);
		}
		if(currentLegalForm.equals("co") || currentLegalForm.equals("inc") || currentLegalForm.equals("ltd"))
			return "corp";
		return currentLegalForm;
	}
	
	private void addIfCompanyNameIsNotNull(Result result, Result.Entry entry) {
		if(!entry.isEmpty()) {
			entry.completeEntry();
			result.addCompanyName(entry);
		}
	}
	
	public Result split(String companyNameFull) {
		Result result = new Result(companyNameFull.toLowerCase());
		List<ICacheEntry> stdTokens;
		stdTokens = tokenizer.tokenise(result.getCompanyNameFull());
		
		Result.Entry currentCompanyName = new Result.Entry();
		for(int currTokenPosId = 0; currTokenPosId < stdTokens.size();currTokenPosId++) {
			ICacheEntry token = stdTokens.get(currTokenPosId);
			TokenType tokenType = token.getTokenType();
			//System.out.println(token.getAltToken()+"["+token.getTokenType()+"] => "+token.getMainToken());
			if(tokenType.equals(NameCacheEntry.NameTokenType.LinkWord)
				|| tokenType.equals(NameCacheEntry.NameTokenType.Article)
				|| tokenType.equals(NameCacheEntry.NameTokenType.Prepos)
				|| tokenType.equals(NameCacheEntry.NameTokenType.Error)) {
				continue;
			} else if(tokenType.equals(NameCacheEntry.NameTokenType.DBASeparator)
				|| tokenType.equals(NameCacheEntry.NameTokenType.DBASeparatorHWM)
				|| tokenType.equals(NameCacheEntry.NameTokenType.DBASeparatorWithHWM)) {
				if(tokenType.equals(NameCacheEntry.NameTokenType.DBASeparatorWithHWM)) {
					for(int i = currTokenPosId+1;i < stdTokens.size();i++) {
						if(stdTokens.get(currTokenPosId).getTokenType().equals(NameCacheEntry.NameTokenType.DBASeparator)) {
							break;
						} else if(stdTokens.get(currTokenPosId).getTokenType().equals(NameCacheEntry.NameTokenType.DBASeparatorHWM)) {
							currTokenPosId = i;
							break;
						}
					}
				}
				addIfCompanyNameIsNotNull(result, currentCompanyName);
				currentCompanyName = new Result.Entry();
			} else if(tokenType.equals(NameCacheEntry.NameTokenType.LegalForm)) {
				currentCompanyName.legalForm = fetchBestLegalForm(currentCompanyName.legalForm, token.getMainToken());
			} else if(tokenType.equals(NameCacheEntry.NameTokenType.Special)) {
				currentCompanyName.specialFlag = 1;
				currentCompanyName.addCompanyNamePart(token.getAltToken());
			} else if(tokenType.equals(NameCacheEntry.NameTokenType.HardStop)) {
				break;
			} else if(tokenType.equals(NameCacheEntry.NameTokenType.Unknown)) {
				currentCompanyName.addCompanyNamePart(token.getMainToken());
				if(token.getMainToken().charAt(0) < '0' || token.getMainToken().charAt(0) > '9') {
					currentCompanyName.acronym = currentCompanyName.acronym.concat(String.valueOf(token.getMainToken().charAt(0)));
				}
			}
		}
		addIfCompanyNameIsNotNull(result, currentCompanyName);
		return result;
	}
}
