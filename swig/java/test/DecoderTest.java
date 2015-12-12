package test;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.URL;
import java.nio.*;

import edu.cmu.pocketsphinx.Decoder;
import edu.cmu.pocketsphinx.Config;
import edu.cmu.pocketsphinx.Segment;
import edu.cmu.pocketsphinx.Hypothesis;
import edu.cmu.pocketsphinx.SegmentIterator;

public class DecoderTest {
    static {
        System.loadLibrary("pocketsphinx_jni");
    }

    public static void main(String args[]) throws IOException {
        Config c = Decoder.defaultConfig();
        c.setString("-hmm", "../../model/en-us/en-us");
        c.setString("-lm", "../../model/en-us/en-us.lm.bin");
        c.setString("-dict", "../../model/en-us/cmudict-en-us.dict");
        Decoder d = new Decoder(c);

        FileInputStream ais = new FileInputStream(new File("../../test/data/goforward.raw"));

        d.startUtt();
        d.setRawdataSize(300000);
        byte[] b = new byte[4096];
        int nbytes;
        while ((nbytes = ais.read(b)) >= 0) {
            ByteBuffer bb = ByteBuffer.wrap(b, 0, nbytes);
            bb.order(ByteOrder.LITTLE_ENDIAN);
            short[] s = new short[nbytes/2];
            bb.asShortBuffer().get(s);
            d.processRaw(s, nbytes/2, false, false);
        }
        d.endUtt();
        System.out.println(d.hyp().getHypstr());

        short[] data = d.getRawdata();
        System.out.println("Data size: " + data.length);
        DataOutputStream dos = new DataOutputStream(new FileOutputStream(new File("/tmp/test.raw")));
	for (int i = 0; i < data.length; i++) {
	    dos.writeShort(data[i]);
	}
	dos.close();

        for (Segment seg : d.seg()) {
    	    System.out.println(seg.getWord());
        }
    }
}
