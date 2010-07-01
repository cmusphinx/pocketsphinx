package edu.cmu.pocketsphinx;

import static org.junit.Assert.*;

import org.junit.Test;
import java.net.URL;

public class ConfigTest {
	static {
		System.loadLibrary("pocketsphinx_jni");
	}

	@Test
	public void testConfig() {
		Config c = new Config();
		assertNotNull(c);
	}

	@Test
	public void testConfigString() {
		URL testcfg = getClass().getResource("test.cfg");
		Config c = new Config(testcfg.getFile());
		assertNotNull(c);
		assertEquals(c.getFloat("-beam"), 1e-60, 1e-10);
		assertEquals(c.getString("-hmm"), "/path/to/somewhere");
	}

	@Test
	public void testSetBoolean() {
		Config c = new Config();
		c.setBoolean("-backtrace", true);
		c.setBoolean("-bestpath", false);
		assertTrue(c.getBoolean("-backtrace"));
		assertFalse(c.getBoolean("-bestpath"));
	}

	@Test
	public void testSetInt() {
		Config c = new Config();
		c.setInt("-ceplen", 42);
		assertEquals(c.getInt("-ceplen"), 42);
	}

	@Test
	public void testSetFloat() {
		Config c = new Config();
		c.setFloat("-lw", 2.5);
		assertEquals(c.getFloat("-lw"), 2.5, 0.1);
	}
	
	@Test
	public void testSetString() {
		Config c = new Config();
		c.setString("-cmn", "howdy");
		assertEquals(c.getString("-cmn"), "howdy");
	}

	@Test
	public void testExists() {
		Config c = new Config();
		assertFalse(c.exists("-foo"));
		assertTrue(c.exists("-beam"));
	}
}
