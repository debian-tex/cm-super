#! /usr/bin/perl -w
#
# t1c2pfb.pl -- a direct rip of t1c2pfb.c
# by pts@fazekas.phu at Sun Mar 30 11:30:12 CEST 2003
#
use integer;
use strict;

#** Modifies $_[0] in place
sub eexec_encrypt($$) {
  my($seed,$len,$i,$c)=($_[1],length($_[0]),0);
  while ($i<$len) {
    $c=vec($_[0],$i,8)^($seed>>8);
    $seed=0xFFFF&(($c+$seed)*52845+22719);
    vec($_[0],$i++,8)=$c;
  }
}

if (@ARGV==1) {
  die "$0: $!" if !open STDIN, "< $ARGV[0]";
}
binmode STDIN;
while (<STDIN>) {
  die if !/^%!t1c (\d+) (.*)\n\Z(?!\n)/;
  my $len=0+$1;
  my $filename=$2;
  print STDERR "extracting filename=($filename) from_len=$len\n";
  my $buf;
  die if $len!=read STDIN, $buf, $len;
  unlink $filename;
  die "$0: > $filename: $!\n" if! open OF, "> $filename";
  binmode OF;
  my $i=index($buf,"\ne\n");
  die if $i<4;
  my $S=substr($buf,0,$i)."\ncurrentfile eexec\n";
  $buf=substr($buf,$i-1);
  substr($buf,0,4)="\0\0\0\0";
  die unless print OF pack("nV", 0x8001, length($S)), $S;
  
  my $j;
  for ($i=0; 0<=($i=index($buf," -| ",$i)); $i+=4+$j) {
    # Dat: `$buf=~/(\d+) -[|] /g' was very-very slow, so I chose index()
    ## print STDERR "$i\n";
    die if 0>($j=rindex($buf,' ',$i-1));
    $j=0+substr($buf,$j,$i-$j); # length of string;
    # substr($buf,$i,4)=" XYZ";
    eexec_encrypt(substr($buf,$i+4,$j), 4330); # sloow
  }
  $buf.="dup/FontName get exch definefont pop\nmark currentfile closefile\n";
  eexec_encrypt($buf, 55665); # sloow
  die unless print OF
    pack("nV", 0x8002, length($buf)),
    $buf,
    "\200\001\024\002\000\000", (scalar(("0"x64)."\n")x8), "cleartomark\n\200\003";
  die unless close OF;
}
# Imp: die if ferror(stdin), ferror(of)
