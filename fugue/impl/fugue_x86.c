/*
 * Fugue-MIDI to x86 compiler
 *
 * Copyright (c) 2007 by Martin Rosenau
 *
 * Fugue is an esoteric programming language that uses
 * notes instead of letters as input.
 * The input file must be in MIDI file format
 */
/*
 * Comment added in January 2010:
 *
 * This program is FREEWARE and you can do with it
 * whatever you like.
 *
 * According the German law nearly all written text
 * (even texts like discussions on the "discussion"
 * page of Wiki articles) are copyrighted; therefore
 * I had to add the copyright notice at the top of
 * this file.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
typedef struct {
    unsigned stack:2;
    unsigned time:2;
    unsigned push:2;
    unsigned obj:3;
    unsigned data:2;
    unsigned submode:2;
    unsigned eofmode:2;
    long eofvalue;
} CompOptions;
#define STACK_ERR    1
//#define STACK_WRAP   2
#define STACK_ZERO   3
#define TIME_SECOND  1
#define TIME_FIRST   2
#define TIME_THIRD   3
#define OUTPUT_COFF 1
#define OUTPUT_RAW 2
#define OUTPUT_COFFH 3
#define OUTPUT_COFFC 4
#define OUTPUT_DISASSEMBLE 5
#define DATA_32 1
#define DATA_8 2
#define SUBMODE_NEG 1
#define SUBMODE_POS 2
#define EOFMODE_VALUE 1
#define EOFMODE_STOP 2
#define DEFAULT(x) if(!compoption.x) compoption.x=1
#define ARGCHECK(s,x,y) \
    else if(!stricmp(argv[i],s) && !compoption.x) \
    compoption.x=y;
#define CMD_POP 1
#define CMD_STACK_ABOVE 2
#define CMD_STACK_BELOW 3
#define CMD_ADD 4
#define CMD_SUBTRACT 5
#define CMD_LOOP_START 6
#define CMD_LOOP_END 7
#define CMD_GETCHAR 8
#define CMD_PUTCHAR 9
#define CMD_PUSH_OFFSET 30
#define CMD_PUSH_MIN 20
#define CMD_PUSH_MAX 40
const char * const disasm_cmds[]={NULL,
    "POP","-> PUSH ABOVE","PUSH BELOW <-",
    "ADD","SUBTRACT","LOOP START","LOOP END",
    "GETCHAR","PUTCHAR"};

/* returns a pointer to the string of the filename src
 * with a different file extension.
 * The pointer is only valid up to the next call of
 * this function but it is allowed to pass the pointer
 * as "src" argument to the next call of the function.
 * newext must already contain the "." */
const char *flncpy(const char *src,const char *newext)
{
    int i,l;
    static char dest[1000];
    for(i=0,l=(-1);src[i];i++)
    {
        dest[i]=src[i];
        if(src[i]=='.') l=i;
        else if(src[i]=='/' || src[i]=='\\' || src[i]==':')
            l=(-1);
    }
    if(l==(-1)) strcat(dest,newext);
    else strcpy(&(dest[l]),newext);
    return &(dest[0]);
}

/* Load a MIDI track and pre-interpret the data
 * in the Fugue way */
int LoadTrack(FILE *f,long **cmds,const CompOptions *options)
{
    unsigned char hdr[8],cmd,oldcmd;
    int n,c,c2,nnote;
    long timep,endpos,tdiff;
    /* Read the header */
    if(fread(hdr,1,8,f)!=8) hdr[0]=0;
    if(memcmp(hdr,"MTrk",4))
    {
        fprintf(stderr,"MIDI file format error.\n");
        return 5;
    }
    tdiff=(hdr[4]<<24)+(hdr[5]<<16)+(hdr[6]<<8)+hdr[7];
    endpos=ftell(f)+tdiff;
    *cmds=(long *)malloc(sizeof(long)*(tdiff+50));
    /* Read the track's data */
    for(cmd=0,timep=0,nnote=0;cmd>0x7F || !cmd;)
    {
        /* Read the time */
        tdiff=0;
        do {
            c=fgetc(f);
            tdiff=(tdiff<<7)|(c&0x7F);
        } while(c!=EOF && (c&0x80));
        timep+=tdiff;
        /* Get the command
         * Some compilers do not like fseek(f,-1,SEEK_CUR)
         * => We use fseek(f,ftell(f)-1,SEEK_SET) */
        if(ftell(f)>=endpos) c=EOF;
        else c=fgetc(f);
        if(c==EOF) cmd=2;
        else if(c<0x80) fseek(f,ftell(f)-1,SEEK_SET);
        else
        {
            oldcmd=cmd;
            cmd=c;
        }
        /* Now interpret the command
         * 0x9x: Play a tone */
        if(cmd>=0x90 && cmd<=0x9F)
        {
            c=fgetc(f);
            c2=fgetc(f);
            if(c2)
            {
                (*cmds)[2*nnote+2]=timep;
                (*cmds)[2*(nnote++)+3]=c;
            }
        }
        /* Three-byte commands we ignore */
        else if(cmd>=0x80 && cmd<=0xBF) fseek(f,2,SEEK_CUR);
        /* Two-byte commands we ignore */
        else if(cmd>=0xC0 && cmd<=0xF0) fgetc(f);
        /* Keyboard specific commands */
        else if(cmd==0xF0 || cmd==0xF7)
        {
            c=fgetc(f);
            if(c>0) fseek(f,c,SEEK_CUR);
            cmd=oldcmd;
        }
        /* Special commands */
        else if(cmd==0xFF)
        {
            c=fgetc(f);
            /* EOF */
            if(c==0x2F) cmd=1;
            else
            {
                c=fgetc(f);
                if(c>0) fseek(f,c,SEEK_CUR);
                cmd=oldcmd;
            }
        }
        /* Oups... */
        else cmd=2;
    }
    if(cmd!=1)
    {
        fprintf(stderr,"MIDI file format error.\n");
        return 5;
    }
    /* SEEK to the next track */
    fseek(f,endpos,SEEK_SET);
    /* There are no tones in this voice */
    if(!nnote)
    {
        free(*cmds);
        return (-1);
    }
    /* Pre-interpret the notes */
    for(c=0,n=1;n<nnote;n++)
    {
        c2=(*cmds)[2*n+3]-(*cmds)[2*n+1];
        /* PUSH command */
        if((c2>=3 && c2<=4) || (c2>=-4 && c2<=-3))
        {
            if(n+1<nnote)
            {
                c2=(*cmds)[2*n+5]-(*cmds)[2*n+3];
                if(c2>=-10 && c2<=10)
                {
                    if(options->push==TIME_FIRST)
                        (*cmds)[2*c]=(*cmds)[2*n];
                    else if(options->push==TIME_THIRD)
                        (*cmds)[2*c]=(*cmds)[2*n+4];
                    else (*cmds)[2*c]=(*cmds)[2*n+2];
                    (*cmds)[2*(c++)+1]=c2+CMD_PUSH_OFFSET;
                }
            }
            /* Skip one tone in every case. */
            n++;
        }
        /* Other commands */
        else if(c2>-10 && c2<10)
        {
            if(!c2) c2=CMD_POP;
            else if(c2>=1 && c2<=2) c2=CMD_STACK_ABOVE;
            else if(c2>=-2 && c2<=-1) c2=CMD_STACK_BELOW;
            else if(c2>=5 && c2<=6) c2=CMD_ADD;
            else if(c2>=-6 && c2<=5) c2=CMD_SUBTRACT;
            else if(c2==7) c2=CMD_LOOP_START;
            else if(c2==-7) c2=CMD_LOOP_END;
            else if(c2<0) c2=CMD_GETCHAR;
            else c2=CMD_PUTCHAR;
            if(options->time==TIME_FIRST)
                (*cmds)[2*c]=(*cmds)[2*n];
            else (*cmds)[2*c]=(*cmds)[2*n+2];
            (*cmds)[2*(c++)+1]=c2;
        }
    }
    (*cmds)[2*c]=(*cmds)[2*c+1]=(-1);
    return 0;
}

/* Ensure that an array is big enough */
void ArraySize(void *array,int size,long *max)
{
    if(size+100<(*max)) return;
    *max=size+2000;
    *(void **)array=realloc(*(void **)array,*max);
}

/* The main program */
int main(int argc,char **argv)
{
    int i,nerr,options,nvoices,
        *vptrs,bps,loopcmd,loopvoice,oldnerr;
    long **voices,binlen,maxbin,l,ctime,
        *loopstack,*exitptrs,
        maxloop,maxexit,nloop,nexit;
    const char *outfile,*infile,*funcname;
    unsigned char *binary;
    CompOptions compoption;
    unsigned char hdr[14];
    FILE *f;
    time_t curtime;
    /* Check the input syntax */
    outfile=infile=funcname=0;
    memset(&compoption,0,sizeof(compoption));
    for(i=nerr=1;i<argc && nerr;i++)
    {
        /* "/o" */
        if(!stricmp(argv[i],"/o") && !outfile && i+1<argc)
            outfile=argv[++i];
        /* "/fn" */
        else if(!stricmp(argv[i],"/fn") && !funcname && i+1<argc)
            funcname=argv[++i];
        /* "/eof" */
        else if(!stricmp(argv[i],"/eof") &&
            !funcname && i+1<argc && !compoption.eofmode)
        {
            compoption.eofvalue=atoi(argv[++i]);
            compoption.eofmode=EOFMODE_VALUE;
        }
        /* Other options */
        ARGCHECK("/eofstop",eofmode,EOFMODE_STOP)
        ARGCHECK("/coff",obj,OUTPUT_COFF)
        ARGCHECK("/coffh",obj,OUTPUT_COFFH)
        ARGCHECK("/coffc",obj,OUTPUT_COFFC)
        ARGCHECK("/dis",obj,OUTPUT_DISASSEMBLE)
        ARGCHECK("/raw",obj,OUTPUT_RAW)
        ARGCHECK("/8",data,DATA_8)
        ARGCHECK("/32",data,DATA_32)
        ARGCHECK("/subpos",submode,SUBMODE_POS)
        ARGCHECK("/subneg",submode,SUBMODE_NEG)
        ARGCHECK("/ostackerr",stack,STACK_ERR)
        // ARGCHECK("/ostackwrap",stack,STACK_WRAP)
        ARGCHECK("/ostackzero",stack,STACK_ZERO)
        ARGCHECK("/t1",time,TIME_FIRST)
        ARGCHECK("/t2",time,TIME_SECOND)
        ARGCHECK("/p1",push,TIME_FIRST)
        ARGCHECK("/p2",push,TIME_SECOND)
        ARGCHECK("/p3",push,TIME_THIRD)
        /* Must be the input file name */
        else if(infile || argv[i][0]=='/') nerr=0;
        else infile=argv[i];
    }
    if(!(infile && nerr))
    {
        fprintf(stderr,
            "\n\nFugue compiler v. 0.9\n"
            "Copyright (c) 2007 by Martin Rosenau\n\n"
            "Syntax:\n"
            "  fuguec mid_file [options]\n\n"
            "Output options:\n"
            "  /o output_file_name  (name of the output file)\n"
            "  /coff         (generate a COFF file - default)\n"
            "  /raw          (generate a RAW file)\n"
            "  /coffh        (generate a COFF file with header)\n"
            "  /coffc        (generate a COFF file with C wrapper)\n"
            "  /dis          (\"disassemble\" the MIDI file "
                "into a CSV table)\n"
            "  /fn funcname  (function name to generate - "
                "default: /fn fugue)\n\n"
            "Language options:\n"
            "  /8            (use 8 bit data)\n"
            "  /32           (use 32 bit data - default)\n"
            "  /subpos       (subtract "
                "top_of_stack-second_of_stack)\n"
            "  /subneg       (subtract "
                "second_of_stack-top_of_stack - default)\n\n"
            "Stack options:\n"
            "  /ostackerr    (don\'t compile if first track "
                "wants to read the stack\n"
            "                 of the previous track or the "
                "last track wants to read\n"
            "                 the next track\'s stack - default)\n"
            "  /ostackzero   (result is zero in the case "
                "mentioned above)\n"
            "  /ostackwrap   (wrap around from the last track "
                "to to first one\n"
            "                 NOT SUPPORTED IN THIS VERSION.)\n\n"
            "I/O options:\n"
            "  /eofstop      (stop if getchar() returns EOF)\n"
            "  /eof N        (translate EOF to N - "
                "default: /eof 0)\n\n"
            "Timing options:\n"
            "  *** Note that these options influence "
                "the \"disassembler\", too ! ***\n"
            "  /t1           (Time of a non-PUSH command is the "
                "first of the two notes)\n"
            "  /t2           (Time of a non-PUSH command is the "
                "second of the two\n"
            "                 notes - default)\n"
            "  /p1           (Time of a PUSH command is the first"
                "of three notes)\n"
            "  /p2           (Time of a PUSH command is the second"
                "of three notes - default)\n"
            "  /p3           (Time of a PUSH command is the third"
                "of three notes)\n");
        return 1;
    }
    /* Default values if nothing was specified */
    DEFAULT(stack);
    DEFAULT(time);
    DEFAULT(push);
    DEFAULT(obj);
    DEFAULT(data);
    DEFAULT(submode);
    DEFAULT(eofmode);
    /* Load the MIDI file:
     * Open it and check the header */
    f=fopen(infile,"rb");
    if(!f)
    {
        fprintf(stderr,"Could not open %s.\n",infile);
        return 2;
    }
    if(fread(hdr,1,14,f)!=14) hdr[0]=0;
    if(memcmp(hdr,"MThd",4))
    {
        fclose(f);
        fprintf(stderr,"File is not a MIDI file.\n");
        return 3;
    }
    fseek(f,8+(hdr[4]<<24)+(hdr[5]<<16)+
        (hdr[6]<<8)+hdr[7],SEEK_SET);
    if(hdr[8] || hdr[9]!=1)
        printf("** WARNING: ** - MIDI file "
            "format variant may be unsupported.\n");
    bps=(hdr[12]<<8)+hdr[13];
    /* Load all voices */
    voices=(long **)malloc(sizeof(long *)*((hdr[11]<<8)+hdr[12]));
    for(i=nvoices=nerr=0;i<(hdr[10]<<8)+hdr[11];i++)
    {
        nerr=LoadTrack(f,voices+nvoices,&compoption);
        if(nerr==(-1))
        { /* No notes in the track */ }
        else if(nerr==0) nvoices++;
        else
        {
            fclose(f);
            return nerr;
        }
    }
    fclose(f);
    /* Here voices[i] contains a pointer to the voice array
     * while i=0..(nvoices-1) is the voice (i+1):
     *   time1 (divide by (double)bps to get
     *          the time in quaters !)
     *   command1 (CMD_xxx)
     *   time2
     *   command2
     *   ...
     *   (-1) (marks EOF) */
    /* These are needed for the compiler
     * and for the disassembler */
    vptrs=(int *)malloc(nvoices*sizeof(int));
    memset(vptrs,0,nvoices*sizeof(int));
    /* "Disassemble" the Fugue program but
     * do not compile it. */
    if(compoption.obj==OUTPUT_DISASSEMBLE)
    {
        /* Open the CSV file */
        if(!outfile) outfile=flncpy(infile,".csv");
        printf("Writing %s ...\n",outfile);
        f=fopen(outfile,"wt");
        if(!f)
        {
            fprintf(stderr,"Could not create %s.\n",outfile);
            return 19;
        }
        /* Write the header */
        fprintf(f,"\"Time in quaters\"");
        for(i=0;i<nvoices;i++) fprintf(f,";\"Voice %u\"",i+1);
        /* Disassemble the program
         * The code has been taken and modified from the
         * compile code below! */
        while(1)
        {
            /* The last line has not been terminated by
             * a new line, yet! */
            fputc('\n',f);
            /* Find the next note (it is in any voice) */
            ctime=(-1);
            for(i=0;i<nvoices;i++)
                if(ctime==(-1) || (voices[i][vptrs[i]]<ctime))
                    ctime=voices[i][vptrs[i]];
            /* All voices have been done. */
            if(ctime==(-1)) break;
            /* Print the time */
            fprintf(f,"%f",ctime/(double)bps);
            /* OK. Perform all commands at this time in
             * each voice. */
            for(i=nerr=loopcmd=0;i<nvoices;i++)
            {
                fputc(';',f);
                if(ctime==voices[i][vptrs[i]])
                {
                    l=voices[i][vptrs[i]+1];
                    if(l>=CMD_PUSH_MIN)
                        fprintf(f,"\"PUSH %i\"",l-CMD_PUSH_OFFSET);
                    else fprintf(f,"\"%s\"",disasm_cmds[l]);
                    vptrs[i]+=2;
                }
            }
        }
        /* Disassembly is done... */
        fclose(f);
        return 0;
    }
    /* Initialize the compiler */
    binlen=maxbin=maxloop=maxexit=nloop=nexit=0;
    binary=0;
    loopstack=exitptrs=0;
    /* Code header
     * Real stack layout within the file:
     *   pgetchar (cdecl argument)
     *   pputchar (cdecl argument)
     *   Return address
     *   Original EBP
     *   Original ESI
     *   top-of-stack value for a loop command
     *   voice n stack pointer
     *   ...
     *   voice 2 stack pointer
     *   voice 1 stack pointer
     * -> New EBP
     *   0 (bottommost value of voice stack is always 0)
     *   ...
     *   0
     *   0
     *   voice n second bottommost stack value
     *   ...
     *   voice 1 second bottommost stack value
     *   voice n third bottommost stack value
     *   ...
     *   voice 1 third bottommost stack value
     *   ...
     * -> ESP is decresed when the stack size of a voice
     *    increses!
     */
    ArraySize(&binary,100,&maxbin);
    /* PUSH EBP
     * PUSH ESI
     * MOV EAX,ESP
     * SUB EAX,4*nvoices+8
     * PUSH EAX just to increment the stack */
    memcpy(binary,"\x55\x56\x89\xE0\x2D????\x50",10);
    l=4*nvoices+8;
    memcpy(binary+5,&l,4);
    binlen=10;
    /* For each voice do:
     * PUSH EAX
     * SUB EAX,4 */
    for(i=0;i<nvoices;i++)
    {
        memcpy(binary+binlen,"\x50\x83\xE8\4",4);
        /* In the last loop we remove the SUB EAX,4 */
        binlen+=(i+1==nvoices)?1:4;
    }
    binary[binlen++]=0x89; /* MOV EBP,ESP */
    binary[binlen++]=0xE5;
    /* For each voice do:
     * PUSH DWORD 0 */
    for(i=0;i<nvoices;i++)
    {
        binary[binlen++]=0x6A;
        binary[binlen++]=0;
    }
    /* OK. Now compile the program */
    while(1)
    {
        /* Find the next note (it is in any voice) */
        ctime=(-1);
        for(i=0;i<nvoices;i++)
            if(ctime==(-1) || (voices[i][vptrs[i]]<ctime))
                ctime=voices[i][vptrs[i]];
        /* All voices have been done. */
        if(ctime==(-1)) break;
        /* OK. Perform all commands at this time in
         * each voice.
         * Commands are executed from voice 1 to
         * voice n when they are executed "the same
         * time".
         * This is done to handle the "push value
         * from voice above" command:
         * nerr=0: This is voice 1 (i=0)
         * nerr=1: The stack of the voice above
         *         has not been modified and ESI has
         *         nothing to do with it.
         * // nerr=2: ESI points to the old ToS value
         * //         of the voice above (old=before
         * //         executing the command)
         * nerr=3: ESI+4*nvoices points to the old ToS ...
         * nerr=4: ESI-4*nvoices points to the old ToS ...
         * nerr=5: EAX contains the old ToS value */
        for(i=nerr=loopcmd=0;i<nvoices;i++)
        {
            if(ctime==voices[i][vptrs[i]])
            {
                oldnerr=nerr;
                /* Only check out what the stack will do
                 * Only in the case of CMD_STACK_ABOVE
                 * we may need to load the ToS because
                 * ESI will be overwritten in the
                 * next command... */
                switch(voices[i][vptrs[i]+1])
                {
                  case CMD_ADD:
                  case CMD_SUBTRACT:
                    nerr=5;
                    break;
                  case CMD_POP:
                  case CMD_PUTCHAR:
                    nerr=4;
                    break;
                  case CMD_LOOP_START:
                  case CMD_LOOP_END:
                    nerr=1;
                    break;
                  case CMD_STACK_ABOVE:
                    if(oldnerr==3 || oldnerr==4)
                    {
                        ArraySize(&binary,100+binlen,&maxbin);
                        /* MOV EAX,[ESI +/- 4] */
                        binary[binlen++]=0x8B;
                        binary[binlen++]=0x86;
                        l=((oldnerr==3)?4:(-4))*nvoices;
                        memcpy(binary+binlen,&l,4);
                        binlen+=4;
                        oldnerr=5;
                    }
                    nerr=3;
                    break;
                  default:
                    nerr=3;
                }
                /* Load the voice stack pointer:
                 * MOV ESI,[EBP+4*voice]
                 * This is not needed for loop
                 * commands but the array is already
                 * resized! */
                ArraySize(&binary,100+binlen,&maxbin);
                if(nerr!=1)
                {
                    l=4*i;
                    binary[binlen++]=0x8B;
                    binary[binlen++]=0xB5;
                    memcpy(binary+binlen,&l,4);
                    binlen+=4;
                }
                /* Case PUSH: modify the voice
                 * stack pointer and the stack
                 * pointer if necessary
                 *   SUB ESI,4*nvoices
                 *   CMP ESI,ESP
                 *   JAE dontdo
                 *   MOV ESP,ESI
                 *  dontdo: */
                if(nerr==3)
                {
                    memcpy(binary+binlen,
                        "\x81\xee????\x39\xe6\x73\2\x89\xf4",12);
                    l=4*nvoices;
                    memcpy(binary+binlen+2,&l,4);
                    binlen+=12;
                }
                /* The actual command */
                switch(voices[i][vptrs[i]+1])
                {
                  case CMD_POP:
                    /* Nothing must be done here ! */
                    break;
                  case CMD_STACK_ABOVE:
                    if(oldnerr==1)
                    {
                        /* MOV EAX,[EBP+4*(voice-1)]
                         * MOV EAX,[EAX]
                         * MOV [ESI],EAX */
                        memcpy(binary+binlen,
                            "\x8B\x85????\x8B\0\x89\6",10);
                        l=4*(i-1);
                        memcpy(binary+binlen+2,&l,4);
                        binlen+=10;
                    }
                    else if(oldnerr==5)
                    {
                        /* MOV [ESI],EAX */
                        binary[binlen++]=0x89;
                        binary[binlen++]=6;
                    }
                    else if(compoption.stack==STACK_ZERO)
                    {
                        /* MOV DWORD PTR [ESI],0 */
                        memcpy(binary+binlen,"\xC7\6\0\0\0\0",6);
                        binlen+=6;
                    }
                    else
                    {
                        fprintf(stderr,"Time=%f quarters:\n"
                            "  The first voice wants to read the "
                            "stack of the voice above.\n"
                            "  This is not allowed in "
                            "/ostackerr mode.",ctime/(double)bps);
                        return 22;
                    }
                    break;
                  case CMD_STACK_BELOW:
                    if(i+1<nvoices)
                    {
                        /* MOV EAX,[EBP+4*(voice+1)]
                         * MOV EAX,[EAX]
                         * MOV [ESI],EAX */
                        memcpy(binary+binlen,
                            "\x8B\x85????\x8B\0\x89\6",10);
                        l=4*(i+1);
                        memcpy(binary+binlen+2,&l,4);
                        binlen+=10;
                    }
                    else if(compoption.stack==STACK_ZERO)
                    {
                        /* MOV DWORD PTR [ESI],0 */
                        memcpy(binary+binlen,"\xC7\6\0\0\0\0",6);
                        binlen+=6;
                    }
                    else
                    {
                        fprintf(stderr,"Time=%f quarters:\n"
                            "  The last voice wants to read the "
                            "stack of the voice below.\n"
                            "  This is not allowed in "
                            "/ostackerr mode.",ctime/(double)bps);
                        return 23;
                    }
                    break;
                  case CMD_ADD:
                    /* MOV EAX,[ESI]
                     *   ; check if the stack has at
                     *   ; least 2 elements
                     *   ; do nothing if not
                     * ADD ESI,8*nvoices
                     * CMP ESI,EBP
                     * JAE end_of_this
                     *   ; Do the action
                     * SUB ESI,4*nvoices
                     * MOV [EBP+4*voice],ESI
                     * MOV [ESI],EAX */
                    memcpy(binary+binlen,"\x8B\6\x81\xC6????"
                        "\x39\xEE\x73\xE\x81\xEE????"
                        "\x89\xB5????\1\6",26);
                    l=8*nvoices;
                    memcpy(binary+binlen+4,&l,4);
                    l=4*nvoices;
                    memcpy(binary+binlen+14,&l,4);
                    l=4*i;
                    memcpy(binary+binlen+20,&l,4);
                    binlen+=26;
                    break;
                  case CMD_SUBTRACT:
                    if(compoption.submode==SUBMODE_NEG)
                    {
                        /* MOV EAX,[ESI]
                         *   ; This is only there for the
                         *   ; case that there is exactly
                         *   ; one element on the stack.
                         * NEG DWORD PTR [ESI]
                         *   ; check if the stack has at
                         *   ; least 2 elements
                         *   ; do nothing if not
                         * ADD ESI,8*nvoices
                         * CMP ESI,EBP
                         * JAE end_of_this
                         *   ; Do the action
                         * SUB ESI,4*nvoices
                         * MOV [EBP+4*voice],ESI
                         * SUB [ESI],EAX */
                        memcpy(binary+binlen,"\x8B\6\xF7\x1E"
                            "\x81\xC6????\x39\xEE\x73\xE"
                            "\x81\xEE????\x89\xB5????\x29\6",28);
                        l=8*nvoices;
                        memcpy(binary+binlen+6,&l,4);
                        l=4*nvoices;
                        memcpy(binary+binlen+16,&l,4);
                        l=4*i;
                        memcpy(binary+binlen+22,&l,4);
                        binlen+=28;
                        break;
                    }
                    else
                    {
                        /* MOV EAX,[ESI]
                         *   ; check if the stack has at
                         *   ; least 2 elements
                         *   ; do nothing if not
                         * ADD ESI,8*nvoices
                         * CMP ESI,EBP
                         * JAE end_of_this
                         *   ; Do the action
                         * SUB ESI,4*nvoices
                         * MOV [EBP+4*voice],ESI
                         * SUB [ESI],EAX
                         * NEG DWORD PTR [ESI] */
                        memcpy(binary+binlen,"\x8B\6\x81\xC6????"
                            "\x39\xEE\x73\x10\x81\xEE????"
                            "\x89\xB5????\x29\6\xF7\x1E",28);
                        l=8*nvoices;
                        memcpy(binary+binlen+4,&l,4);
                        l=4*nvoices;
                        memcpy(binary+binlen+14,&l,4);
                        l=4*i;
                        memcpy(binary+binlen+20,&l,4);
                        binlen+=28;
                        break;
                    }
                    break;
                  case CMD_LOOP_START:
                  case CMD_LOOP_END:
                    if(loopcmd)
                    {
                        fprintf(stderr,"Time=%f quarters:\n"
                            "  Two loop commands in voices %u"
                            " and %u at the same time.\n",
                            ctime/(double)bps,loopvoice+1,i+1);
                        return 20;
                    }
                    loopcmd=voices[i][vptrs[i]+1];
                    loopvoice=i;
                    break;
                  case CMD_GETCHAR:
                    /* CALL [EBP+4*nvoices+20]
                     * CMP EAX,(-1) */
                    memcpy(binary+binlen,
                        "\xFF\x95????\x83\xF8\xFF",9);
                    l=4*nvoices+20;
                    memcpy(binary+binlen+2,&l,4);
                    binlen+=9;
                    if(compoption.eofmode==EOFMODE_STOP)
                    {
                        /* JE end_of_file
                         * The last 4 bytes are set later! */
                        binary[binlen++]=0xF;
                        binary[binlen++]=0x84;
                        ArraySize(&exitptrs,
                            sizeof(long)*(nexit+1),&maxexit);
                        exitptrs[nexit++]=binlen;
                        binlen+=4;
                    }
                    else
                    {
                        /* JNE +5
                         * MOV EAX,value */
                        binary[binlen++]=0x75;
                        binary[binlen++]=5;
                        binary[binlen++]=0xB8;
                        memcpy(binary+binlen,&(compoption.eofvalue),4);
                        binlen+=4;
                    }
                    /* MOV [ESI],EAX */
                    binary[binlen++]=0x89;
                    binary[binlen++]=6;
                    break;
                  case CMD_PUTCHAR:
                    /* MOV EAX,[ESI] */
                    binary[binlen++]=0x8B;
                    binary[binlen++]=6;
                    if(compoption.data==DATA_8)
                    {
                        /* MOVZX EAX,AL */
                        binary[binlen++]=0xF;
                        binary[binlen++]=0xB6;
                        binary[binlen++]=0xC0;
                    }
                    binary[binlen++]=0x50; /* PUSH EAX */
                    /* CALL [EBP+4*nvoices+16] */
                    binary[binlen++]=0xFF;
                    binary[binlen++]=0x95;
                    l=4*nvoices+16;
                    memcpy(binary+binlen,&l,4);
                    binlen+=4;
                    /* POP EAX just to restore the stack */
                    binary[binlen++]=0x58;
                    break;
                  default: /* Push a value */
                    l=voices[i][vptrs[i]+1]-CMD_PUSH_OFFSET;
                    /* MOV DWORD PTR [ESI],pused_value */
                    binary[binlen++]=0xC7;
                    binary[binlen++]=0x06;
                    memcpy(binary+binlen,&l,4);
                    binlen+=4;
                }
                /* Update the voice's stack pointer */
                if(nerr==4)
                {
                    /* ADD ESI,4*nvoices
                     * CMP ESI,EBP
                     * JAE +6 (skip the MOV [EBP+x],ESI) */
                    memcpy(binary+binlen,
                        "\x81\xC6????\x39\xEE\x73\6",10);
                    l=4*nvoices;
                    memcpy(binary+binlen+2,&l,4);
                    binlen+=10;
                }
                if(nerr==3 || nerr==4)
                {
                    /* MOV [EBP+4*voice],ESI */
                    l=4*i;
                    binary[binlen++]=0x89;
                    binary[binlen++]=0xB5;
                    memcpy(binary+binlen,&l,4);
                    binlen+=4;
                }
                vptrs[i]+=2;
            }
            else nerr=1;
            /* Loop_start and Loop_end commands
             * are actually handled here! */
            if(loopcmd==CMD_LOOP_START)
            {
                /* MOV ESI,[EBP+4*loopvoice]
                 * JMP xxx (will be updated later) */
                memcpy(binary+binlen,"\x8B\xB5????\xE9",7);
                l=4*loopvoice;
                memcpy(binary+binlen+2,&l,4);
                /* Remember the loop's position */
                ArraySize(&loopstack,
                    sizeof(long)*(3*nloop+3),&maxloop);
                loopstack[3*nloop]=ctime;
                loopstack[3*nloop+1]=loopvoice;
                loopstack[3*(nloop++)+2]=binlen+7;
                binlen+=11;
            }
            else if(loopcmd==CMD_LOOP_END)
            {
                if(nloop<1)
                {
                    fprintf(stderr,"Time=%f quarters:\n"
                        "  Loop End command found outside "
                        "a loop.\n",
                        ctime/(double)bps,loopvoice+1,i+1);
                    return 22;
                }
                /* MOV ESI,[EBP+4*loopvoice] */
                binary[binlen++]=0x8B;
                binary[binlen++]=0xB5;
                l=4*loopvoice;
                memcpy(binary+binlen,&l,4);
                binlen+=4;
                /* Update the "JMP" instruction of the
                 * loop begin command - it jumps here */
                l=binlen-loopstack[3*(--nloop)+2]-4;
                memcpy(binary+loopstack[3*nloop+2],&l,4);
                /* CMP BYTE/WORD PTR [ESI],0 */
                binary[binlen++]=(compoption.data==DATA_8)?0x80:0x83;
                binary[binlen++]=0x3E;
                binary[binlen++]=0;
                /* JNE xxx */
                binary[binlen++]=0xF;
                binary[binlen++]=0x85;
                l=loopstack[3*nloop+2]-binlen;
                memcpy(binary+binlen,&l,4);
                binlen+=4;
            }
        }
    }
    /* Open loops ? */
    if(nloop)
    {
        fprintf(stderr,"ERROR: The following "
            "loop(s) is/are not terminated:\n");
        for(i=0;i<nloop;i++)
            fprintf(stderr,
                "  Starting time: %f quaters, "
                "starting in voice %u\n",
                loopstack[3*i]/(double)bps,
                loopstack[3*i+1]+1);
        return 21;
    }
    /* Code trailer */
    for(i=0;i<nexit;i++)
    {
        /* Update the "JE" instructions for /EOFSTOP */
        l=binlen-exitptrs[i]-4;
        memcpy(binary+exitptrs[i],&l,4);
    }
    ArraySize(&binary,100+binlen,&maxbin);
    binary[binlen++]=0x81; /* ADD EBP,4*nvoices+4 */
    binary[binlen++]=0xC5;
    l=4*nvoices+4;
    memcpy(binary+binlen,&l,4);
    binlen+=4;
    /* MOV ESP,EBP
     * POP ESI
     * POP EBP
     * RET */
    memcpy(binary+binlen,"\x89\xEC\x5E\x5D\xC3",5);
    binlen+=5;
    /* Alignment */
    while(binlen&7) binary[binlen++]=0x90; /* NOP */
    /* Print some statistics */
    printf("Compiling complete. Code size: %u bytes.\n",binlen);
    /* Build the output file name and open the output file */
    if(!funcname) funcname="fugue";
    if(!outfile) outfile=flncpy(infile,
        (compoption.obj==OUTPUT_RAW)?".bin":".obj");
    printf("Writing %s ...\n",outfile);
    f=fopen(outfile,"wb");
    if(!f)
    {
        fprintf(stderr,"Could not write %s.\n",outfile);
        return 10;
    }
    /* Write the COFF file stuff (not including the raw data) */
    if(compoption.obj!=OUTPUT_RAW)
    {
        fwrite("\x4C\1\1\0",1,4,f); /* HDR, 1 section */
        time(&curtime);
        fwrite(&curtime,1,4,f);
        /* Symtab following first section entry;
         * lenght = 1 symbol */
        fwrite("\x3C\0\0\0\1\0\0\0\0\0\0\0"
            /* Only section entry: ".text" */
            ".text\0\0\0\0\0\0\0\0\0\0\0",1,28,f);
        l=(strlen(funcname)+0x3C+18+9)&~3;
        fwrite(&binlen,1,4,f); /* Length of section */
        fwrite(&l,1,4,f); /* Pos. of section */
        /* No relocations, no debug symbols, Code */
        fwrite("\0\0\0\0\0\0\0\0\0\0\0\0\x20\0\0\x60"
            /* Symbol 1: Pos=4 in name table
             * Offset = 0, Section = 1, Global */
            "\0\0\0\0\4\0\0\0\0\0\0\0\1\0\0\0\2\0",1,34,f);
        /* Symbol name table */
        l=(8+strlen(funcname))&~3;
        fwrite(&l,1,4,f);
        fputc('_',f);
        fwrite(funcname,1,strlen(funcname)+1,f);
        l=strlen(funcname)+4;
        while((l++)&3) fputc(0,f);
    }
    /* Write the binary data */
    fwrite(binary,1,binlen,f);
    fclose(f);
    /* Build the header file */
    if(compoption.obj==OUTPUT_COFFH || compoption.obj==OUTPUT_COFFC)
    {
        outfile=flncpy(outfile,
            (compoption.obj==OUTPUT_COFFH)?".h":"_wrp.c");
        printf("Writing %s ...\n",outfile);
        f=fopen(outfile,"wt");
        if(!f)
        {
            fprintf(stderr,"Could not write %s.\n",outfile);
            return 11;
        }
        fprintf(f,"/*\n"
            " * This file belongs to an object file\n"
            " * generated by the Fugue compiler.\n"
            " */\n\n");
        if(compoption.obj==OUTPUT_COFFC)
            fprintf(f,"#include <stdio.h>\n\n");
        fprintf(f,"extern void __cdecl %s(\n"
            "        /* pputchar\'s retuned value is ignored ! */\n"
            "        int (__cdecl *pputchar)(int),\n"
            "        int (__cdecl *pgetchar)(void));\n\n",funcname);
        if(compoption.obj==OUTPUT_COFFC)
            fprintf(f,
                "\n/* In cygwin getchar() and "
                "putchar() do __NOT__ work!!\n"
                " * They are #defines there and not "
                "__cdecl functions! */\n"
                "int __cdecl my_putchar(int c)\n"
                "{\n    return putchar(c);\n}\n"
                "int __cdecl my_getchar(void)\n"
                "{\n    return getchar();\n}\n\n"
                "int main(int argc,char **argv)\n"
                "{\n    %s(my_putchar,my_getchar);\n"
                "    fflush(stdout);\n"
                "    return 0;\n}\n",funcname);
        fclose(f);
    }
    return 0;
}
