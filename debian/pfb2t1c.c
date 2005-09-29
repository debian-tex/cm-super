#define DUMMY \
set -ex; \
gcc -DNDEBUG=1 -s -O3 -ansi -pedantic \
  -Wall -W -Wstrict-prototypes -Wnested-externs -Winline \
  -Wpointer-arith -Wbad-function-cast -Wcast-qual -Wmissing-prototypes \
  -Wmissing-declarations "$0" -o pfb2t1c; \
exit

/* pfb2t1c.c -- convert .pfb (file) to .t1c (stdout)
 * by pts@fazekas.hu at Sat Mar 29 21:24:20 CET 2003
 * works at Sat Mar 29 22:19:28 CET 2003
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
static char* eexec_decrypt(char* buf, unsigned long len, unsigned long seed) {
  unsigned char p; /* unsigned is important */
  while (len--!=0) {
    p=*buf^(seed>>8);
    seed=0xFFFFU&((*(unsigned char*)buf+seed)*52845U+22719U);
    *buf++=p;
  }
  return buf;
}

int main(int argc, char** argv) {
  int c;
  unsigned long len, inlen;
  unsigned u;
  char *p, *buf, *pend, *bufend, *t, *next, *out, *filename;
  
  if (argc==2) {
    if (NULL==(freopen(filename=argv[1], "rb", stdin))) {
      fprintf(stderr, "pfb2t1c: cannot fopen(): %s\n", argv[1]); exit(13);
    }
  } else filename="(stdin).pfb";
  #ifdef WIN32
    setmode(0, O_BINARY);
  #endif
  if (0!=fseek(stdin, 0, SEEK_END)) { fprintf(stderr, "pfb2t1c: fseek 1 failed -- input must be seekable\n"); exit(16); }
  inlen=ftell(stdin);
  if (0!=fseek(stdin, 0, SEEK_SET)) { fprintf(stderr, "pfb2t1c: fseek 2 failed\n"); exit(17); }
  if (NULL==(buf=(char*)malloc(inlen))) { fprintf(stderr, "pfb2t1c: out of memory\n"); exit(6); }
  if (inlen!=fread(buf, 1, inlen, stdin)) { fprintf(stderr, "pfb2t1c: unexpected EOF\n"); exit(8); }
  /* Dat: `buf' will be used for both input and output. `t' points to the next
   * character to be read, `out' points to the next character to be written
   */
  bufend=buf+inlen;
  out=t=buf;
  /* fprintf(stderr, "%u..\n", bufend-t); */
  while (1) {
    if (bufend-t>=2 && t[0]=='\200' && t[1]=='\003') break; /* EOD (EOF) */
    if (bufend-t<6 || t[0]!='\200' || (t[1]!='\001' && t[1]!='\002')) {
      fprintf(stderr, "pfb2t1c: bad block header -- maybe not a .pfb\n"); exit(2);
    }
    len=2[(unsigned char*)t] | 3[(unsigned char*)t]<<8 | 4[(unsigned char*)t]<<16 | 5[(unsigned char*)t]<<24;
    next=t+6+len;
    #if 0
      fprintf(stderr, "read block type=%d len=%lu\n", t[1], len);
    #endif
    if (t[1]==1 && len>=32 && 0==memcmp(t+6, "00000000000000000000000000000000000", 32)) {
      #if 0
        fprintf(stderr, "ignoring block with `cleartomark'\n");
      #endif
    } else if (t[1]==1) { /* text block */
      /* Imp: check for \s+ between currentfile and eexec */
      /* Imp: PostScript definition of \s */
      /* Imp: create proc for scanning the end of the string */
      t+=6;
      while (len!=0 && ((c=t[len-1])==' ' || c=='\t' || c=='\r' || c=='\n')) len--;
      if (len< 5 || 0!=memcmp(t+(len-= 5), "eexec", 5)) { err_cee: fprintf(stderr, "pfb2t1c: `currentfile eexec' expected\n"); exit(15); }
      while (len!=0 && ((c=t[len-1])==' ' || c=='\t' || c=='\r' || c=='\n')) len--;
      if (len<11 || 0!=memcmp(t+(len-=11), "currentfile", 11)) goto err_cee;
      while (len!=0 && ((c=t[len-1])==' ' || c=='\t' || c=='\r' || c=='\n')) len--;
      t[len++]='\n'; t[len++]='e'; t[len++]='\n';
      memmove(out, t, len); out+=len; 
    } else if (t[1]==2) { /* eexec-encrypted binary block */
      if (len<32) { fprintf(stderr, "pfb2t1c: eexeced data too short\n"); exit(18); }
      t+=6;
      eexec_decrypt(t, len, SEED_EEXEC);

      /* Remove trailer of encrypted code */
      while (len!=0 && ((c=t[len-1])==' ' || c=='\t' || c=='\r' || c=='\n')) len--;
      if (len<26 || 0!=memcmp(t+(len-=26), "mark currentfile closefile", 26)) { fprintf(stderr, "pfb2t1c: `dup/FontName' expected\n"); exit(16); }
      while (len!=0 && ((c=t[len-1])==' ' || c=='\t' || c=='\r' || c=='\n')) len--;
      if (len<36 || 0!=memcmp(t+(len-=36), "dup/FontName get exch definefont pop", 36)) { fprintf(stderr, "pfb2t1c: `dup/FontName' expected\n"); exit(17); }
      while (len!=0 && ((c=t[len-1])==' ' || c=='\t' || c=='\r' || c=='\n')) len--;
      t[len++]='\n';
      strcpy(t+len, " -|"); /* sentinel */
      
      /* Decrypt individual charstrings */
      u=0; p=t; pend=t+len;
      while (1) {
        c=*p++;
        if (c-'0'+0U<='9'+0U) { u=10*u+c-'0'; }
        else if (c==' ' && p[0]=='-' && p[1]=='|') { /* eexec encoding */
          if (p>pend) break;
          if (p[2]!=' ') { fprintf(stderr, "pfb2t1c: bad charstring lead\n"); exit(12); }
          p=eexec_decrypt(p+=3, u, SEED_CHARSTRINGS);
        } else u=0;
      }
      
      memmove(out, t+4, len-4); out+=len-4; /* Dat: +4: skip eexec extra seed */
    }
    t=next;
  }
  printf("%%!t1c %lu %s\n", 0UL+(out-buf), filename);
  fwrite(buf, 1, out-buf, stdout);
  free(buf);
  if (ferror(stdin)) { fprintf(stderr, "error reading\n"); exit(7); }
  if (ferror(stdout)) { fprintf(stderr, "error writing\n"); exit(14); }
  return 0;
}
