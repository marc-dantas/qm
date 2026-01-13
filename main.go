package main

import (
	"fmt"
	"log"
	"os"
	"os/exec"
	"path/filepath"
 	"golang.org/x/term"
)

type Entry struct {
	Name string
	IsDir bool
}

func CountDirs(entries []Entry) int {
	count := 0
	for _, item := range entries {
		if item.IsDir { count += 1 }
	}
	return count
}

func CountFiles(entries []Entry) int {
	count := 0
	for _, item := range entries {
		if !item.IsDir { count += 1 }
	}
	return count
}

func (e Entry) Print(Selected bool) {
	if Selected {
		fmt.Printf(" %s| %s> ", GRAY, RESET)
	} else {
		fmt.Printf(" %s| %s  ", GRAY, RESET)
	}
	if e.IsDir {
		fmt.Printf("%s%s%s/", CYAN, e.Name, RESET)
	} else {
		fmt.Printf("%s%s%s", ITAL, e.Name, RESET)
	}
	fmt.Print("\n")
}

const RESET  = "\x1B[0m"  // Reset code, don't change this
const RED    = "\x1B[31m"
const PURPLE = "\x1B[35m"
const GREEN  = "\x1B[32m"
const UNDER  = "\x1B[4m"  // Underline
const CYAN   = "\x1B[36m"
const ITAL   = "\x1B[3m"
const BLUE   = "\x1B[34m"
const GRAY   = "\x1B[0;30m"

func getchar() byte {
	oldState, _ := term.MakeRaw(int(os.Stdin.Fd()))
	defer term.Restore(int(os.Stdin.Fd()), oldState)
	b := make([]byte, 1)
	os.Stdin.Read(b)
	return b[0]
}

func clear() {
	fmt.Print("\033[H\033[2J")
}

func trace_error(message string) {
	clear();
	fmt.Printf("%serror%s: %s\n", RED, RESET, message)
	fmt.Printf("%sPress any key to continue...%s\n", ITAL, RESET)
	getchar()
	
}

func ls(path string) []Entry {
	direntries, err := os.ReadDir(path);
	if err != nil {
		trace_error(fmt.Sprintf("Could not open directory %q", path))
		return nil
	}
	entries := make([]Entry, len(direntries))
	for i, direntry := range direntries {
		e := Entry{Name: direntry.Name(), IsDir: direntry.IsDir()}
		entries[i] = e
	}
	return entries
}

func PrintEntries(entries []Entry, cursor int, window int) {
	if len(entries) <= 0 {
		fmt.Printf("\n    _\n")
		return
	}
	gap := (window/2)
	start := 0
	if cursor > gap {
		start = cursor - gap
	}
	end := len(entries)
	if (cursor + gap) < len(entries) {
		end = cursor + gap
	}
	if (cursor + gap) < window {
		end = cursor + window - cursor
	}

	if start > 0 {
		fmt.Printf("%s...%s\n", PURPLE, RESET)
	}
	for (start < end) && (start < len(entries)) {
		entries[start].Print(start == cursor)
		start += 1
	}
	if end < len(entries) {
		fmt.Printf("%s...%s\n", PURPLE, RESET)
	}
}

func main() {
	path := "."
	if len(os.Args) > 1 {
		path = os.Args[1]
	}
	cursor := 0;

	entries := ls(path);
	for {
		clear();
		
		absPath, err := filepath.Abs(path)
		if err != nil {
			log.Fatal(err)
		}
		fmt.Printf("    %s%s%s\n", PURPLE, absPath, RESET)
		fmt.Printf("    %s%d directories\t%d files%s\n", GREEN, CountDirs(entries), CountFiles(entries), RESET)

		PrintEntries(entries, cursor, 10)
		c := getchar();
		switch c {
			case 'q':
				clear()
				return
			case 's':
				if cursor < len(entries)-1 {
					cursor += 1;
				}
			case ' ':
				if len(entries) > 0 && !entries[cursor].IsDir {
					var program string
					fmt.Printf("%sopen with%s: ", BLUE, RESET)
					fmt.Scanln(&program)
					fullpath := filepath.Join(path, entries[cursor].Name)
					clear()
					cmd := exec.Command(program, fullpath)
					err := cmd.Run()
					if err != nil {
						trace_error(err.Error())
					}
				}
			case 'd':
				if len(entries) > 0 && entries[cursor].IsDir {
					path = filepath.Join(path, entries[cursor].Name)
					cursor = 0
					entries = ls(path);
				}
			case 'a':
				path = filepath.Join(path, "..")
				cursor = 0
				entries = ls(path);
			case 'w':
				if cursor > 0 {
					cursor -= 1;
				}
		}
	}
	
	fmt.Println(path)
}
