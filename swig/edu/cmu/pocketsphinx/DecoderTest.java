package edu.cmu.pocketsphinx;

import static org.junit.Assert.*;

import org.junit.Test;
import java.net.URL;
import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioInputStream;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.UnsupportedAudioFileException;
import javax.sound.sampled.AudioFormat.Encoding;


public class DecoderTest {
	static {
		System.loadLibrary("pocketsphinx_jni");
	}

	@Test
	public void testDecoder() {
		Decoder d = new Decoder();
		assertNotNull(d);
	}

	@Test
	public void testDecoderConfig() {
		Config c = new Config();
		c.setBoolean("-backtrace", true);
		Decoder d = new Decoder(c);
		assertNotNull(d);
	}

	@Test
	public void testGetConfig() {
		Decoder d = new Decoder();
		Config c = d.getConfig();
		assertNotNull(c);
		assertEquals(c.getString("-lmname"), "default");
	}
	
	@Test
	public void testDecodeRaw() {
		Decoder d = new Decoder();
		Config c = d.getConfig();
		URL testwav = getClass().getResource("goforward.wav");
		AudioInputStream ais = AudioSystem.getAudioInputStream(testwav);
		/* Convert it to the desired audio format for PocketSphinx. */
		AudioFormat targetAudioFormat = new AudioFormat(c.getInt("-samprate"),
														16, 1, true, true);
	}

}
