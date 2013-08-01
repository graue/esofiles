/*
 *  by Jannis Harder (jix)
 *
 *  compile using: gcc -O3 cipple.c -o cipple
 *  optional flags:
 *  -DNOINLINE disable inlineing
 *  -DINCR <int value> sets memory increment for stacks
 *  -DDECR <int value> sets memory decrement for stacks (should be a multiply of INCR)
 *  -DSTARTS <int value> sets initial stack size (should be less than DECR and a multiply of INCR)
 *  -DINSPECT enables instruction dumping (generating kipple code form the internal representation)
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#ifndef INCR
#define INCR 20
#endif
#ifndef DECR
#define DECR (INCR*4)
#endif

#ifndef STARTS
#define STARTS (INCR*2)
#endif




#ifdef NOINLINE
#define ILINE /*ignore*/ 
#else
#define ILINE inline
#endif




enum {
    PuNone = 0,
    PuItoa
};

enum {
    PoNone = 0,
};


typedef struct {
    int *data;
    int usize;
    int msize;
    int puspecial;
    /*int pospecial;
    int clrspecial;*/
}  stack;

ILINE void push(stack *cstack,int value) {
    char temp[13];
    int ccount, i;
    switch(cstack->puspecial) {
    case PuNone:
        if(cstack->usize+1 == cstack->msize){
            cstack->msize+=INCR;
            cstack->data = realloc(cstack->data, cstack->msize*sizeof(int));
        }
        cstack->data[++(cstack->usize)]=value;
        break;
    case PuItoa:
        ccount = sprintf(temp,"%i",value);
        if(cstack->usize+ccount+1 > cstack->msize){
            cstack->msize+=INCR;
            cstack->data = realloc(cstack->data, cstack->msize*sizeof(int));
        }
        for(i=0;i<ccount;i++)
            cstack->data[++(cstack->usize)]=temp[i];
        break;
    }
}

ILINE int pop(stack *cstack) {
    int rv;
    /*switch(cstack->pospecial) {
    case 0:*/
        if(cstack->usize == 0) 
            return 0;
        rv = cstack->data[cstack->usize--];
        if(cstack->usize < cstack->msize-DECR){
            cstack->msize-=DECR;
            cstack->data = realloc(cstack->data, cstack->msize*sizeof(int));
        }
    /*    break;
    }*/
    return rv;
}

ILINE void clear(stack *cstack) {
    /*if(!cstack->clrspecial){*/
        free(cstack->data);
        cstack->data = malloc(STARTS*sizeof(int));
        cstack->usize = 0;
        cstack->msize = STARTS;
    /*}*/
}

ILINE void init(stack *cstack) {
    cstack->data = malloc(STARTS*sizeof(int)+1);
    cstack->usize = 0;
    cstack->msize = STARTS;
    cstack->puspecial = 0;
    /*cstack->pospecial = 0;
    cstack->clrspecial = 0;*/
}

ILINE void freestack(stack *cstack) {
    free(cstack->data);
    free(cstack);
}

ILINE int empty(stack *cstack) {
    return cstack->usize == 0;
}

ILINE int last(stack *cstack) {
    return cstack->data[cstack->usize];
}

enum {
    Inoop = 0,
    Ipush,
    Imove,
    Iaddv,
    Iadds,
    Isubv,
    Isubs,
    Izclr,
    Iloop,
};

typedef struct s_ins {
    int instruction;
    union {
        stack* s;
        int i;
    } op_a;
    union {
        stack* s;
        int i;
        struct s_ins *p;
    } op_b;
    struct s_ins *next;
} instruction;

void freeprog (instruction *program) {
    instruction *cins = program;
    instruction *tins;
    while(cins){
        if(cins->instruction == Iloop)
                freeprog(cins->op_b.p);
        tins = cins;
        cins = cins->next;
        free(tins);
    }
}

void run (instruction *program) {
    instruction *cins = program;
    while(cins) {
        switch(cins->instruction){
        case Inoop:
            break;
        case Ipush:
            push(cins->op_a.s,cins->op_b.i);
            break;
        case Imove:
            push(cins->op_a.s,pop(cins->op_b.s));
            break;
        case Iaddv:
            push(cins->op_a.s,last(cins->op_a.s)+cins->op_b.i);
            break;
        case Iadds:
            push(cins->op_a.s,last(cins->op_a.s)+pop(cins->op_b.s));
            break;
        case Isubv:
            push(cins->op_a.s,last(cins->op_a.s)-cins->op_b.i);
            break;
        case Isubs:
            push(cins->op_a.s,last(cins->op_a.s)-pop(cins->op_b.s));
            break;
        case Izclr:
            if(!last(cins->op_a.s))
                clear(cins->op_a.s);
            break;
        case Iloop:
            while(!empty(cins->op_a.s))
                run(cins->op_b.p);
            break;
        }
        cins = cins->next;
    } 
}





#define UCASE(X) (((X)>96)?((X)-32):(X))
#define CRANGE(X) ((X)>63 && (X)<91)
#define NRANGE(X) ((X)>47 && (X)<58)

#define MALLOCI nins = malloc(sizeof(instruction)); \
                nins->next = NULL;\
                if(!first) first = nins; \
                if(current) current->next = nins; \
                current = nins;



instruction * iparse(stack *stacks,int length, char *ldata) {
    instruction *first = NULL;
    instruction *current = NULL;
    instruction *nins = NULL;
    int isstring = 0;

    char *lchr = ldata;
    char *mchr = ldata+length;
    int i1,i2,i3,i4;

    /* search for instructions */
    isstring = 0;
    for(lchr=ldata;lchr<mchr;lchr++) {
        if((*lchr) == '"'){
            isstring = !isstring;
        }
        else if(!isstring){
            switch(*lchr){
            case '+':
                if(lchr==ldata || lchr==mchr-1) break;
                i1 = UCASE(*(lchr-1));
                i2 = UCASE(*(lchr+1));
                if(!(CRANGE(i1) && (CRANGE(i2) || NRANGE(i2))))
                    break;
                MALLOCI;
                    current->op_a.s = &stacks[i1-'@'];
                if(CRANGE(i2)){
                    current->instruction = Iadds;
                    current->op_b.s = &stacks[i2-'@'];
                }
                else {
                    current->instruction = Iaddv;
                    current->op_b.i = 0;
                    for(i3=1;lchr+i3<mchr && NRANGE(lchr[i3]);i3++){
                        current->op_b.i*=10;
                        current->op_b.i+=lchr[i3]-'0';
                    }
                }
                break;
            case '-':
                if(lchr==ldata || lchr==mchr-1) break;
                i1 = UCASE(*(lchr-1));
                i2 = UCASE(*(lchr+1));
                if(!(CRANGE(i1) && (CRANGE(i2) || NRANGE(i2))))
                    break;
                MALLOCI;
                    current->op_a.s = &stacks[i1-'@'];
                if(CRANGE(i2)){
                    current->instruction = Isubs;
                    current->op_b.s = &stacks[i2-'@'];
                }
                else {
                    current->instruction = Isubv;
                    current->op_b.i = 0;
                    for(i3=1;lchr+i3<mchr && NRANGE(lchr[i3]);i3++){
                        current->op_b.i*=10;
                        current->op_b.i+=lchr[i3]-'0';
                    }
                }
                break;
            case '?':
                if(lchr==ldata) break;
                i1 = UCASE(*(lchr-1));
                if(!CRANGE(i1))
                    break;
                MALLOCI;
                current->instruction = Izclr;
                current->op_a.s = &stacks[i1-'@'];
                break;
            case '<':
                if(lchr==ldata || lchr==mchr-1) break;
                i1 = UCASE(*(lchr-1));
                i2 = UCASE(*(lchr+1));
                if(!CRANGE(i1))
                    break;
                if(CRANGE(i2)){
                    MALLOCI;
                    current->instruction = Imove;
                    current->op_a.s = &stacks[i1-'@'];
                    current->op_b.s = &stacks[i2-'@'];
                }
                else if(NRANGE(i2)){
                    MALLOCI;
                    current->instruction = Ipush;
                    current->op_a.s = &stacks[i1-'@'];
                    current->op_b.i = 0;
                    for(i3=1;lchr+i3<mchr && NRANGE(lchr[i3]);i3++){
                        current->op_b.i*=10;
                        current->op_b.i+=lchr[i3]-'0';
                    }
                }
                else if(i2=='"'){
                    for(i3=2;lchr+i3<mchr && (lchr[i3]!='"');i3++){
                        MALLOCI;
                        current->instruction = Ipush;
                        current->op_a.s = &stacks[i1-'@'];
                        current->op_b.i = lchr[i3];
                    }
                }
                break;
            case '>':
                if(lchr==ldata || lchr==mchr-1) break;
                i1 = UCASE(*(lchr-1));
                i2 = UCASE(*(lchr+1));
                if(!CRANGE(i2))
                    break;
                if(CRANGE(i1)){
                    MALLOCI;
                    current->instruction = Imove;
                    current->op_a.s = &stacks[i2-'@'];
                    current->op_b.s = &stacks[i1-'@'];
                }
                else if(NRANGE(i1)){
                    MALLOCI;
                    current->instruction = Ipush;
                    current->op_a.s = &stacks[i2-'@'];
                    current->op_b.i = 0;
                    i4=1;
                    for(i3=-1;lchr+i3>=ldata && NRANGE(lchr[i3]);i3--){
                        current->op_b.i+= i4 * (lchr[i3]-'0');
                        i4*=10;
                    }
                }
                else if(i1=='"'){
                    for(i3=-2;lchr+i3>=ldata && (lchr[i3]!='"');i3--){
                        MALLOCI;
                        current->instruction = Ipush;
                        current->op_a.s = &stacks[i2-'@'];
                        current->op_b.i = lchr[i3];
                    }
                }
                break;
            case '(':
                if(lchr==mchr-1) break;
                i1 = UCASE(*(lchr+1));
                if(!CRANGE(i1))
                    break;
                i3=0;
                i4=1;
                for(i2=1;lchr+i2<mchr && i4 > 0;i2++){
                    if(lchr[i2]=='"')
                        i3=!i3;
                    if(!i3){
                        if(lchr[i2]=='(')
                            i4++;
                        else if(lchr[i2]==')')
                            i4--;
                    }
                }
                MALLOCI;
                current->instruction = Iloop;
                current->op_a.s = &stacks[i1-'@'];
                current->op_b.p = iparse(stacks,i2-1,lchr+1);
                lchr+=i2-1;
                break;
            }
        }
    }
    return first;
}


instruction * parse(stack *stacks,int length, char *data) {
    char *ldata = malloc(length);
    instruction *rv;
    char *lchr = ldata;
    char *mchr = ldata+length;

    int isstring = 0;
    int comment = 0;
    memcpy(ldata,data,length);
    /* clear comments */
    for(;lchr<mchr;lchr++) {
        if(isstring && *lchr == '"')
            isstring = 0;
        else if((!comment) && *lchr == '"')
            isstring = 1;
        else if(comment && *lchr == '\n')
            comment = 0;
        else if((!isstring) && *lchr == '#')
            comment = 1;
        if(comment)
            *lchr = ' ';
        
    }
    rv = iparse(stacks,length,ldata);
    free(ldata);
    return rv;
}
#ifdef INSPECT
void dumpprog(int depth,stack *stacks,instruction *program) {
    char *e;
    instruction *cins = program;
    e = malloc(depth+1);
    memset(e,':',depth);
    e[depth]=0;
    while(cins){
        switch(cins->instruction){
        case Inoop:
            fprintf(stderr,"%s noop\n",e);
            break;
        case Ipush:
            fprintf(stderr,"%s push %c<%i\n",e,'@'+cins->op_a.s-stacks,cins->op_b.i);
            break;
        case Imove:
            fprintf(stderr,"%s move %c<%c\n",e,'@'+cins->op_a.s-stacks,'@'+cins->op_b.s-stacks);
            break;
        case Iaddv:
            fprintf(stderr,"%s addv %c+%i\n",e,'@'+cins->op_a.s-stacks,cins->op_b.i);
            break;
        case Iadds:
            fprintf(stderr,"%s adds %c+%c\n",e,'@'+cins->op_a.s-stacks,'@'+cins->op_b.s-stacks);
            break;
        case Isubv:
            fprintf(stderr,"%s subv %c-%i\n",e,'@'+cins->op_a.s-stacks,cins->op_b.i);
            break;
        case Isubs:
            fprintf(stderr,"%s subs %c-%c\n",e,'@'+cins->op_a.s-stacks,'@'+cins->op_b.s-stacks);
            break;
        case Izclr:
            fprintf(stderr,"%s zclr %c?\n",e,'@'+cins->op_a.s-stacks);
            break;
        case Iloop:
            fprintf(stderr,"%s loop (%c\n",e,'@'+cins->op_a.s-stacks);
                dumpprog(depth+2,stacks,cins->op_b.p);
            fprintf(stderr,"%s )\n",e);
            break;

        }
        cins = cins->next;
    }
    
}
#endif

void runstring(int length,char *program) {
    stack stacks[27];
    int i;
    int t;
    instruction *bc;
    for(i=0;i<27;i++)
        init(&stacks[i]);
    stacks[0].puspecial = PuItoa;
    bc = parse(stacks,length,program);
#ifdef INSPECT
    dumpprog(1,stacks,bc);
#endif
    while((t = getchar()) && !feof(stdin)){

        push(&stacks[9],t);
    }

    run(bc);
    freeprog(bc);
    while(!empty(&stacks[15])){
        putchar(pop(&stacks[15]));
    }
    fflush(stdout);
    for(i=0;i<27;i++)
        free(stacks[i].data);
    
}

/*void runcstring(char *program) {
    runstring(strlen(program),program);
}*/
#define APP "10>a (a-1 66>o a?)"

int main (int argc, const char * argv[]) {
    char *program;
    int size = 0;
    FILE *infile;
    if(argc<2)
        infile = stdin;
    else
        infile= fopen(argv[1],"r");
    
    if(!infile)
        return 1;
    program = malloc(200);
    while(!feof(infile)){
        program = realloc(program,size+200);
        size+=fread(program+size,1,200,infile);
    }
    if(argc<2){
        clearerr(stdin);
        rewind(stdin);
    }
    runstring(size,program);
    
    return 0;
}
