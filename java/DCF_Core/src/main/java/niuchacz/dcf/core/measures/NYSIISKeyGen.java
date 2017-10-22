package niuchacz.dcf.core.measures;

public class NYSIISKeyGen implements KeyGen {
	/*
	 * NYSIIS - New York State Identification and Intelligence System
	 */
	
	private char[] valArray;
	private char[] currentKeyArray;
	private int iKey;
	private int iVal;
	private int iValMax;
	
	private final boolean isTrueNYSIIS;
	
	public NYSIISKeyGen(boolean isTrueNYSIIS) {
		this.isTrueNYSIIS = isTrueNYSIIS;
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
		valArray = new char[tmpArray.length];
		iValMax = 0;
		for(int i = 0;i < tmpArray.length;i++) {
			if(!isRestricted(tmpArray[i])) {
				valArray[iValMax++] = tmpArray[i];
			}
		}
		if(iValMax == 0)
			return "";
		currentKeyArray = new char[iValMax];
		doStep1();
		doStep2();
		doStep3();
		doStep4();
		doStep5();
		return new String(currentKeyArray, 0, (isTrueNYSIIS && iKey > 6) ? 6 : iKey);
	}
	
	private void doStep1() {
		iVal = 0;
		//Translate first characters of name: MAC → MCC, KN → N, K → C, PH, PF → FF, SCH → SSS
		if(valArray[0] == 'M' && valArray[1] == 'A' && valArray[2] == 'C') {
			valArray[0] = 'M';
			valArray[1] = 'C';
			valArray[2] = 'C';
		} else if(valArray[0] == 'K' && valArray[1] == 'N') {
			valArray[0] = 'N';
			valArray[1] = 'N';
		} else if(valArray[0] == 'K') {
			valArray[0] = 'C';
		} else if(valArray[0] == 'P' && valArray[1] == 'H'
				|| valArray[0] == 'P' && valArray[1] == 'F') {
			valArray[0] = 'F';
			valArray[1] = 'F';
		} else if(valArray[0] == 'S' && valArray[1] == 'C' && valArray[2] == 'H') {
			valArray[0] = 'S';
			valArray[1] = 'S';
			valArray[2] = 'S';
		}
	}
	
	private void doStep2() {
		//Translate last characters of name: EE → Y, IE → Y, DT, RT, RD, NT, ND → D
		if(valArray[valArray.length-2] == 'E' && valArray[valArray.length-1] == 'E'
				|| valArray[valArray.length-2] == 'I' && valArray[valArray.length-1] == 'E') {
			valArray[valArray.length-2] = 'Y';
			iValMax = valArray.length-1;
		} else if(valArray[valArray.length-2] == 'D' && valArray[valArray.length-1] == 'T'
				|| valArray[valArray.length-2] == 'R' && valArray[valArray.length-1] == 'T'
				|| valArray[valArray.length-2] == 'R' && valArray[valArray.length-1] == 'D'
				|| valArray[valArray.length-2] == 'N' && valArray[valArray.length-1] == 'T'
				|| valArray[valArray.length-2] == 'N' && valArray[valArray.length-1] == 'D') {
			valArray[valArray.length-2] = 'D';
			iValMax = valArray.length-1;
		}
	}
	
	private void doStep3() {
		//First character of key = first character of name.
		iKey = 0;
		currentKeyArray[iKey++] = valArray[iVal++];
	}
	
	private void doStep4() {
		/*
		 * Translate remaining characters by following rules, incrementing by one character each time:
		 * 1. EV → AF else A, E, I, O, U → A
		 * 2. Q → G, Z → S, M → N
		 * 3. KN → N else K → C
		 * 4. SCH → SSS, PH → FF
		 * 5. H → If previous or next is non-vowel, previous.
		 * 6. W → If previous is vowel, A.
		 * 7. Add current to key if current is not same as the last key character.
		 */
		while(iVal < iValMax) {
			if(iVal+1 < iValMax && valArray[iVal] == 'E' && valArray[iVal+1] == 'V') {
				valArray[iVal] = 'A';
				valArray[iVal+1] = 'F';
			} else if(isVowel(valArray[iVal])) {
				valArray[iVal] = 'A';
			} else if(valArray[iVal] == 'Q') {
				valArray[iVal] = 'G';
			} else if(valArray[iVal] == 'Z') {
				valArray[iVal] = 'S';
			} else if(valArray[iVal] == 'M') {
				valArray[iVal] = 'N';
			} else if(iVal+1 < iValMax && valArray[iVal] == 'K' && valArray[iVal+1] == 'N') {
				valArray[++iVal] = 'N';
			} else if(valArray[iVal] == 'K') {
				valArray[iVal] = 'C';
			} else if(iVal+2 < iValMax && valArray[iVal] == 'S' && valArray[iVal+1] == 'C' && valArray[iVal+2] == 'H') {
				valArray[iVal] = 'S';
				valArray[iVal+1] = 'S';
				valArray[iVal+2] = 'S';
			} else if(iVal+1 < iValMax && valArray[iVal] == 'P' && valArray[iVal+1] == 'H') {
				valArray[iVal] = 'F';
				valArray[iVal+1] = 'F';
			} else if(valArray[iVal] == 'H' && (iVal > 0 && !isVowel(valArray[iVal-1]) || iVal+1 < iValMax && !isVowel(valArray[iVal+1]))) {
				valArray[iVal] = valArray[iVal-1];
			} else if(valArray[iVal] == 'W' && isVowel(valArray[iVal-1])) {
				valArray[iVal] = valArray[iVal-1];
			}
			if(currentKeyArray[iKey-1] != valArray[iVal]) {
				currentKeyArray[iKey++] = valArray[iVal];
			}
			iVal++;
		}
	}
	
	private void doStep5() {
		//If last character is S, remove it.
		//If last characters are AY, replace with Y.
		//If last character is A, remove it.
		if(iKey > 0 && currentKeyArray[iKey-1] == 'S') {
			currentKeyArray[iKey-1] = '\0';
			iKey--;
		} else if(iKey > 1 && currentKeyArray[iKey-2] == 'A' && currentKeyArray[iKey-1] == 'Y') {
			currentKeyArray[iKey-2] = 'Y';
			currentKeyArray[iKey-1] = '\0';
			iKey--;
		} else if(iKey > 0 && currentKeyArray[iKey-1] == 'A') {
			currentKeyArray[iKey-1] = '\0';
			iKey--;
		}
	}

}
