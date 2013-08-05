﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Reflection;
using Microsoft.SqlServer.Server;
using Jhu.SqlServer.Array;

namespace GenerateInstallScript
{
    class Program
    {
        static void Main(string[] args)
        {
            Console.WriteLine("Generating install/uninstall scripts...");

            // Command line arguments: 0: Create script
            //                         1: Drop script

            string createfilename = args[0];
            string dropfilename = args[1];

            CreateDirectory(createfilename);
            CreateDirectory(dropfilename);

            // Open output file
            using (StreamWriter createScript = new StreamWriter(createfilename))
            {
                using (StreamWriter dropScript = new StreamWriter(dropfilename))
                {
                    // Open assembly for reflection
                    Assembly a = typeof(SqlArrayAttribute).Assembly;

                    ScriptHeader(createScript, dropScript);
                    ScriptCreateAssembly(createScript, a);
                    ScriptCreateUDTs(createScript);
                    EnumerateTypes(createScript, dropScript, a);
                    ScriptDropUDTs(dropScript);
                    ScriptDropAssembly(dropScript, a);
                }
            }
        }

        static void CreateDirectory(string filename)
        {
            string dir = Path.GetDirectoryName(Path.GetFullPath(filename));

            if (dir != "" && !Directory.Exists(dir))
            {
                Directory.CreateDirectory(dir);
            }
        }

        static void ScriptHeader(StreamWriter createScript, StreamWriter dropScript)
        {
            createScript.WriteLine("-- Install assembly and debug symbols");
            createScript.WriteLine("-- {0}", DateTime.Now);

            dropScript.WriteLine("-- Drop assembly and debug symbols");
            dropScript.WriteLine("-- {0}", DateTime.Now);
        }

        static void ScriptCreateAssembly(StreamWriter createScript, Assembly a)
        {
            foreach (AssemblyName an in a.GetReferencedAssemblies())
            {
                if (an.Name.StartsWith("Jhu."))
                {
                    Assembly aa = Assembly.Load(an);
                    ScriptCreateAssembly(createScript, aa.Location);
                }
            }

            ScriptCreateAssembly(createScript, a.Location);
        }

        static void ScriptCreateAssembly(StreamWriter createScript, string filename)
        {
            createScript.WriteLine(@"
CREATE ASSEMBLY [{0}]
AUTHORIZATION [dbo]
FROM {1}
{2};
GO            
            ",
             Path.GetFileNameWithoutExtension(filename),
             GetFileInHex(filename),
             Path.GetFileNameWithoutExtension(filename) == "Jhu.SqlServer.Array" ? "WITH PERMISSION_SET = UNSAFE" : ""
             );

            string pdbname = filename.Replace(".dll", ".pdb");

            createScript.WriteLine(@"
ALTER ASSEMBLY [{0}]
WITH UNCHECKED DATA	
ADD FILE FROM {1} AS '{2}';
GO
            ",
             Path.GetFileNameWithoutExtension(filename),
             GetFileInHex(pdbname), 
             //"'" + pdbname + "'", 
             Path.GetFileName(pdbname));

        }

        static void ScriptDropAssembly(StreamWriter dropScript, Assembly a)
        {

            ScriptDropAssembly(dropScript, a.Location);

            foreach (AssemblyName an in a.GetReferencedAssemblies())
            {
                if (an.Name.StartsWith("Jhu."))
                {
                    Assembly aa = Assembly.Load(an);
                    ScriptDropAssembly(dropScript, aa.Location);
                }
            }


        }

        static void ScriptDropAssembly(StreamWriter dropScript, string filename)
        {
            dropScript.WriteLine(@"
DROP ASSEMBLY [{0}];
GO
            ", Path.GetFileNameWithoutExtension(filename));
        }

        static void EnumerateTypes(StreamWriter createScript, StreamWriter dropScript, Assembly a)
        {
            HashSet<string> schemas = new HashSet<string>();

            foreach (Type t in a.GetTypes())
            {
                SqlArrayAttribute saattr =
                    (SqlArrayAttribute)t.GetCustomAttributes(typeof(SqlArrayAttribute), false).FirstOrDefault();

                if (saattr != null)
                {
                    // Create schema for the functions
                    string schemaname = GetSchemaName(t.Name);

                    if (!schemas.Contains(schemaname))
                    {
                        createScript.WriteLine(@"
CREATE SCHEMA {0};
GO
                    ", schemaname);    // trim 'Sql' at the beginning of type name

                        schemas.Add(schemaname);
                    }

                    EnumerateMethods(createScript, dropScript, t);
                }

                SqlArrayTypeConverterAttribute acattr =
                    (SqlArrayTypeConverterAttribute)t.GetCustomAttributes(typeof(SqlArrayTypeConverterAttribute), false).FirstOrDefault();

                if (acattr != null)
                {
                    EnumerateMethods(createScript, dropScript, t);
                }

                //SqlUserDefinedAggregateAttribute aggattr =
                //    (SqlUserDefinedAggregateAttribute)t.GetCustomAttributes(typeof(SqlUserDefinedAggregateAttribute), false).FirstOrDefault();


                bool isaggregate = false;
                int attrsize = 0;
                string attrname = null;
                foreach (CustomAttributeData d in t.GetCustomAttributesData())
                {
                    if (d.Constructor.DeclaringType == typeof(SqlUserDefinedAggregateAttribute))
                    {
                        isaggregate = true;

                        foreach (CustomAttributeNamedArgument na in d.NamedArguments)
                        {
                            if (na.MemberInfo.Name == "MaxByteSize")
                            {
                                attrsize = (int)na.TypedValue.Value;
                            }
                            if (na.MemberInfo.Name == "Name")
                            {
                                attrname = (string)na.TypedValue.Value;
                            }
                        }
                    }
                }

                if (isaggregate)
                {
                    string schemaname = GetSchemaName(t.Name);

                    if (!schemas.Contains(schemaname))
                    {
                        createScript.WriteLine(@"
CREATE SCHEMA {0};
GO
                    ", schemaname);    // trim 'Sql' at the beginning of type name

                        schemas.Add(schemaname);
                    }


                    ScriptAggregate(attrname, t, createScript, dropScript);
                }
            }

            foreach (string schemaname in schemas)
            {
                dropScript.WriteLine(@"
DROP SCHEMA {0};
GO
                    ", schemaname);
            }
        }

        static void ScriptCreateUDTs(StreamWriter createScript)
        {
/*            createScript.WriteLine(@"
CREATE TYPE dbo.SingleComplex
EXTERNAL NAME [Jhu.SqlServer.Array].[Jhu.SqlServer.Array.SqlSingleComplex];
GO

CREATE TYPE dbo.DoubleComplex
EXTERNAL NAME [Jhu.SqlServer.Array].[Jhu.SqlServer.Array.SqlDoubleComplex];
GO");*/

            createScript.WriteLine(@"
CREATE TYPE dbo.RealComplex
EXTERNAL NAME [Jhu.SqlServer.Array].[Jhu.SqlServer.Array.SqlRealComplex];
GO

CREATE TYPE dbo.FloatComplex
EXTERNAL NAME [Jhu.SqlServer.Array].[Jhu.SqlServer.Array.SqlFloatComplex];
GO");

        }

        static void ScriptDropUDTs(StreamWriter dropScript)
        {
            /*dropScript.WriteLine(@"
DROP TYPE dbo.SingleComplex
GO

DROP TYPE dbo.DoubleComplex
GO");*/

            dropScript.WriteLine(@"
DROP TYPE dbo.RealComplex
GO

DROP TYPE dbo.FloatComplex
GO");
        }

        static void EnumerateMethods(StreamWriter createScript, StreamWriter dropScript, Type t)
        {
            foreach (MethodInfo m in t.GetMethods())
            {
                SqlFunctionAttribute saattr =
                    (SqlFunctionAttribute)m.GetCustomAttributes(typeof(SqlFunctionAttribute), false).FirstOrDefault();

                if (saattr != null)
                {
                    if (saattr.FillRowMethodName != null)
                    {
                        ScriptTableValuedFunction(createScript, dropScript, m, saattr);
                    }
                    else
                    {
                        ScriptScalarFunction(createScript, dropScript, m, saattr);
                    }
                }
            }
        }

        static void ScriptTableValuedFunction(StreamWriter createScript, StreamWriter dropScript, MethodInfo m, SqlFunctionAttribute attr)
        {
            string schemaname = GetSchemaName(m.DeclaringType.Name);
            string functionname = attr.Name != null ? attr.Name : m.Name;

            createScript.WriteLine(@"
CREATE FUNCTION [{0}].[{1}] ({2})
RETURNS TABLE
( {3} )
AS EXTERNAL NAME [Jhu.SqlServer.Array].[{4}.{5}].[{1}];
GO
",
                schemaname,
                functionname,
                GetParameterList(m, 0, "@"),
                GetTableSchema(m, attr),
                m.DeclaringType.Namespace,
                m.DeclaringType.Name);

            dropScript.WriteLine(@"
DROP FUNCTION [{0}].[{1}];
GO
",
                schemaname,
                functionname);
        }

        static void ScriptScalarFunction(StreamWriter createScript, StreamWriter dropScript, MethodInfo m, SqlFunctionAttribute attr)
        {
            // Figure out function name
            string schemaname = GetSchemaName(m.DeclaringType.Name);
            string functionname = m.Name;
            string sqlfunctionname = attr.Name != null ? attr.Name : GetFunctionName(m.DeclaringType.Name, m.Name);

            createScript.WriteLine(@"
CREATE FUNCTION [{0}].[{1}] ({2})
RETURNS {3}
AS EXTERNAL NAME [Jhu.SqlServer.Array].[{4}.{5}].[{6}];
GO
",
                schemaname,
                sqlfunctionname,
                GetParameterList(m, 0, "@"),
                GetSqlType(m.ReturnType),
                m.DeclaringType.Namespace,
                m.DeclaringType.Name,
                functionname);

            dropScript.WriteLine(@"
DROP FUNCTION [{0}].[{1}];
GO
",
                schemaname,
                sqlfunctionname);
        }

        static void  ScriptAggregate(string name, Type t, StreamWriter createScript, StreamWriter dropScript)
        {
            string schemaname = GetSchemaName(t.Name);
            string sqlfunctionname = name;

            MethodInfo ma = t.GetMethod("Accumulate");
            MethodInfo mt = t.GetMethod("Terminate");

            createScript.WriteLine(@"
CREATE AGGREGATE [{0}].[{1}] ({2})
RETURNS {3}
EXTERNAL NAME [Jhu.SqlServer.Array].[{4}.{5}]
GO",
                schemaname,
                sqlfunctionname,
                GetParameterList(ma, 0, "@"),
                GetSqlType(mt.ReturnType),
                t.Namespace,
                t.Name);


            dropScript.WriteLine(@"
DROP AGGREGATE [{0}].[{1}];
GO
",
                schemaname,
                sqlfunctionname);
        }

        static string GetParameterList(MethodInfo m, int start, string prefix)
        {
            string res = "";

            ParameterInfo[] pars = m.GetParameters();

            for (int i = start; i < pars.Length; i++)
            {
                if (res != "") res += ", ";

                res += String.Format("{0}{1} {2}",
                    prefix,
                    pars[i].Name,
                    GetSqlType(pars[i].ParameterType));
            }

            return res;
        }

        static string GetTableSchema(MethodInfo m, SqlFunctionAttribute attr)
        {
            return GetParameterList(m.DeclaringType.GetMethod(attr.FillRowMethodName), 1, "");
        }

        static string GetSqlType(Type t)
        {
            switch (t.Name)
            {
                case "Byte":
                case "Byte&":
                    return "tinyint";
                case "Sbyte":
                case "SByte&":
                    return "tinyint";
                case "Int16":
                case "Int16&":
                    return "smallint";
                case "Int32":
                case "Int32&":
                    return "int";
                case "Int64":
                case "Int64&":
                    return "bigint";
                case "Single":
                case "Single&":
                    return "real";
                case "Double":
                case "Double&":
                    return "float";
                case "SqlComplex<float>":
                case "SqlComplex<float>&":
                case "SqlSingleComplex":
                case "SqlSingleComplex&":
                    return "SingleComplex";
                case "SqlComplex<double>":
                case "SqlComplex<double>&":
                case "SqlDoubleComplex":
                case "SqlDoubleComplex&":
                    return "DoubleComplex";
                case "SqlRealComplex":
                case "SqlRealComplex&":
                    return "RealComplex";
                case "SqlFloatComplex":
                case "SqlFloatComplex&":
                    return "FloatComplex";
                case "SqlBytes":
                case "SqlBytes&":
                    return "varbinary(max)";
                case "SqlBinary":
                case "SqlBinary&":
                    return "varbinary(8000)";
                case "SqlChars":
                    return "nvarchar(max)";
                default:
                    throw new NotImplementedException();
            }

            /*
            if (t == typeof(byte)) return "tinyint";
            if (t == typeof(byte*)) return "tinyint";
            if (t == typeof(sbyte)) return "tinyint";
            if (t == typeof(Int16)) return "smallint";
            if (t == typeof(Int32)) return "int";
            if (t == typeof(Int64)) return "bigint";
            if (t == typeof(float)) return "real";
            if (t == typeof(double)) return "float";
            if (t == typeof(System.Data.SqlTypes.SqlBytes)) return "varbinary(max)";
            if (t == typeof(System.Data.SqlTypes.SqlBinary)) return "varbinary(8000)";
            if (t == typeof(System.Data.SqlTypes.SqlChars)) return "nvarchar(max)";

            throw new NotImplementedException();*/
        }

        static string GetSchemaName(string classname)
        {
            if (classname.StartsWith("Sql") && classname.EndsWith("Converter"))
            {
                string from, to;
                GetConverterParts(classname, out from, out to);
                return from;
            }
            else if (classname.StartsWith("Sql"))
            {
                int underscore = classname.IndexOf('_');

                if (underscore >= 0)
                {
                    return classname.Substring(3, underscore - 3);
                }
                else
                {
                    return classname.Substring(3);
                }
            }

            /*
            // classname format: Sql...Functions
            if (classname.EndsWith("Functions"))
            {
                return classname.Substring(3).Remove(classname.Length - 12);
            }
            else if (classname.EndsWith("Converter"))
            {
                string from, to;
                GetConverterParts(classname, out from, out to);
                return from;
            }
            else if (classname.EndsWith("Concat"))
            {
                return classname.Substring(3).Remove(classname.Length - 9);
            }
            else if (classname.EndsWith("ConcatSubarrays"))
            {
                return classname.Substring(3).Remove(classname.Length - 18);
            }*/

            throw new NotImplementedException();
        }

        static string GetFunctionName(string classname, string functionname)
        {
            if (classname.EndsWith("Converter"))
            {
                string from, to;
                GetConverterParts(classname, out from, out to);
                return String.Format("ConvertTo{0}", to);
            }
            else
            {
                return functionname;
            }
        }

        static void GetConverterParts(string classname, out string from, out string to)
        {
            int t = classname.IndexOf("To", 3);
            from = classname.Substring(3, t - 3);
            int i = classname.IndexOf("_", t);
            to = classname.Substring(t + 2, i - t - 2);
        }

        static string GetFileInHex(string filename)
        {
            StringBuilder res = new StringBuilder();
            res.Append("0x");

            byte[] buffer = File.ReadAllBytes(filename);
            for (int i = 0; i < buffer.Length; i++)
            {
                res.Append(buffer[i].ToString("X2"));
            }

            return res.ToString();
        }
    }
}
