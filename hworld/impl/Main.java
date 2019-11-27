import java.util.Scanner;

class Main {
	public static void main(String[] args) {
		interpreter();
	}
	public static void interpreter() {
		String cmnd = "";
		Scanner scan = new Scanner(System.in);
		do {
			cmnd = scan.nextLine();
			if(cmnd.equalsIgnoreCase("h")) {
				System.out.println("Hello!");
			} else if (cmnd.equalsIgnoreCase("hh")) {
				System.out.println("Hello, Hello!");
			} else if(cmnd.equalsIgnoreCase("w")) {
				System.out.println("World!");
			} else if(cmnd.equalsIgnoreCase("ww")) {
				System.out.println("World, World!");
			} else if(cmnd.equalsIgnoreCase("wh")) {
				System.out.println("World, Hello!");
			} else if(cmnd.equalsIgnoreCase("hw")) {
				System.out.println("Hello, World!");
			} else {
				System.out.println(cmnd);
			} 
		} while (!cmnd.equalsIgnoreCase("q"));
		scan.close();
	}
}
