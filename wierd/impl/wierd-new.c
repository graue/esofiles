#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>

typedef struct stack
{
   int value;
   struct stack *below;
} stack;

typedef struct thread
{
   int id;
   int x, y, dx, dy;
   stack *stack_head;
   struct thread *next, *prev;
} thread;

int size = 256;
int debug = 0;
int **world;
thread *current_thread;
int threads = 0;

void push(int value)
{
   stack *new;

   new = malloc(sizeof(*new));
   new->value = value;
   new->below = current_thread->stack_head;
   current_thread->stack_head = new;
}

int pop(void)
{
   stack *old;
   int value;

   old = current_thread->stack_head;
   if(old == NULL)
   {
      fprintf(stderr, "Insufficient stack!\n");
      exit(1);
   }
   value = old->value;
   current_thread->stack_head = old->below;
   free(old);
   return value;
}

void copy_stack(thread *dst, thread *src)
{
   stack *dst_loc, *prev_dst_loc = NULL, *src_loc;

   if(src->stack_head == NULL)
   {
      dst->stack_head = NULL;
      return;
   }
   for(src_loc = src->stack_head; src_loc != NULL; src_loc = src_loc->below)
   {
      dst_loc = malloc(sizeof(*dst_loc));
      dst_loc->value = src_loc->value;
      dst_loc->below = NULL;
      if(prev_dst_loc == NULL) dst->stack_head = dst_loc; else prev_dst_loc->below = dst_loc;
      prev_dst_loc = dst_loc;
   }
}

void new_thread(void)
{
   thread *new;

   new = malloc(sizeof(*new));
   new->id = threads++;
   new->x = current_thread->x;
   new->y = current_thread->y;
   new->dx = -current_thread->dx;
   new->dy = -current_thread->dy;
   copy_stack(new, current_thread);
   new->next = current_thread->next;
   new->prev = current_thread;
   current_thread->next->prev = new;
   current_thread->next = new;
}

void kill_thread(void)
{
   thread *old;

   old = current_thread;
   while(old->stack_head != NULL) pop();
   if(old->next == old) exit(0);
   old->prev->next = old->next;
   old->next->prev = old->prev;
   current_thread = old->next;
   free(old);
}

void turn_dxdy(int *dx, int *dy, int direction)
{
   int old_dx, old_dy;

   direction %= 8;
   if(direction < 0) direction += 8;
   while(direction-- > 0)
   {
      old_dx = *dx;
      old_dy = *dy;
      *dx = old_dy + old_dx;
      *dy = old_dy - old_dx;
      if(abs(*dx) == 2) *dx /= 2;
      if(abs(*dy) == 2) *dy /= 2;
   }
}

int can_turn(int direction)
{
   int dx = current_thread->dx, dy = current_thread->dy;

   turn_dxdy(&dx, &dy, direction);
   return world[current_thread->x + dx][current_thread->y + dy] != ' ';
}

void turn(int direction)
{
   turn_dxdy(&current_thread->dx, &current_thread->dy, direction);
}

void print_stack(void)
{
   char str1[256], str2[256] = "";
   stack *printing;

   fprintf(stderr, "The new stack is");
   if(current_thread->stack_head == NULL)
   {
      fprintf(stderr, " empty.\n");
      return;
   }
   for(printing = current_thread->stack_head; printing != NULL; printing = printing->below)
   {
      sprintf(str1, " [%d]%s", printing->value, str2);
      strcpy(str2, str1);
   }
   fputs(str2, stderr);
   fputc('\n', stderr);
}

void help(void)
{
   fprintf(stderr, "Valid options:\n");
   fprintf(stderr, "-help:          Show this screen.\n");
   fprintf(stderr, "-size <number>: Set the memory size.\n");
   fprintf(stderr, "-debug:         Print debugging information.\n");
   fprintf(stderr, "There should also be a non-option argument naming the program to be run.\n");
   exit(1);
}

int main(int argc, char **argv)
{
   struct option options[] = {{"help",    0, NULL, '?'},
                              {"size",    1, NULL, 's'},
                              {"debug",   0, NULL, 'd'}};
   int done = 0;
   char *ptr;
   int x, y;
   char *in_name = NULL;
   FILE *in_file;

   opterr = 0;
   while(!done)
   {
      switch(getopt_long_only(argc, argv, "-?", options, NULL))
      {
      case 's': if(*optarg == '\0') help();
                size = strtol(optarg, &ptr, 10);
                if(*ptr != '\0') help();
                break;
      case 'd': if(debug) help();
                debug = 1;
                break;
      case '?': help();
      case 1:   if(in_name != NULL) help();
                in_name = strdup(optarg);
                break;
      case EOF: done = 1;
                break;
      }
   }
   if(in_name == NULL) help();
   world = malloc(sizeof(*world) * size);
   for(x = 0; x < size; x++)
   {
      world[x] = malloc(sizeof(**world) * size);
      for(y = 0; y < size; y++) world[x][y] = ' ';
   }
   in_file = fopen(in_name, "r");
   if(in_file == NULL)
   {
      perror(in_name);
      return 1;
   }
   y = 1;
   for(;;)
   {
      char ch;

      x = 1;
      for(;;)
      {
         ch = fgetc(in_file);
         if(ch == '\n' || ch == EOF) break;
         world[x++][y] = ch;
      }
      if(ch == EOF) break;
      y++;
   }
   fclose(in_file);
   if(world[1][1] == ' ')
   {
      fprintf(stderr, "Starting point for program is empty.\n");
      return 1;
   }
   current_thread = malloc(sizeof(*current_thread));
   current_thread->id = threads++;
   current_thread->x = current_thread->y = 1;
   current_thread->dx = current_thread->dy = 1;
   current_thread->stack_head = NULL;
   current_thread->next = current_thread->prev = current_thread;
   srandom(time(NULL));
   for(;;)
   {
      int closest_right = 4, closest_left = 4;
      int which, x, y;

      if(debug) fprintf(stderr, "Thread %3d at (%3d,%3d) ", current_thread->id, current_thread->x, current_thread->y);
      if(can_turn(1)) closest_left = 1; else if(can_turn(2)) closest_left = 2; else if(can_turn(3)) closest_left = 3;
      if(can_turn(-1)) closest_right = 1; else if(can_turn(-2)) closest_right = 2; else if(can_turn(-3)) closest_right = 3;
      if(can_turn(0))
      {
         if(debug) fprintf(stderr, "is happily marching along (0 degrees).\n");
      }
      else if(closest_left == closest_right)
      {
         switch(closest_left)
         {
         case 1: if(debug) fprintf(stderr, "faces a difficult choice (45 or 315 degrees).\n");
                 if(random() & 1024) turn(1); else turn(-1);
                 break;
         case 2: if(debug) fprintf(stderr, "spawns a new thread (90 and 270 degrees).\n");
                 turn(2);
                 new_thread();
                 current_thread = current_thread->next;
                 continue;
         case 3: if(debug) fprintf(stderr, "is confused (135 and 225 degrees).\n");
                 fprintf(stderr, "The programmer has not yet decided what a 135/225 T-junction should do!");
                 return 1;
         case 4: if(debug) fprintf(stderr, "bumps into a dead end (180 degrees).\n");
                 kill_thread();
                 continue;
         }
      }
      else if(closest_left < closest_right)
      {
         switch(closest_left)
         {
         case 1: if(debug) fprintf(stderr, "places a one on his stack (45 degrees).\n");
                 push(1);
                 break;
         case 2: which = pop();
                 if(which)
                 {
                    if(debug) fprintf(stderr, "is afraid from the turn (90 degrees).\n");
                    turn(2);
                    break;
                 }
                 if(debug) fprintf(stderr, "happily marches through the turn (90 degrees).\n");
                 break;
         case 3: which = pop();
                 y = pop();
                 x = pop();
                 if(which)
                 {
                    if(debug) fprintf(stderr, "gets a number for his stack (135 degrees).\n");
                    push(world[x][y]);
                 }
                 else
                 {
                    if(debug) fprintf(stderr, "puts a number in his stack down (135 degrees).\n");
                    world[x][y] = pop();
                 }
                 break;
         }
         if(debug) print_stack();
         turn(closest_left);
      }
      else
      {
         switch(closest_right)
         {
         case 1: if(debug) fprintf(stderr, "sits down to work out a subtraction (315 degrees).\n");
                 y = pop();
                 x = pop();
                 push(x - y);
                 break;
         case 2: which = pop();
                 if(which)
                 {
                    if(debug) fprintf(stderr, "is afraid from the turn (270 degrees).\n");
                    turn(-2);
                    break;
                 }
                 if(debug) fprintf(stderr, "happily marches through the turn (270 degrees).\n");
                 break;
         case 3: which = pop();
                 if(which)
                 {
                    which = pop();
                    if(debug) fprintf(stderr, "wishes you to know that \"%c\" (225 degrees).\n", which);
                    if(!debug || !isatty(1)) putchar(which);
                 }
                 else
                 {
                    which = getchar();
                    if(debug) fprintf(stderr, "discovers that \"%c\" (225 degrees).\n", which);
                    push(which);
                 }
                 break;
         }
         if(debug) print_stack();
         turn(-closest_right);
      }
      current_thread->x += current_thread->dx;
      current_thread->y += current_thread->dy;
      current_thread = current_thread->next;
   }
}

