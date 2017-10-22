package niuchacz.dcf.stdtool.cache;

import java.util.Date;

public class NameCacheEntry extends ACacheEntry {

	public enum NameTokenType implements TokenType {
		Unknown,
		DBASeparator, DBASeparatorWithHWM, DBASeparatorHWM,
		Prepos, Article, LinkWord, Error,
		LegalForm, Special,
		HardStop
		//'REJECT_ROW'
		//'HARD_STOP'
	}
	
	public NameCacheEntry(String altToken, String mainToken, TokenType tokenType, Date changeDate) {
		super(altToken, mainToken, tokenType, changeDate);
	}

}
