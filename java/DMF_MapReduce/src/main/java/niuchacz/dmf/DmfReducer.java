package niuchacz.dmf;

import java.io.IOException;

import org.apache.hadoop.io.LongWritable;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.mapreduce.Reducer;

public class DmfReducer extends Reducer<LongWritable, Text, LongWritable, Text> {

	protected void reduce(LongWritable key, Iterable<Text> values, Context context)
			throws IOException, InterruptedException {
		for(Text value : values) {
			context.write(key, value);
		}
	}
}
