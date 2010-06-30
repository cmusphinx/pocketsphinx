import edu.cmu.pocketsphinx.Vector;

public class test {
    static {
	System.loadLibrary("test");
    }

    public static void main(String argv[]) {
	Vector a = new Vector(1,2,3);
	Vector b = new Vector(2,3,4);
	a.add(b);
	System.out.println(a.getX() + " " + a.getY() + " " + a.getZ());
    }
}