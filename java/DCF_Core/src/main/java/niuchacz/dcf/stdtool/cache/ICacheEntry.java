package niuchacz.dcf.stdtool.cache;

import java.util.Date;

import niuchacz.dcf.stdtool.cache.ACacheEntry.TokenType;

public interface ICacheEntry {

	public String getAltToken();
	public void setAltToken(String altToken);
	public String getMainToken();
	public void setMainToken(String mainToken);
	public TokenType getTokenType();
	public void setTokenType(TokenType tokenType);
	public Date getChangeDate();
	public void setChangeDate(Date changeDate);
}
