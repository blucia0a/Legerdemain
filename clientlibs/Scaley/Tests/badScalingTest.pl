#!/usr/bin/perl

my %dict;
while(<>){

  chomp;
  my @wds = split /\s+/;
  my $wd;
  foreach $wd(@wds){
    if(exists $dict{$wd}){
      $dict{$wd}++;
    }else{
      $dict{$wd} = 1;
    }
  }
  

}
