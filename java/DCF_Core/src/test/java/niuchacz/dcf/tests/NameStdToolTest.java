package niuchacz.dcf.tests;

import java.util.Iterator;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;
import niuchacz.dcf.stdtool.NameStdTool;
import niuchacz.dcf.stdtool.NameStdTool.Result;

public class NameStdToolTest extends TestCase
{
	private NameStdTool stdTool;
	
	public static class TestResult {
		public String originalName;
		public NameStdTool.Result parsedName;
		
		public TestResult(String originalName, NameStdTool.Result parsedName) {
			this.originalName = originalName;
			this.parsedName = parsedName;
		}
	}
	
	TestResult testVectors[] = {
			new TestResult("LAYNE TEXAS COMPANY", null),
			new TestResult("B J SHEPPARD FURNITURE INC", null),
			new TestResult("IROZ CONSTRUCTION CO", null),
			new TestResult("CORPUS CHRISTI GASKET & PACKING COMPANY INC", null),
			new TestResult("RED CEDAR STEEL ERECTORS INC", null),
			new TestResult("AMERICAN CAN CO", null),
			new TestResult("REPUBLIC STEEL CORP", null),
			new TestResult("PAGE & WIRTZ CONSTRUCTION COMP", null),
			new TestResult("CLARKSVILLE MACHINE WORKS INC", null),
			new TestResult("W. D. LUTCH", null),
			new TestResult("WARSAW CITY OF", null),
			new TestResult("JOSEPH PRESTI FARM", null),
			new TestResult("CITY OF BUENA VISTA", null),
			new TestResult("SUN CONSTRUCTION INC", null),
			new TestResult("KING CHEONG HONG CO.", null),
			new TestResult("BARCLAY BRAND CORP", null),
			new TestResult("NEW JERSEY PORCELAIN CO", null),
			new TestResult("SOUTHWEST FORMING SYSTEM", null),
			new TestResult("HANNER MACHINE CO INC", null),
			new TestResult("CAMBRIDGE WIRE CLOTH CO", null),
			new TestResult("W E ARNOLD GENERAL CONTRACTOR", null),
			new TestResult("PFIZER INC", null),
			new TestResult("MAJESTIC CONSTRUCTION", null),
			new TestResult("LEONARD B HEBERT JR & CO INC", null),
			new TestResult("ECLIPSE SUPPLY CO INC", null),
			new TestResult("CELMET COMPANY INC", null),
			new TestResult("J M FIELDS CO INC STORE #1691", null),
			new TestResult("SUN ELECTRIC CORPORATION", null),
			new TestResult("STONERS INK CO", null),
			new TestResult("MARK VENIERO TRUCKING", null),
			new TestResult("FOLLOW SUN PRODUCTS INC", null),
			new TestResult("GRAY ENVELOPE MFG CO INC", null),
			new TestResult("BRISTOL MFG CORPORAT", null),
			new TestResult("G F CLINGERMAN", null),
			new TestResult("ALLSTATES ELECTRIC", null),
			new TestResult("CERUSSI & VERRI INC MASONS", null),
			new TestResult("CREATIVE PAINTING & DECORATING", null),
			new TestResult("C E MYERS  CO. INC.", null),
			new TestResult("OMEGA HEATER CO INC", null),
			new TestResult("PONCA ELECTRIC INC.", null)
			};
	
    public NameStdToolTest(String i)
    {
        super(i);
        stdTool = new NameStdTool();
    }

    public static Test suite()
    {
        return new TestSuite(NameStdToolTest.class);
    }

    public void testApp1()
    {
    	for(TestResult testVector : testVectors) {
    		System.out.println("ORIGINAL:"+testVector.originalName);
    		NameStdTool.Result parsedData = stdTool.split(testVector.originalName);
    		System.out.println(parsedData);
    	}
    }
}
