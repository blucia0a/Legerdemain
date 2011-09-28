#!/usr/bin/perl

warn "Starting\n";
while(<>){

  chomp;
  my @wds = split /\s+/;
  my $wd;
  foreach $wd(@wds){
    print "$wd 1\n";
  }
  $wd = undef;
  @wds = (); 

}
