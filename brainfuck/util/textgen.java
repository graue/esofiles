//====================================================================
// TEXTGEN -- BrainF*** Genetic Text Generator
// textgen.java
// Copyright (C) 2003, 2004 by Jeffry Johnston
// Version 0.20
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation. See the file LICENSE
// for more details.
//
// Online resource used:
// http://www.genetic-programming.com/gpflowchart.html
//
// I have not read any books and have seen very little information
// online about GP implementation.  Any suggestions are most welcome.
//
// Inspiration:
// 111-byte "Hello World!" with newline (assumed optimal)
// ++++++++++[>+++++++>++++++++++>+++>+<<<<-]>++.>+.+++++++..+++.>++.
// <<+++++++++++++++.>.+++.------.--------.>+.>.
//
// Version history:
// 0.20    11 Aug 2004 Added -i and -o options
// 0.10    01 Dec 2003 Initial release
//====================================================================

import java.io.*;
import java.util.*;

//********************************************************************
// textgen
//********************************************************************
public class textgen {
  static final String VERSION = "0.20";
  static Random r = new Random();

  //------------------------------------------------------------------
  // main
  //------------------------------------------------------------------
  public static void main(String[] cmdline) {
    int terms = 4, population = 500, generations = 0;
    String text = "Hello World!\n";
    double toppercent = 0.10;
    String fileout = null;

    // display copyright text
    System.out.println("TEXTGEN BrainF*** Genetic Text Generator, version " + VERSION + ".");
    System.out.println("Copyright (c) 2003 Jeffry Johnston.");
    System.out.println();

    // process command line
    for (int n = 0; ; n++) {
      try {
        if (cmdline[n].equals("-?") || cmdline[n].equals("/?") || cmdline[n].equals("--?")
            || cmdline[n].equals("--help") ) {
          usage();
          System.exit(0);
        } else if (cmdline[n].equalsIgnoreCase("-i")) {
          // open file
          DataInputStream filein = null;
          try {
            filein = new DataInputStream(new FileInputStream(cmdline[n + 1]));
          } catch (FileNotFoundException e) {
            System.out.println("Error opening input file.  File not found");
            System.exit(1);
          }
          // read file
          byte[] data = null;
          int size = 0;
          try {
            size = filein.available();
            data = new byte[size];
            filein.readFully(data);
          } catch (IOException e) {
            System.out.println("Error reading input file.");
            System.exit(1);
          }
          // close file
          try {
            filein.close();
          } catch (IOException e) {
            System.out.println("Error closing input file.");
            System.exit(1);
          }
          try {
            text = new String(data, "ISO-8859-1");
          } catch (UnsupportedEncodingException e) {
            System.out.println("This program requires ISO-8859-1 character set support.");
          }
          n++;
        } else if (cmdline[n].equalsIgnoreCase("-o")) {
          fileout = cmdline[n + 1];
          n++;
        } else if (cmdline[n].equalsIgnoreCase("-g")) {
          generations = Integer.parseInt(cmdline[n + 1]);
          if (generations < 1) {
            System.out.println("Must have at least 1 generation.");
            System.exit(1);
          }
          n++;
        } else if (cmdline[n].equalsIgnoreCase("-t")) {
          terms = Integer.parseInt(cmdline[n + 1]);
          if (terms < 1) {
            System.out.println("Must have at least 1 term.");
            System.exit(1);
          }
          n++;
        } else if (cmdline[n].equalsIgnoreCase("-p")) {
          population = Integer.parseInt(cmdline[n + 1]);
          if (terms < 2) {
            System.out.println("Must have population of at least 10.");
            System.exit(1);
          }
          n++;
        } else if (cmdline[n].equalsIgnoreCase("-%")) {
          int tp = Integer.parseInt(cmdline[n + 1]);
          if (tp < 0 || tp > 100) {
            System.out.println("Percentage out of range (0, 100).");
            System.exit(1);
          }
          toppercent = (double) tp / 100;
          n++;
        } else {
          text = cmdline[n];
          if (text.charAt(0) == '\"') {
            text = text.substring(1);
          }
          if (text.charAt(text.length() - 1) == '\"') {
            text = text.substring(0, text.length());
          }
        }
      } catch (ArrayIndexOutOfBoundsException e) {
        break;
      } catch (NumberFormatException e) {
        System.out.println("Invalid number");
        System.exit(1);
      }
    }
    int top = (int) (toppercent * population);
    int bottom = population - top;

    if (fileout == null) {
      System.out.println("Text: \"" + text + "\"");
    }
    System.out.print("Generations: ");
    if (generations == 0) {
      System.out.println("Infinite");
    } else {
      System.out.println(generations);
    }
    System.out.println("Terms: " + terms);
    System.out.println("Population: " + population);
    System.out.println("Top %: " + toppercent);
    System.out.println();

    CompareIndividuals compare = new CompareIndividuals();

    // generate initial population
    Individual[] citizen = new Individual[population];
    for (int n = 0; n < population; n++) {
      citizen[n] = new Individual(text, terms);
    }

    // keep looking for the best program
    int best = Integer.MAX_VALUE;
    for(int generation = 1; (generation <= generations) || (generations == 0);
        generation++) {
      // choose a top citizen to copy and tweak number order
      for (int n = bottom; n < population; n++) {
        citizen[n].tweak(citizen[r.nextInt(top)]);
      }

      // genetic stimulation
      for (int n = 1; n < population; n++) {
        int p = r.nextInt(population);
        if (p < n) { // less mutation with good, more mutation with bad
          citizen[n].mutate();
        } else {
          p = r.nextInt(top);
          citizen[n].crossover(citizen[p]);
        }
      }

      // get rid of the duds
      Arrays.sort(citizen, compare);
      for (int n = bottom; n < population; n++) {
        citizen[n].restart();
      }

      // display progress
      Arrays.sort(citizen, compare);
      if (citizen[0].getFitness() < best) {
        String temptext;
        System.out.println((best = citizen[0].getFitness()) + " " + (temptext = citizen[0].getCode())
                           + " [" + generation + "]");
        System.out.println();
        if (fileout != null) {
          PrintStream out;
          try {
            out = new PrintStream(new FileOutputStream(fileout));
            out.print(temptext);
            out.close();
          } catch (IOException e) {
            System.out.println("Error writing output file.");
            System.exit(1);
          }
        }
      }

    }
  }

  //------------------------------------------------------------------
  // getRandomBoolean()
  //------------------------------------------------------------------
  public static boolean getRandomBoolean() {
    return r.nextBoolean();
  }

  //------------------------------------------------------------------
  // getRandomInt()
  //------------------------------------------------------------------
  public static int getRandomInt(int n) {
    return r.nextInt(n);
  }

  //------------------------------------------------------------------
  // usage -- prints usage information
  //------------------------------------------------------------------
  public static void usage() {
    System.out.println("Usage: textgen [-i file] [-o file] [-t #] [-p #] [-% #] \"text\" [-?]");
    System.out.println("Where: \"text\"       Text to generate");
    System.out.println("       -i           Input filename (in place of \"text\")");
    System.out.println("       -o           Output filename, default: screen only");
    System.out.println("       -g           Generations, default: infinite");
    System.out.println("       -t           Number of terms, default: 4");
    System.out.println("       -p           Population, default: 500");
    System.out.println("       -%           Percent to rank as top, default: 10");
    System.out.println("       -?           Display usage information");
  }

}

//********************************************************************
// CompareIndividuals
//********************************************************************
class CompareIndividuals implements Comparator {
  public int compare(Object o1, Object o2) {
    return (((Individual) o1).getFitness() - ((Individual) o2).getFitness());
  }
}

//********************************************************************
// Individual
//********************************************************************
class Individual {
  int letters, mult, terms, fitness = -1;
  int[] term, finder, fixer;
  String expected;

  //------------------------------------------------------------------
  // (constructor)
  //------------------------------------------------------------------
  Individual(String expected, int terms) {
    this.expected = expected;
    letters = expected.length();
    this.terms = terms;
    term = new int[terms];
    restart();
  }

  //------------------------------------------------------------------
  // restart()
  //------------------------------------------------------------------
  public void restart() {
    // multiplier (1 to 15)
    mult = textgen.getRandomInt(15) + 1;
    // value of each term (0 to 15)
    for (int n = 0; n < terms; n++) {
      term[n] = textgen.getRandomInt(16);
    }
    // ><+- multiple before each dot (one for each letter)
    finder = new int[letters];
    fixer = new int[letters];
    for (int n = 0; n < letters; n++) {
      finder[n] = textgen.getRandomInt(terms);
    }
    fitness = -1;
  }

  //------------------------------------------------------------------
  // mutate()
  //------------------------------------------------------------------
  public void mutate() {
    int n;
    // mutate item
    if (textgen.getRandomBoolean()) {
      // term
      n = textgen.getRandomInt(terms);
      term[n] = textgen.getRandomInt(16);
    } else {
      // finder
      n = textgen.getRandomInt(letters);
      finder[n] = textgen.getRandomInt(terms);
    }
    fitness = -1;
  }

  //------------------------------------------------------------------
  // tweak()
  //------------------------------------------------------------------
  public void tweak(Individual giver) {
    if ((textgen.r).nextBoolean()) {
      // copy the giver exactly
      mult = giver.getMult();
      for (int n = 0; n < terms; n++) {
        term[n] = giver.getTerm(n);
      }
    } else {
      // copy the giver, but use a new multiplier, adjust the terms accordingly
      mult = textgen.getRandomInt(15) + 1;
      for (int n = 0; n < terms; n++) {
        term[n] = Math.round(giver.getTerm(n) * giver.getMult() / (float) mult);
      }
    }
    // copy the finder
    for (int n = 0; n < letters; n++) {
      finder[n] = giver.getFinder(n);
    }

    // swap two of the terms to see if it helps (can be the same term)
    int n1 = textgen.getRandomInt(terms);
    int n2 = textgen.getRandomInt(terms);
    int n3 = term[n2];
    term[n2] = term[n1];
    term[n1] = n3;

    // swap finders to match swapped terms
    for (int n = 0; n < letters; n++) {
      if (finder[n] == n1) {
        finder[n] = n2;
      } else if (finder[n] == n2) {
        finder[n] = n1;
      }
    }

    fitness = -1;
  }

  //------------------------------------------------------------------
  // crossover()
  //------------------------------------------------------------------
  public void crossover(Individual giver) {
    int n;
    if ((textgen.r).nextBoolean()) {
      // term
      n = textgen.getRandomInt(terms);
      int n2 = textgen.getRandomInt(terms);
      term[n2] = giver.getTerm(n);
    } else {
      // finder
      n = textgen.getRandomInt(letters);
      finder[n] = giver.getFinder(n);
    }
    fitness = -1;
  }

  //------------------------------------------------------------------
  // getMult()
  //------------------------------------------------------------------
  public int getMult() {
    return mult;
  }

  //------------------------------------------------------------------
  // getTerms()
  //------------------------------------------------------------------
  public int getTerms() {
    return terms;
  }

  //------------------------------------------------------------------
  // getTerm()
  //------------------------------------------------------------------
  public int getTerm(int n) {
    return term[n];
  }

  //------------------------------------------------------------------
  // getFinder()
  //------------------------------------------------------------------
  public int getFinder(int n) {
    return finder[n];
  }

  //------------------------------------------------------------------
  // getFitness()
  //------------------------------------------------------------------
  public int getFitness() {
    if (fitness < 0) {
      int length = 4 + mult + terms * 2 + letters; // mult [ terms> terms< -]> letters.
      int[] memory = new int[terms];
      for (int n = 0; n < terms; n++) {
        length += term[n];
        memory[n] = (mult * term[n]) % 256;
      }
      int mp = 0;
      for (int n = 0; n < letters; n++) {
        length += Math.abs(finder[n] - mp);
        mp = finder[n];
        fixer[n] = expected.charAt(n) - memory[mp];
        int n2 = Math.abs(fixer[n]);
        if (Math.abs(fixer[n]) > 128) {
          n2 = 256 - n2;
          if (fixer[n] > 0) {
            n2 = -n2;
          }
        }
        length += Math.abs(fixer[n]);
        memory[mp] = expected.charAt(n);
      }

      // combine results
      fitness = length;

      if (fitness < 0) { fitness = Integer.MAX_VALUE; }

    }
    return fitness;
  }

  //------------------------------------------------------------------
  // getCode()
  //------------------------------------------------------------------
  public String getCode() {
    String code = getBF(mult, '+', '+') + "[";
    for (int n = 0; n < terms; n++) {
      code += ">" + getBF(term[n], '+', '+');
    }
    code += getBF(terms, '<', '<') + "-]>";
    int mp = 0;
    for (int n = 0; n < letters; n++) {
      code += getBF(finder[n] - mp, '>', '<');
      mp = finder[n];
      code += getBF(fixer[n], '+', '-') + ".";
    }
    return code;
  }

  //------------------------------------------------------------------
  // getBF() {
  //------------------------------------------------------------------
  public String getBF(int n, char pos, char neg) {
    char ch = ' ';
    if (n > 0) {
      ch = pos;
    } else if (n < 0) {
      ch = neg;
    }
    n = Math.abs(n);
    if (n != 0) {
      char[] c = new char[Math.abs(n)];
      Arrays.fill(c, ch);
      return new String(c);
    } else {
      return "";
    }
  }

}



