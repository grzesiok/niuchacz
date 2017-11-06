package niuchacz.dmf;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.io.LongWritable;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.mapreduce.lib.input.FileInputFormat;
import org.apache.hadoop.mapreduce.lib.output.FileOutputFormat;
import org.apache.hadoop.mapreduce.Job;

public class DmfDriver {

	public static void main(String[] args) throws Exception {
		if (args.length != 2) {
			for(int i = 0;i < args.length;i++) {
				System.err.println("arg["+i+"]="+args[i]);
			}
			System.err.println("Usage: dmf <input path> <output path>");
			System.exit(-1);
		}

		// Create the job specification object
		Job job = Job.getInstance(new Configuration(), "Data Matching Framework");
		
	    // set jar
	    job.setJarByClass(DmfDriver.class);

		// Setup input and output paths
		FileInputFormat.addInputPath(job, new Path(args[0]));
		FileOutputFormat.setOutputPath(job, new Path(args[1]));

		// Set the Mapper and Reducer classes
		job.setMapperClass(DmfMapper.class);
		job.setReducerClass(DmfReducer.class);

		// Specify the type of output keys and values
		job.setOutputKeyClass(LongWritable.class);
		job.setOutputValueClass(Text.class);

		// job.setInputFormat(SequenceFileInputFormat.class);
		// job.setOutputFormat(SequenceFileOutputFormat.class);

		// Wait for the job to finish before terminating
		System.exit(job.waitForCompletion(true) ? 0 : 1);
	}
}
