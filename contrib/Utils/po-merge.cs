using System;
using System.Collections;
using System.IO;
using System.Text;

class Program
{
    private static void Usage() {
        Console.WriteLine("Usage:\n\tpo-merge update.po old.po new.po\n" +
                "\n" +
                "This program will replace the translation in old.po file with " +
                "update.po file, write the result to new.po file.\n\n" +
                "Only msgstr in old.po file be modified, msgid become unwound. " +
                "Strings(msgstr) that are not found in the update.po are left untouched.\n"
                );
    }

    public static void Main(String[] args) {
        if(args.Length != 3) {
            Usage();
            return;
        }

        Hashtable msg = ParseTranslation(args[0]);

        UpdateTranslation(args[1], args[2], msg);
    }

    private static Hashtable ParseTranslation(String filename) {
        Hashtable msg = new Hashtable(2047);
        StreamReader sr = new StreamReader(filename, Encoding.UTF8);
        int n = 0;
        bool isFinished = false;
        String line, msgid, msgstr;

        while (true) {
            while (true) {
                n++;
                line = sr.ReadLine();
                if (line == null) {
                    isFinished = true;
                    break;
                }

                // May be non-empty msgid ?
                line = line.Trim();

                if (line.Length >= 8 && line[0] != '#')
                    break;
            }

            if (isFinished)
                break;

            if (!line.StartsWith("msgid \"") || (line[line.Length - 1] != '\"')) {
                Console.Error.WriteLine("parse error before msgid, line " + n + ":\n\t" + line);
                throw new Exception("parse error");
            }

            // Get msgid
            if (line.Length <= 8) {
                msgid = "";
            } else {
                msgid = line.Substring(7, line.Length - 8);
            }

            while (true) {
                n++;
                line = sr.ReadLine();
                if (line == null) {
                    Console.Error.WriteLine("parse msgid error, line " + n + ":\n\t" + line);
                    throw new Exception("parse error");
                }

                line = line.Trim();
                if (line.Length == 0 || line[0] != '"')
                    break;

                msgid += line.Substring(1, line.Length - 2);
            }

            if (!line.StartsWith("msgstr \"") || (line[line.Length - 1] != '\"')) {
                Console.Error.WriteLine("parse error after msgid, line " + n + ":\n\t" + line);
                throw new Exception("parse error");
            }

            // Get msgstr
            if (line.Length <= 9) {
                msgstr = "";
            } else {
                msgstr = line.Substring(8, line.Length - 9);
            }

            while (true) {
                n++;
                line = sr.ReadLine();
                if (line == null) break;

                line = line.Trim();
                if (line.Length == 0 || line[0] != '"')
                    break;

                if (msgid.Length == 0)
                    msgstr += "\"\n\"";

                msgstr += line.Substring(1, line.Length - 2);
            }

            msg[msgid] = msgstr;

            if (line == null)
                break;

            if (line.Length != 0 && line[0] != '#') {
                Console.Error.WriteLine("parse error after msgstr, line " + n + ":\n\t" + line);
                throw new Exception("parse error");
            }
        }

        sr.Close();

        return msg;
    }

    private static void UpdateTranslation(String filename, String newfilename, Hashtable msg) {
        StreamReader sr = new StreamReader(filename, Encoding.UTF8);
        StreamWriter sw = new StreamWriter(newfilename, false);

        int n = 0, total = 0, tr = 0, ut = 0, mt = 0;
        bool isFinished = false;
        String line, msgid, msgstr, str = "";

        while (true) {
            while (true) {
                n++;
                line = sr.ReadLine();
                if (line == null) {
                    isFinished = true;
                    break;
                }

                str = line.Trim();

                // May be non-empty msgid ?
                if (str.Length >= 8 && str[0] != '#')
                    break;

                if(total == 0 && str.Equals("#, fuzzy"))
                    line = ""; // Remove PO file header fuzzy

                sw.WriteLine(line);
            }

            if (isFinished) break;

            if (!str.StartsWith("msgid \"") || (str[str.Length - 1] != '\"')) {
                Console.Error.WriteLine("parse error before msgid, line " + n + ":\n\t" + line);
                throw new Exception("parse error");
            }

            // Get msgid
            if (str.Length <= 8) {
                msgid = "";
            } else {
                msgid = str.Substring(7, str.Length - 8);
            }

            while (true) {
                n++;
                line = sr.ReadLine();
                if (line == null)
                {
                    Console.Error.WriteLine("parse msgid error, line " + n + ":\n\t" + line);
                    throw new Exception("parse error");
                }

                str = line.Trim();

                if (str.Length == 0 || line[0] != '"')
                    break;

                msgid += str.Substring(1, str.Length - 2);
            }

            if (!str.StartsWith("msgstr \"") || (str[str.Length - 1] != '\"')) {
                Console.Error.WriteLine("parse error after msgid, line " + n + ":\n\t" + line);
                throw new Exception("parse error");
            }

            // Get msgstr
            if (str.Length <= 9) {
                msgstr = "";
            } else {
                msgstr = str.Substring(8, str.Length - 9);
            }

            while (true) {
                n++;
                line = sr.ReadLine();
                if (line == null)
                    break;

                str = line.Trim();

                if (str.Length == 0 || str[0] != '"')
                    break;

                if (msgid.Length == 0)
                    msgstr += "\"\n\"";

                msgstr += str.Substring(1, str.Length - 2);
            }

            if(msgid.Length != 0)
                total++;

            str = (String) msg[msgid];

            if (str.Length > 0) {
                if(msgid.Length != 0) {
                    mt ++;
                    tr ++;
                }
                sw.WriteLine("msgid \"" + msgid + "\"");
                sw.WriteLine("msgstr \"" + str + "\"\n");
            } else {
                if(msgstr.Length == 0) {
                    ut ++;
                } else {
                    tr++;
                }
                sw.WriteLine("msgid \"" + msgid + "\"");
                sw.WriteLine("msgstr \"" + msgstr + "\"\n");
            }

            if (line == null) {
                break;
            }

            if (line.Length != 0 && line[0] != '#') {
                Console.Error.WriteLine("parse error after msgstr, line " + n + ":\n\t" + line);
                throw new Exception("parse error");
            }
        }

        Console.Error.WriteLine("Total " + total + " messages, " + tr + " translated, "
                + mt + " updated, " + ut + " untranslated.");

        sr.Close();
        sw.Close();

        return;
    }
}
