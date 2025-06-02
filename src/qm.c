#include <dirent.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

// Color, change each color according to your preference.
#define RESET  "\x1B[0m"  // Reset code, don't change this
#define RED    "\x1B[31m"
#define PURPLE "\x1B[35m"
#define UNDER  "\x1B[4m"
#define CYAN   "\x1B[36m"
#define ITAL   "\x1B[3m"

#define C_DIR      "\x1B[0;36m" // Color applied to directory names  
#define C_DIR_SEL  "\x1B[4;36m" // Color applied to selected directory  
#define C_FILE     "\x1B[0;3m"  // Color applied to file names  
#define C_FILE_SEL "\x1B[4;3m"  // Color applied to selected file  

// Maximum capacity of items (folders + files) that this explorer handles inside a folder
#define MAX_ITEMS 1024

// Amount of items (folders + files) that can be rendered at once (Rolling window size in lines)
#define ROLLING_WINDOW_SIZE 12

typedef struct {
	char* dirs[MAX_ITEMS];
	char* files[MAX_ITEMS];
	int dlen, flen, len, selection;
} Instance;

void usage(char* program) {
	fprintf(stderr, "qm: usage: %s PATH\n", program);	
}

int cmpstringp(const void* p1, const void* p2) {
    return strcmp(*(const char**)p1, *(const char**)p2);
}

int Instance_new(Instance* target, char* path) {
	DIR* d = opendir(path);
	if (d == NULL) {
		fprintf(stderr, "qm: error: could not open directory \"%s\"\n", path);
		return 255;
	}
	target->dlen = 0;
	target->flen = 0;
	struct dirent* dir;
	for (int i = 0; i < MAX_ITEMS; i++) {
		dir = readdir(d);
		if (dir == NULL) break;
		if (dir->d_type == DT_DIR) {
			target->dirs[target->dlen] = dir->d_name;
			target->dlen += 1;
		} else {
			target->files[target->flen] = dir->d_name;
			target->flen += 1;
		}
	}
	closedir(d);

	qsort(target->dirs, target->dlen, sizeof(char*), cmpstringp);
    qsort(target->files, target->flen, sizeof(char*), cmpstringp);

	target->len = target->flen + target->dlen;
	target->selection = 0;
	return 0;
}

void reset() {
	printf("\033[H\033[J");
    fflush(stdout);
}

char readchar() {
	struct termios oldt, newt;
    char ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);	
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

void draw(Instance* inst) {
	char p[1024];
	getcwd(p, sizeof(p));
	printf(PURPLE"%s"RESET"\n\n", p);

	int window = ROLLING_WINDOW_SIZE;
	int start = inst->selection - (window / 2);
	if (start < 0) start = 0;
	if (start + window > inst->len) start = inst->len - window;
	if (start < 0) start = 0;

	if (start > 0) printf("   ["PURPLE"..."RESET"]\n");

	int render_count = 0;

	// Render directories
	for (int i = start; i < inst->dlen && render_count < window; i++) {
		if (inst->selection == i) printf("  > "C_DIR_SEL"%s"RESET"/", inst->dirs[i]);
		else                      printf("    "C_DIR"%s"RESET"/", inst->dirs[i]);
		printf("\n");
		render_count++;
	}

	// Render files
	for (int i = 0; i < inst->flen && render_count < window; i++) {
		int file_index = inst->dlen + i;
		if (file_index >= start) {
			if (inst->selection == file_index) printf("  > "C_FILE_SEL"%s"RESET, inst->files[i]);
			else                               printf("    "C_FILE"%s"RESET, inst->files[i]);
			printf("\n");
			render_count++;
		}
	}

	if (start + window < inst->len) printf("   ["PURPLE"..."RESET"]\n");
}

int main(int argc, char** argv) {
	char p[1024];
	getcwd(p, sizeof(p));
	char* path = argc >= 2 ? argv[1] : p;
	Instance inst = {0};
	if (Instance_new(&inst, path) != 0) return 255;
	chdir(path);
	for (;;) {
		reset();
		draw(&inst);
		switch (readchar()) {
		case 'w':
			if (inst.selection > 0) {
				inst.selection -= 1;
			}
			break;
		case 'd':
		    if (inst.selection < inst.dlen) {
				path = inst.dirs[inst.selection];
	            if (Instance_new(&inst, path) == 0) {
	                chdir(path);
	            }
	        }
	        break;
		case 'h':
			reset();
			printf(PURPLE"HELP"RESET"\n");
			printf("  - "CYAN"Navigation"RESET":\n");
			printf("      ["RED"W"RESET"] Move selection up\n");
			printf("      ["RED"A"RESET"] Go back\n");
			printf("      ["RED"S"RESET"] Move selection down\n");
			printf("      ["RED"D"RESET"] Enter folder under selection\n");
			printf("  - "CYAN"Application"RESET":\n");
			printf("      ["RED"Q"RESET"] Quit application\n");
			printf("      ["RED"H"RESET"] Show this help message\n");
			printf("  - "CYAN"Utility"RESET":\n");
			printf("      ["RED"R"RESET"] Run an arbitrary terminal command\n");
			printf("      ["RED"ENTER"RESET"] Launch desired application with selected file\n");


			printf(UNDER"\n(c) 2025 Marcio Dantas"RESET"\n");
			printf(ITAL"Press any key to continue..."RESET"\n");
			readchar();
			break;
		case 'a':
			path = "..";
            if (Instance_new(&inst, path) == 0) {
                chdir(path);
            }
			break;
		case 's':
			if (inst.selection+1 < inst.len) {
				inst.selection += 1;
			}
			break;
		case ' ':
		case '\n':
			if (inst.selection >= inst.dlen) {
				char* filename = inst.files[inst.selection - inst.dlen];
				printf(CYAN"Program to launch with '"RESET C_FILE_SEL"%s"RESET CYAN"'"RESET": ", filename);
				char program[64];
				fgets(program, 64, stdin);
				program[strcspn(program, "\n")] = 0;
				char cmd[255];
				sprintf(cmd, "%s \"%s\"", program, filename);
				reset();
				system(cmd);
				printf(ITAL"\nPress any key to continue..."RESET"\n");
				readchar();
			}
			break;
		case 'r':
			printf(CYAN"Run"RESET": ");
			char cmd[255];
			fgets(cmd, 255, stdin);
			cmd[strcspn(cmd, "\n")] = 0;
			reset();
			system(cmd);
			printf(ITAL"\nPress any key to continue..."RESET"\n");
			readchar();
			break;
		case 'q':
			reset();
		    return 0;		
		}
	}
	return 0;
}
