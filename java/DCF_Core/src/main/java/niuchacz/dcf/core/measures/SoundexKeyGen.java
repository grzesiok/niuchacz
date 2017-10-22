package niuchacz.dcf.core.measures;

import java.security.InvalidParameterException;

public class SoundexKeyGen implements KeyGen {

	private final boolean isRefinedAlgo;
	private final char[] mapping = new char['Z'-'A'+1];
	private char[] currentKeyArray;
	private int iKey;
	
	private void initMap(String indx, String vals) {
		if(indx.length() != vals.length())
			throw new InvalidParameterException();
		for(int i = 0;i < this.mapping.length;i++) {
			this.mapping[i] = '0';
		}
		for(int i = 0;i < indx.length();i++) {
			this.mapping[indx.charAt(i)-'A'] = vals.charAt(i);
		}
	}
	
	public SoundexKeyGen(boolean isRefinedAlgo) {
		this.isRefinedAlgo = isRefinedAlgo;
		if(isRefinedAlgo) {
			initMap("BPFVCKSGJQXZDTLMNR", "112233344555667889");
		} else {
			initMap("BPFVCSKGJQXZDTLMNR", "111122222222334556");
		}
	}
	
	private boolean isVowel(char c) {
		return (c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U');
	}
	
	private boolean isRestricted(char c) {
		if(c >= 'A' && c <= 'Z')
			return false;
		return true;
	}
	
	public String keygen(String val) {
		if(val == null)
			return null;
		if(val.length() == 0)
			return "";
		char[] tmpArray = val.toUpperCase().toCharArray();
		currentKeyArray = new char[tmpArray.length];
		iKey = 0;
		char groupId;
		for(int i = 0;i < tmpArray.length;i++) {
			if(isRestricted(tmpArray[i])) {
				continue;
			}
			if(iKey == 0) {
				currentKeyArray[iKey++] = tmpArray[i];
				continue;
			} else if(tmpArray[i] == 'H' || tmpArray[i] == 'W') {
				continue;
			} else {
				groupId = mapping[tmpArray[i]-'A'];
				if(!isRefinedAlgo && groupId == '0')
					continue;
				if(currentKeyArray[iKey-1] != groupId) {
					currentKeyArray[iKey++] = groupId;
				}
			}
		}
		return new String(currentKeyArray, 0, (!isRefinedAlgo && iKey > 4) ? 4 : iKey);
	}

}
