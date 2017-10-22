package niuchacz.dcf.stdtool.cache;

import java.util.Date;

public abstract class ACacheEntry implements ICacheEntry {
	
	public interface TokenType {
	}

	private String altToken;
	private String mainToken;
	private TokenType tokenType;
	private Date changeDate;
	
	public ACacheEntry(String altToken, String mainToken, TokenType tokenType, Date changeDate) {
		this.altToken = altToken;
		this.mainToken = mainToken;
		this.tokenType = tokenType;
		this.changeDate = changeDate;
	}
	
	public String getAltToken() {
		return altToken;
	}
	public void setAltToken(String altToken) {
		this.altToken = altToken;
	}
	public String getMainToken() {
		return mainToken;
	}
	public void setMainToken(String mainToken) {
		this.mainToken = mainToken;
	}
	public TokenType getTokenType() {
		return tokenType;
	}
	public void setTokenType(TokenType tokenType) {
		this.tokenType = tokenType;
	}
	public Date getChangeDate() {
		return changeDate;
	}
	public void setChangeDate(Date changeDate) {
		this.changeDate = changeDate;
	}
}
