package niuchacz.dcf.tests;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;
import niuchacz.dcf.core.ngram.NgramTokenizer;
import opennlp.tools.tokenize.Tokenizer;

public class NgramTokenizerTest extends TestCase
{
    public NgramTokenizerTest(String i)
    {
        super(i);
    }

    public static Test suite()
    {
        return new TestSuite(NgramTokenizerTest.class);
    }

    public void testApp1()
    {
    	Tokenizer tokenizer = new NgramTokenizer(2);
    	String[] s1 = tokenizer.tokenize("a");
    	assertTrue(s1[0].equals("a"));
    }

    public void testApp2() throws Exception
    {
    	Tokenizer tokenizer = new NgramTokenizer(2);
    	String[] s2 = tokenizer.tokenize("ab");
    	assertTrue(s2[0].equals("ab"));
    }

    public void testApp3() throws Exception
    {
    	Tokenizer tokenizer = new NgramTokenizer(2);
    	String[] s3 = tokenizer.tokenize("abc");
    	assertTrue(s3[0].equals("ab"));
    	assertTrue(s3[1].equals("bc"));
    }
}
