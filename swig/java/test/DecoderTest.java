package test;

import static org.junit.Assert.*;

import org.junit.Test;

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
    public void testDecodeRaw() {
        Config c = Decoder.defaultConfig();
        c.setFloat("-samprate", 8000);
        c.setString("-hmm", "../../model/hmm/en_US/hub4wsj_sc_8k");
        c.setString("-lm", "../../model/lm/en_US/hub4.5000.DMP");
        c.setString("-dict", "../../model/lm/en_US/hub4.5000.dic");
        Decoder d = new Decoder(c);
        AudioInputStream ais = null;
        try {
            URL testwav = new URL("file:../../test/data/wsj/n800_440c0207.wav");
            AudioInputStream tmp = AudioSystem.getAudioInputStream(testwav);
            // Convert it to the desired audio format for PocketSphinx.
            AudioFormat targetAudioFormat =
                new AudioFormat((float)c.getFloat("-samprate"),
                                16, 1, true, true);
            ais = AudioSystem.getAudioInputStream(targetAudioFormat, tmp);
        } catch (IOException e) {
            fail("Failed to open " + e.getMessage());
        } catch (UnsupportedAudioFileException e) {
            fail("Unsupported file type of " + e.getMessage());
        }

        d.startUtt("");
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
        
        for (Segment seg : d.seg()) {
    	    System.out.println(seg.getWord());
        }
    }
}
