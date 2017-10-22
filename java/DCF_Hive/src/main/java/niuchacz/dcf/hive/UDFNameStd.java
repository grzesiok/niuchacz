package niuchacz.dcf.hive;

import org.apache.hadoop.hive.ql.exec.UDF;
import org.apache.hadoop.hive.serde2.objectinspector.primitive.JavaStringObjectInspector;
import org.apache.hadoop.hive.serde2.objectinspector.primitive.PrimitiveObjectInspectorFactory;
import org.apache.hadoop.io.Text;

import com.creditsafe.dcf.stdtool.NameStdTool;
import com.creditsafe.dcf.stdtool.NameStdTool.Result;

public class UDFNameStd extends UDF {
	
	private JavaStringObjectInspector stringInspector;
    private NameStdTool stdTool = new NameStdTool();

	public Text evaluate(Text input) {
		if(input == null) return null;
		stringInspector = PrimitiveObjectInspectorFactory.javaStringObjectInspector;
		String str = stringInspector.getPrimitiveJavaObject(input);
		Result result = stdTool.split(str);
	    return new Text(result.toString());
	}

}
