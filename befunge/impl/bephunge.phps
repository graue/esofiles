<?php

/*
Bephunge
A Befunge-93 interpretter written in PHP
by David Gucwa (6/9/2006)
dgucwa@gmail.com

6/12/2006 - Commented out a line in the push function

This code is in the public domain.

Usage: php bephunge.php <befunge program>
*/


define('UP', 1);
define('DOWN', 2);
define('LEFT', 3);
define('RIGHT', 4);

class Program
{
	/* The class that holds the Befunge program */
	
	// The 80x25 array that holds the program instructions
	var $instructions;
	
	// The current position of the program and its direction
	var $cursor_x;
	var $cursor_y;
	var $cursor_dir;
	
	// The stack. An array of characters.
	var $stack;
	
	// The stack pointer. Gives the index of the most recently pushed value.
	var $stackPtr;
	
	// String mode. Turns on the first time a double-quote " is seen, and toggles thereafter.
	var $stringMode;
	
	function Program ()
	{
		// Initially set the entire program to blank spaces
		for ($i=0; $i<80; $i++)
			for ($j=0; $j<25; $j++)
				$this->instructions[$i][$j] = ' ';
		
		// Program starts in the upper left corner and heading right
		$this->cursor_x = 0;
		$this->cursor_y = 0;
		$this->cursor_dir = RIGHT;
		
		$this->stack = array();
		
		// -1 means there are no values in the stack
		$this->stackPtr = -1; 
		
		$this->stringMode = false;
	}
	
	function loadFromFile ($filename)
	{
		$fileArray = file($filename);
		if (!$fileArray)
			die("Couldn't open $filename for reading.\n");
		$numLines = 0;
		foreach ($fileArray as $line)
		{
			$numLines++;
			if ($numLines > 25)
				break;
			$line = trim($line, "\n");
			for ($i=0; $i<strlen($line); $i++)
			{
				if ($i >= 80)
					break;
				$this->instructions[$i][$numLines-1] = substr($line, $i, 1);
			}
		}
	}
	
	function push ($value)
	{
		// The stack can hold signed long integers.
		//$value = $value % 256;
		$this->stackPtr++;
		$this->stack[$this->stackPtr] = $value;
	}
	
	function pop ()
	{
		if ($this->stackPtr == -1)
			return 0;
		else
			return $this->stack[$this->stackPtr--];
	}
	
	function step ()
	{
		// Moves the cursor one step in the direction it is facing
		switch ($this->cursor_dir)
		{
			case UP:
				$this->cursor_y--;
				if ($this->cursor_y < 0)
					$this->cursor_y = 24;
				break;
			case DOWN:
				$this->cursor_y++;
				if ($this->cursor_y >= 25)
					$this->cursor_y = 0;
				break;
			case LEFT:
				$this->cursor_x--;
				if ($this->cursor_x < 0)
					$this->cursor_x = 79;
				break;
			case RIGHT:
				$this->cursor_x++;
				if ($this->cursor_x >= 80)
					$this->cursor_x = 0;
				break;
		}
	}
	
	function execute ()
	{
		// Executes whichever instruction the cursor is on, and moves to the next instruction accordingly
		// Returns true normally
		// Returns false if the program is done
		
		$instruction = $this->instructions[$this->cursor_x][$this->cursor_y];
		
		if ($this->stringMode AND $instruction != '"')
		{
			$this->push(ord($instruction));
			$this->step();
			return true;
		}
		
		switch ($instruction)
		{
			case '<':
				$this->cursor_dir = LEFT;
				break;
			case '>':
				$this->cursor_dir = RIGHT;
				break;
			case 'v':
				$this->cursor_dir = DOWN;
				break;
			case '^':
				$this->cursor_dir = UP;
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				$this->push($instruction - '0');
				break;
			case '+':
				$this->push($this->pop() + $this->pop());
				break;
			case '-':
				$a = $this->pop();
				$b = $this->pop();
				$this->push($b-$a);
				break;
			case '*':
				$this->push($this->pop() * $this->pop());
				break;
			case '/':
				$a = $this->pop();
				$b = $this->pop();
				if ($a == 0)
					$result = $this->ask('int');
				else
					$result = (int)($b/$a);
				$this->push($result);
				break;
			case '%':
				$a = $this->pop();
				$b = $this->pop();
				if ($a == 0)
					$result = $this->ask('int');
				else
					$result = $b % $a;
				$this->push($result);
				break;
			case '!':
				$a = $this->pop();
				$this->push($a == 0 ? 1 : 0);
				break;
			case '`':
				$a = $this->pop();
				$b = $this->pop();
				$this->push($b > $a ? 1 : 0);
				break;
			case '?':
				$this->cursor_dir = rand(1,4);
				break;
			case '_':
				if ($this->pop() == 0)
					$this->cursor_dir = RIGHT;
				else
					$this->cursor_dir = LEFT;
				break;
			case '|':
				if ($this->pop() == 0)
					$this->cursor_dir = DOWN;
				else
					$this->cursor_dir = UP;
				break;
			case '"':
				$this->stringMode = !$this->stringMode;
				break;
			case ':':
				$a = $this->pop();
				$this->push($a);
				$this->push($a);
				break;
			case '\\':
				$a = $this->pop();
				$b = $this->pop();
				$this->push($a);
				$this->push($b);
				break;
			case '$':
				$this->pop();
				break;
			case '.':
				print($this->pop());
				break;
			case ',':
				print(chr($this->pop()));
				break;
			case '#':
				$this->step();
				break;
			case 'p':
				$y = $this->pop();
				$x = $this->pop();
				$v = $this->pop();
				$this->instructions[$x][$y] = chr($v);
				break;
			case 'g':
				$y = $this->pop();
				$x = $this->pop();
				$this->push(ord($this->instructions[$x][$y]));
				break;
			case '&':
				$this->push($this->ask('int'));
				break;
			case '~':
				$this->push($this->ask('char'));
				break;
			case '@':
				return false;
			default:
				break;
		}
		$this->step();
		return true;
	}
	
	function ask ($type)
	{
		// Asks the user for input from the keyboard. $type can be 'int' or 'char'
		if ($type == 'int')
			return intval(fgets(STDIN));
		else if ($type == 'char')
			return ord(fgetc(STDIN));
	}			
}

$Program = new Program();
$Program->loadFromFile($argv[1]);
while ($Program->execute());

?>
