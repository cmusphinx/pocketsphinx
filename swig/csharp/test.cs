using System;
using System.IO;
using Pocketsphinx;

public class Test
{
   public static void Main()
   {
	Config c = Decoder.DefaultConfig();
        c.SetString("-hmm", "../../model/en-us/en-us");
        c.SetString("-lm", "../../model/en-us/en-us.lm.bin");
        c.SetString("-dict", "../../model/en-us/cmudict-en-us.dict");
	Decoder d = new Decoder(c);
	
	byte[] data = File.ReadAllBytes("../../test/data/goforward.raw");
	d.StartUtt();
	d.ProcessRaw(data, data.Length, false, false);
	d.EndUtt();
	
	Console.WriteLine("Result is '{0}'", d.Hyp().Hypstr);
	
	foreach (Segment s in d.Seg()) {
	    Console.WriteLine(s);
	}
   }
}

