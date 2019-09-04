CREATE OR REPLACE AND RESOLVE JAVA SOURCE NAMED JFILE AS
package tools;
import java.io.*;
import oracle.jdbc.*;
import java.sql.*;
import java.util.*;
import java.math.*;

public class File {

  private static Connection con;
  
  private static Object[] oFileType = new Object[10];

  static {
    try {
      con = DriverManager.getConnection("jdbc:default:connection:");
    } catch (Exception e) {
      e.printStackTrace(System.out);
    }
  }
  
  public static Array getFileList(String pDir) throws Exception {
    List<Struct> list = new ArrayList<Struct>();
    java.io.File[] fileList = new java.io.File(pDir).listFiles();
    if(fileList != null) {
      for(int i=0;i<fileList.length;i++) {
        list.add(convertToStruct(fileList[i]));
      }
    }
    OracleConnection ocon = (OracleConnection) con;
    return ocon.createARRAY("T_FILES", list.toArray());
  }
  
  public static Struct getFile(String pFilePath) throws Exception {
    java.io.File f = new java.io.File(pFilePath);
    return convertToStruct(f);
  }
  
  private static Struct convertToStruct(java.io.File f) throws Exception {
    oFileType[0] = f.getAbsolutePath();
    oFileType[1] = null;
    oFileType[2] = null;
    oFileType[3] = "N";
    oFileType[4] = null;
    oFileType[5] = null;
    oFileType[6] = null;
    oFileType[7] = null;
    oFileType[8] = null;
    try {
      if (f.exists()) {
        oFileType[1] = f.getName();
        oFileType[2] = new BigDecimal(f.length());
        oFileType[3] = "Y";
        oFileType[4] = new java.sql.Timestamp(f.lastModified());
        oFileType[5] = (f.isDirectory()?"Y":"N");
        try {
          oFileType[6] = (f.canWrite()?"Y":"N");
        } catch (SecurityException e) {
          oFileType[6] = "N";
        }
        try {
          oFileType[7] = (f.canRead()?"Y":"N");
        } catch (SecurityException e) {
          oFileType[7] = "N";
        }
        try {
          oFileType[8] = (f.canExecute()?"Y":"N");
        } catch (SecurityException e) {
          oFileType[8] = "N";
        }
      }
    } catch (java.security.AccessControlException e) {
    }
    oFileType[9] = f.hashCode();
    return con.createStruct("T_FILE", oFileType);
  }
}
