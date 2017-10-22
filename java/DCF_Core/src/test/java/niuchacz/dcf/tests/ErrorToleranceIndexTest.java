package niuchacz.dcf.tests;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;
import niuchacz.dcf.core.eti.ErrorToleranceIndex;
import niuchacz.dcf.core.eti.FuzzyResult;

public class ErrorToleranceIndexTest extends TestCase {

	ErrorToleranceIndex index = new ErrorToleranceIndex(2);
	
    public ErrorToleranceIndexTest(String i)
    {
        super(i);
    	index.insert("abcdef");
    	index.insert("abcdf");
    }

    public static Test suite()
    {
        return new TestSuite(ErrorToleranceIndexTest.class);
    }

    public void testApp1()
    {
    	FuzzyResult result;
    	result = index.fetchScored("abcd", 1, 0.2, 2);
    	assertTrue(result.text.equals("abcdf"));
    }

    public void testApp2()
    {
    	FuzzyResult result;
    	result = index.fetchScored("abcde", 1, 0.2, 2);
    	assertTrue(result.text.equals("abcdef"));
    }

    public void testApp3()
    {
    	FuzzyResult result;
    	result = index.fetchScored("abcdf", 1, 0.2, 2);
    	assertTrue(result.text.equals("abcdf"));
    }

    public void testApp4()
    {
    	FuzzyResult result;
    	result = index.fetchScored("abcdef", 1, 0.2, 2);
    	assertTrue(result.text.equals("abcdef"));
    }

    public void testApp5()
    {
    	FuzzyResult result;
    	result = index.fetchScored("abcdefa", 1, 0.2, 2);
    	assertTrue(result.text.equals("abcdef"));
    }

}
