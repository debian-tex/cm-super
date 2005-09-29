#define DUMMY \
set -ex; \
gcc -DNDEBUG=1 -s -O3 -ansi -pedantic \
  -Wall -W -Wstrict-prototypes -Wnested-externs -Winline \
  -Wpointer-arith -Wbad-function-cast -Wcast-qual -Wmissing-prototypes \
  -Wmissing-declarations "$0" -o t1c2pfb; \
exit

/* t1c2pfb.c -- convert from small .t1c (stdin) to binary Type1 PFB (file)
 * by pts@fazekas.hu at Sat Mar 29 18:24:48 CET 2003
 */
#include <stdio.h>
#include <stdlib.h> /* malloc() */
#include <string.h>
#ifdef MSDOS
#  define WIN32 1
#endif
#ifdef WIN32
#  include <fcntl.h>
#  include <io.h>
#endif

#define SEED_EEXEC 55665UL
#define SEED_CHARSTRINGS 4330UL

/** @return buf+len */
static char* eexec_encrypt(char* buf, unsigned long len, unsigned long seed) {
  unsigned char c; /* unsigned is important */
  while (len--!=0) {
    c=*buf^(seed>>8);
    seed=0xFFFFU&((c+seed)*52845U+22719U);
    *buf++=c;
  }
  return buf;
}

#define BLOCK_ASCII '\001'
#define BLOCK_BINARY '\002'
/** Writes a PFB ASCII or binary block */
static void pfb_write(char ty, char* buf, unsigned long len, long pd, FILE *of) {
  char head[6];
  head[0]='\200'; head[1]=ty;
  head[2]=len; head[3]=len>>8; head[4]=len>>16; head[5]=len>>24;
  fwrite(head, 1, 6, of);
  fwrite(buf, 1, len+pd, of);
}

int main(int argc, char** argv) {
  FILE *of;
  int c;
  unsigned long len, k;
  unsigned u;
  char *p, *buf, *bufend, *bufee, *q;
  char t[6];
  char filename[80];
  
  if (argc==3) return 0;
  if (argc==2) {
    if (NULL==(freopen(argv[1], "rb", stdin))) {
      fprintf(stderr, "t1c2pfb: cannot fopen(): %s\n", argv[1]); exit(13);
    }
  }
  #ifdef WIN32
    setmode(0, O_BINARY);
  #endif
  while (0!=(len=fread(t, 1, 6, stdin))) {
    if (6!=len || 0!=memcmp(t, "%!t1c ", 6)) {
      fprintf(stderr, "t1c2pfb: stdin is not a .t1c\n");
      exit(2);
    }
    if (1!=scanf("%lu ", &len)) { fprintf(stderr, "t1c2pfb: incomplete len\n"); exit(3); }
    p=filename; k=sizeof(filename);
    while (1) {
      if (0>(c=getchar())) { fprintf(stderr,"t1c2pfb: incomplete filename\n"); exit(4); }
      if (c=='\n' || c=='\0') break;
      *p++=c;
      if (0==--k) { fprintf(stderr,"t1c2pfb: filename too long\n"); exit(5); }
    }
    *p='\0';
    fprintf(stderr, "extracting filename=(%s) from_len=%lu\n", filename, len);
    if (NULL==(buf=(char*)malloc(len+100))) { fprintf(stderr, "t1c2pfb: out of memory\n"); exit(6); }
    if (len!=fread(buf, 1, len, stdin)) { fprintf(stderr, "t1c2pfb: EOF inside .t1c content\n"); exit(8); }
    strcpy(bufend=buf+len, " -| \ne\n"); /* sentinels */
    remove(filename);
    if (NULL==(of=fopen(filename, "wb"))) { fprintf(stderr, "t1c2pfb: cannot rewrite outfile\n"); exit(9); }

    /* reproduce plain part */
    p=buf; while (p[0]!='\n' || p[1]!='e' || p[2]!='\n') p++;
    if (p>bufend) { fprintf(stderr, "t1c2pfb: incomplete plain part\n"); exit(10); }
    if (p-buf<32) { fprintf(stderr, "t1c2pfb: plain part too short\n"); exit(14); }
    bufee=p+3;
    k=strlen(q="currentfile eexec\n");
    pfb_write(BLOCK_ASCII, buf, p+1-buf+k, -k, of);
    fputs(q, of);
    
    /* Encrypt individual charstrings */
    u=0; p=bufee;
    while (1) {
      c=*p++;
      if (c-'0'+0U<='9'+0U) { u=10*u+c-'0'; }
      else if (c==' ' && p[0]=='-' && p[1]=='|') { /* eexec encoding */
        if (p>bufend) break;
        if (p[2]!=' ') { fprintf(stderr, "t1c2pfb: bad charstring lead\n"); exit(12); }
        #if 0
          fprintf(stderr, "eexec %u\n", u);
        #endif
        #if 0
          p[0]='\236'; p[1]='\264'; p[2]='\365'; p[3]='\225'; /* arbitrary, but encrypted must be binary */
        #endif
        p=eexec_encrypt(p+=3, u, SEED_CHARSTRINGS);
      } else u=0;
    }
    
    /* Encrypt everything after bufee */
    strcpy(bufend, "dup/FontName get exch definefont pop\nmark currentfile closefile\n");
    while (*bufend!='\0') bufend++;
    *--bufee='\0';  *--bufee='\0';  *--bufee='\0';  *--bufee='\0';
    eexec_encrypt(bufee, bufend-bufee, SEED_EEXEC);
    /* ^^^ Imp: dump additional bytes? */
    
    /* dump the results */
    #if 0
      pfb_write(BLOCK_BINARY, bufee, bufend-bufee+532, -532, of);
      /* ^^^ 532 is the length till the end of "cleartomark\n" */
    #else
      pfb_write(BLOCK_BINARY, bufee, bufend-bufee, 0, of);
      fwrite("\200\001\024\002\000\000", 1, 6, of); /* 532 */
    #endif
    for (k=8; k--!=0; ) fputs("0000000000000000000000000000000000000000000000000000000000000000\n", of);
    fputs("cleartomark\n\200\003", of);
    if (ferror(of)) { fprintf(stderr, "t1c2pfb: error writing outfile\n"); exit(11); }
    fclose(of);
    free(buf);
  }
  if (ferror(stdin)) {
    fprintf(stderr, "error reading\n");
    exit(7);
  }
  return 0;
}
