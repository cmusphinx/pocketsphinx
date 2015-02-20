package test;

import static org.junit.Assert.*;

import org.junit.Test;

import java.io.File;
import java.io.FileOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.URL;
import javax.sound.sampled.*;
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

    @Test
    public void testDecodeRaw() throws IOException, UnsupportedAudioFileException {
        Config c = Decoder.defaultConfig();
        c.setString("-hmm", "../../model/en-us/en-us");
        c.setString("-lm", "../../model/en-us/en-us.lm.dmp");
        c.setString("-dict", "../../model/en-us/cmudict-en-us.dict");
        Decoder d = new Decoder(c);
        AudioInputStream ais = null;

        URL testwav = new URL("file:../../test/data/goforward.wav");
        AudioInputStream tmp = AudioSystem.getAudioInputStream(testwav);
        // Convert it to the desired audio format for PocketSphinx.
        AudioFormat targetAudioFormat =
            new AudioFormat((float)c.getFloat("-samprate"),
                                16, 1, true, true);
        ais = AudioSystem.getAudioInputStream(targetAudioFormat, tmp);

        d.startUtt();
        d.setRawdataSize(300000);
        byte[] b = new byte[4096];
        try {
            int nbytes;
            while ((nbytes = ais.read(b)) >= 0) {
                ByteBuffer bb = ByteBuffer.wrap(b, 0, nbytes);
                short[] s = new short[nbytes/2];
                bb.asShortBuffer().get(s);
                d.processRaw(s, nbytes/2, false, false);
            }
        } catch (IOException e) {
            fail("Error when reading goforward.wav" + e.getMessage());
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
